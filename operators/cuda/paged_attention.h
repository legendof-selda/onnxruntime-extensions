// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "ocos.h"
#include "cuda_type.h"
#include "paged_attention_impl.h"

template <typename T>
using UniquePtrWithDeletor = std::unique_ptr<T, std::function<void(T*)>>;

template <typename T>
inline UniquePtrWithDeletor<T> GetScratchBuffer(void* p, OrtAllocator* allocator) {
  return UniquePtrWithDeletor<T>{static_cast<T*>(p), [allocator = std::move(allocator)](T* p) {
                                  allocator->Free(allocator, p);
                                }};
}

struct AttnBias {
  typedef struct {
    int64_t seqstart;
    int64_t max_seqlen;
    int64_t seqstart_py;
  } block_tables;
  block_tables q_seqinfo;
  int64_t batchsize;
};

struct InputMetadata {
  //int64_t schedule_type;  // 0: vllm. 1:sarathi, 2:custom, 3:self-build
  //int64_t block_tables;
  int64_t max_num_blocks_per_seq;
  //int64_t context_lens;
  int64_t max_context_len = 0;
  int64_t num_prompt_tokens = 0;
  int64_t num_valid_tokens = 0;
  //int64_t slot_mapping;
  int64_t num_generation_tokens = 0;
  AttnBias attn_bias;
  UniquePtrWithDeletor<int64_t> position_ids; 
};

//// TODO(leca): remove unnecessary parameters, move all cuda call to .cu file and check return value by calling CudaCall().
template <typename T>
OrtStatusPtr CheckInputs(const cudaStream_t stream, OrtAllocator* allocator, const ortc::Tensor<T>& query, const ortc::Tensor<T>& key,
                         const ortc::Tensor<T>& value, const ortc::Tensor<T>& key_cache, const ortc::Tensor<T>& value_cache,
                         const ortc::Tensor<int32_t>& block_tables, const ortc::Tensor<int32_t>& slot_mappings, 
                         std::optional<const ortc::Tensor<int32_t>*> context_lens,
                         std::optional<const ortc::Tensor<int64_t>*> positions, int32_t num_heads, int32_t head_size, InputMetadata& input_metadata) {
  const std::vector<int64_t>& query_shape = query.Shape();
  if (query_shape.size() < 2 || query_shape.size() > 3) {
    return OrtW::CreateStatus(MakeString("Invalid query shape, expect 2 or 3 dimensions"), ORT_INVALID_ARGUMENT);
  }
  if (query_shape.back() != num_heads * head_size) {
    return OrtW::CreateStatus(MakeString("query shape should equal to num_heads_ * head_size_"), ORT_INVALID_ARGUMENT);
  }

  // TODO(leca): Cpu input or CUDA input?
  int seq_len = query_shape.size() == 3 ? query_shape[1] : query_shape[0];
  if (positions.has_value()) {
    std::vector<int64_t> positions_host((*positions)->Shape().size());
    //ORTX_RETURN_IF_ERROR(CudaCall(cudaMemcpy(positions_host.data(), (*positions)->DataRaw(), (*positions)->SizeInBytes(), cudaMemcpyDeviceToHost)));
    cudaMemcpy(positions_host.data(), (*positions)->DataRaw(), (*positions)->SizeInBytes(), cudaMemcpyDeviceToHost);
    while (positions_host.back() == 0) {
      positions_host.pop_back();
      seq_len--;
    }

    input_metadata.max_num_blocks_per_seq = 0;
    // in prompt mode
    if (positions_host.size() > 1 || positions_host.back() == 0) {
      input_metadata.num_prompt_tokens = seq_len;
      input_metadata.num_generation_tokens = 0;
    } else {
      input_metadata.num_prompt_tokens = 0;
      input_metadata.num_generation_tokens = seq_len;
      input_metadata.max_context_len = positions_host.back() + 1; // TODO(leca): what if position_host is empty?

      int32_t block_size = gsl::narrow<int32_t>(key_cache.Shape()[3]);
      for (int i = 0; i < positions_host.back() + 1; i += block_size) input_metadata.max_num_blocks_per_seq++;
    }
  } else {
    // TODO(leca): context_lens is nullptr?
    std::vector<int32_t> context_len_host((*context_lens)->SizeInBytes());
    //ORTX_RETURN_IF_ERROR(CudaCall(cudaMemcpy(context_len_host.data(), (*context_lens)->DataRaw(), (*context_lens)->SizeInBytes(), cudaMemcpyDeviceToHost)));
    cudaMemcpy(context_len_host.data(), (*context_lens)->DataRaw(), (*context_lens)->SizeInBytes(), cudaMemcpyDeviceToHost);
    std::vector<int64_t> position_ids;
    for (size_t i = 0; i < context_len_host.size(); i++) {
      if (context_len_host[i] == 0)   continue;
      std::vector<int64_t> position_id(context_len_host[i]);
      std::iota(position_id.begin(), position_id.end(), 0);   // fill position_id with {0, 1, 2, ...context_len_span[i]-1}
      position_ids.insert(position_ids.end(), position_id.begin(), position_id.end());
    }
    input_metadata.position_ids = GetScratchBuffer<int64_t>(allocator->Alloc(allocator, position_ids.size()), allocator);
    //ORTX_RETURN_IF_ERROR(CudaCall(cudaMemcpyAsync(input_metadata.position_ids.get(), position_ids.data(), position_ids.size(), cudaMemcpyHostToDevice, stream)));
    cudaMemcpyAsync(input_metadata.position_ids.get(), position_ids.data(), position_ids.size(), cudaMemcpyHostToDevice, stream);
  }
    input_metadata.num_valid_tokens = seq_len;
  
  return nullptr;
}

