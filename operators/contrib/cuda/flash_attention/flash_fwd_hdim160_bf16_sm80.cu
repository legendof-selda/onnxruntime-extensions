// Copyright (c) 2023, Tri Dao.

// Splitting the different head dimensions to different files to speed up compilation.
#if USE_FLASH_ATTENTION

#include "flash_fwd_launch_template.h"

namespace flash {

template<>
void run_mha_fwd_<cutlass::bfloat16_t, 160>(Flash_fwd_params& params, cudaStream_t stream) {
    run_mha_fwd_hdim160<cutlass::bfloat16_t>(params, stream);
}

}  // namespace flash
#endif