template<typename T>
struct PagedAttention {
  using TT = typename contrib::CudaT<T>::MappedType;
  OrtStatusPtr OnModelAttach(const OrtApi& api, const OrtKernelInfo& info) {
    int64_t num_heads = 0, head_size = 0;
    ORTX_RETURN_IF_ERROR(api.KernelInfoGetAttribute_int64(&info, "num_heads", &num_heads));
    assert(num_heads > 0);
    num_heads_ = static_cast<int32_t>(num_heads);
    num_kv_heads_ = static_cast<int32_t>(OrtW::GetOpAttributeOrDefault<int64_t>(info, "num_kv_heads", num_heads));
    
    ORTX_RETURN_IF_ERROR(api.KernelInfoGetAttribute_int64(&info, "head_size", &head_size));
    assert(head_size > 0);
    head_size_ = static_cast<int32_t>(head_size);

    ORTX_RETURN_IF_ERROR(api.KernelInfoGetAttribute_float(&info, "scale", &scale_));
    assert(scale_ > 0);

    num_queries_per_kv_ = num_heads_ / num_kv_heads_;
    std::vector<int32_t> head_mapping_host(num_heads_);
    for (int i = 0; i < num_kv_heads_; i++) {
      for (int j = 0; j < num_queries_per_kv_; j++) {
        head_mapping_host[i * num_queries_per_kv_ + j] = i;
      }
    }

    OrtAllocator* allocator = nullptr;
    ORTX_RETURN_IF_ERROR(api.KernelInfoGetAllocator(&info, OrtMemType::OrtMemTypeDefault, &allocator));
    allocator_ = UniquePtrWithDeletor<OrtAllocator>{allocator, [&api](OrtAllocator* p){api.ReleaseAllocator(p);}};
    head_mapping_ = GetScratchBuffer<int32_t>(allocator_->Alloc(allocator_.get(), num_heads_), allocator_.get());
    cudaMemcpy(head_mapping_.get(), head_mapping_host.data(), head_mapping_host.size(), cudaMemcpyHostToDevice);
    return nullptr;
  }

  OrtStatusPtr RunMultiHeadAttention(Ort::Custom::CUDAKernelContext* ctx, PackedAttentionParameters& parameters) const {
    PackedMultiHeadAttentionData<TT> data;
    
    return cuda::QkvToContext<TT>(reinterpret_cast<cudaStream_t>(ctx->GetCudaStream()), parameters, data);
//    return nullptr;
  }

  OrtStatusPtr Compute(Ort::Custom::CUDAKernelContext* ctx, const ortc::Tensor<T>& query, const ortc::Tensor<T>& key,
                       const ortc::Tensor<T>& value, const ortc::Tensor<T>& key_cache, const ortc::Tensor<T>& value_cache,
                       const ortc::Tensor<int32_t>& block_tables, const ortc::Tensor<int32_t>& slot_mappings, 
                       std::optional<const ortc::Tensor<int32_t>*> context_lens,
                       std::optional<const ortc::Tensor<int64_t>*> positions,
                       std::optional<const ortc::Tensor<T>*> cos_sin_cache, ortc::Tensor<T>& attn_out) const {
    InputMetadata input_metadata;
    PackedAttentionParameters parameters;
    ORTX_RETURN_IF_ERROR(CheckInputs<T>(reinterpret_cast<cudaStream_t>(ctx->GetCudaStream()), allocator_.get(), query, key, value, 
                         key_cache, value_cache, block_tables, slot_mappings, context_lens, positions, num_heads_, head_size_, input_metadata));
    const std::vector<int64_t>& query_shape = query.Shape();
    T* output_data = attn_out.Allocate(query_shape);

    if (cos_sin_cache.has_value()) {
      int64_t rot_dim = (*cos_sin_cache)->Shape()[1];
      assert(rot_dim == head_size_);
      cuda::rotary_embedding_neox(reinterpret_cast<cudaStream_t>(ctx->GetCudaStream()), (*positions)->Data(), const_cast<void*>(query.DataRaw()), const_cast<void*>(key.DataRaw()), head_size_,
                            (*cos_sin_cache)->DataRaw(), input_metadata.num_valid_tokens, rot_dim, num_heads_, num_kv_heads_, 1);
    }

    const std::vector<int64_t>& key_cache_shape = key_cache.Shape();
    if (input_metadata.num_valid_tokens > 0 && key_cache_shape.size() > 3) {
      int64_t key_shape_r[3] = {input_metadata.num_valid_tokens, num_kv_heads_, head_size_};
      int64_t value_shape_r[3] = {input_metadata.num_valid_tokens, num_kv_heads_, head_size_};
      int block_size = gsl::narrow<int>(key_cache_shape[3]);
      cuda::reshape_and_cache(reinterpret_cast<cudaStream_t>(ctx->GetCudaStream()), key.DataRaw(), value.DataRaw(), key_cache.DataRaw(), value_cache.DataRaw(), slot_mappings.Data(),
                        key_shape_r, value_shape_r, block_size, key_cache_shape[4], 1);
    }

    if (input_metadata.num_prompt_tokens > 0) {
      return RunMultiHeadAttention(ctx, parameters); // Don't handle prompt with decoding case for now
    }

    if (input_metadata.num_generation_tokens > 0) {
      constexpr int PARTITION_SIZE = 512;
      int max_num_partitions = (input_metadata.max_context_len + PARTITION_SIZE - 1) / PARTITION_SIZE;
      bool use_v1 = max_num_partitions == 1 || (query_shape[0] * query_shape[1]) > PARTITION_SIZE;
      int64_t generation_qeury_shape[3] = {input_metadata.num_valid_tokens, num_heads_, head_size_};
      if (use_v1) {
        cuda::paged_attention_v1(reinterpret_cast<cudaStream_t>(ctx->GetCudaStream()), reinterpret_cast<TT*>(output_data), query.DataRaw(),
                           key_cache.DataRaw(), value_cache.DataRaw(), head_mapping_.get(), scale_, 
                           block_tables.Data(), context_lens.has_value() ? (*context_lens)->Data() : nullptr,
                           value_cache.Shape()[3], input_metadata.max_context_len, nullptr,
                           input_metadata.max_num_blocks_per_seq, generation_qeury_shape, num_queries_per_kv_, 1);
      } else {
        OrtMemoryInfo* mem_info = nullptr;
        ORTX_RETURN_IF_ERROR(OrtW::API::CreateOrtMemoryInfo("Cuda", OrtDeviceAllocator, ctx->GetCudaDeviceId(), OrtMemTypeDefault, &mem_info));
        void* tmp_output_raw = ctx->GetScratchBufferUnderMultiStream(mem_info, query_shape.size() * max_num_partitions * sizeof(T));
        UniquePtrWithDeletor<T> tmp_output = GetScratchBuffer<T>(tmp_output_raw, allocator_.get());   // TODO(leca): should deallocate inside ORT
        void* exp_sums_raw = ctx->GetScratchBufferUnderMultiStream(mem_info, query_shape[0] * query_shape[1] * num_heads_ * max_num_partitions * sizeof(T));
        UniquePtrWithDeletor<T> exp_sums = GetScratchBuffer<T>(exp_sums_raw, allocator_.get());
        void* max_logits_raw = ctx->GetScratchBufferUnderMultiStream(mem_info, query_shape[0] * query_shape[1] * num_heads_ * max_num_partitions * sizeof(T));
        UniquePtrWithDeletor<T> max_logits = GetScratchBuffer<T>(max_logits_raw, allocator_.get());
        cuda::paged_attention_v2(reinterpret_cast<cudaStream_t>(ctx->GetCudaStream()), exp_sums_raw, max_logits_raw, tmp_output_raw, reinterpret_cast<TT*>(output_data), query.DataRaw(),
                           key_cache.DataRaw(), value_cache.DataRaw(), head_mapping_.get(), scale_,
                           block_tables.Data(), context_lens.has_value() ? (*context_lens)->Data() : nullptr,
                           value_cache.Shape()[3], input_metadata.max_context_len, nullptr,
                           input_metadata.max_num_blocks_per_seq, generation_qeury_shape, num_queries_per_kv_, 1);

        OrtW::API::ReleaseMemoryInfo(mem_info);
      }
    }
    return nullptr;
  }

private:
  int32_t num_heads_;                  // number of attention heads
  int32_t num_kv_heads_;                  // number of attention kv_heads
  int32_t head_size_;                      // number of attention heads
  float scale_;                            // sqrt(head_size_)
  UniquePtrWithDeletor<int32_t> head_mapping_;
  int32_t num_queries_per_kv_;
  UniquePtrWithDeletor<OrtAllocator> allocator_;
};