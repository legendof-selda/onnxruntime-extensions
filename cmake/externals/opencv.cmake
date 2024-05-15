set(BUILD_ANDROID_EXAMPLES      OFF CACHE INTERNAL "")
set(BUILD_ANDROID_PROJECTS      OFF CACHE INTERNAL "")
set(BUILD_ANDROID_SERVICE       OFF CACHE INTERNAL "")
set(BUILD_DOCS                  OFF CACHE INTERNAL "")
set(BUILD_FAT_JAVA_LIB          OFF CACHE INTERNAL "")
set(BUILD_IPP_IW                OFF CACHE INTERNAL "")
set(BUILD_ITT                   OFF CACHE INTERNAL "")
set(BUILD_JASPER                OFF CACHE INTERNAL "")
set(BUILD_JAVA                  OFF CACHE INTERNAL "")
set(BUILD_JPEG                  OFF CACHE INTERNAL "")
set(BUILD_OBJC                  OFF CACHE INTERNAL "")
set(BUILD_OPENJPEG              OFF CACHE INTERNAL "")
set(BUILD_PNG                   OFF CACHE INTERNAL "")
set(BUILD_opencv_apps           OFF CACHE INTERNAL "")
set(BUILD_opencv_calib3d        OFF CACHE INTERNAL "")
set(BUILD_opencv_dnn            OFF CACHE INTERNAL "")
set(BUILD_opencv_features2d     OFF CACHE INTERNAL "")
set(BUILD_opencv_flann          OFF CACHE INTERNAL "")
set(BUILD_opencv_gapi           OFF CACHE INTERNAL "")
set(BUILD_opencv_highgui        OFF CACHE INTERNAL "")
set(BUILD_opencv_imgcodecs      OFF CACHE INTERNAL "")
set(BUILD_opencv_java           OFF CACHE INTERNAL "")
set(BUILD_opencv_js             OFF CACHE INTERNAL "")
set(BUILD_opencv_ml             OFF CACHE INTERNAL "")
set(BUILD_opencv_objc           OFF CACHE INTERNAL "")
set(BUILD_opencv_objdetect      OFF CACHE INTERNAL "")
set(BUILD_opencv_photo          OFF CACHE INTERNAL "")
set(BUILD_opencv_python2        OFF CACHE INTERNAL "")
set(BUILD_opencv_python3        OFF CACHE INTERNAL "")
set(BUILD_opencv_stitching      OFF CACHE INTERNAL "")
set(BUILD_opencv_ts             OFF CACHE INTERNAL "")
set(BUILD_opencv_video          OFF CACHE INTERNAL "")
set(BUILD_opencv_videoio        OFF CACHE INTERNAL "")
set(BUILD_opencv_world          OFF CACHE INTERNAL "")
set(BUILD_OPENEXR               OFF CACHE INTERNAL "")
set(BUILD_TBB                   OFF CACHE INTERNAL "")
set(BUILD_TIFF                  OFF CACHE INTERNAL "")
set(BUILD_WEBP                  OFF CACHE INTERNAL "")
set(BUILD_WITH_STATIC_CRT       OFF CACHE INTERNAL "")
if(OCOS_BUILD_APPLE_FRAMEWORK)
  # tell OpenCV to build zlib so we can link to the static library
  set(BUILD_ZLIB                ON  CACHE INTERNAL "")
else()
  set(BUILD_ZLIB                OFF CACHE INTERNAL "")
endif()
set(ENABLE_FAST_MATH            OFF CACHE INTERNAL "")
set(ENABLE_PRECOMPILED_HEADERS  OFF CACHE INTERNAL "")
set(WITH_ANDROID_MEDIANDK       OFF CACHE INTERNAL "")
set(WITH_AVFOUNDATION           OFF CACHE INTERNAL "")
set(WITH_CAP_IOS                OFF CACHE INTERNAL "")
set(WITH_CAROTENE               OFF CACHE INTERNAL "")
set(WITH_CLP                    OFF CACHE INTERNAL "")
set(WITH_CPUFEATURES            OFF CACHE INTERNAL "")
set(WITH_DIRECTX                OFF CACHE INTERNAL "")
set(WITH_DSHOW                  OFF CACHE INTERNAL "")
set(WITH_EIGEN                  OFF CACHE INTERNAL "")
set(WITH_FFMPEG                 OFF CACHE INTERNAL "")
set(WITH_GDCM                   OFF CACHE INTERNAL "")
set(WITH_GSTREAMER              OFF CACHE INTERNAL "")
set(WITH_GTK                    OFF CACHE INTERNAL "")
set(WITH_HALIDE                 OFF CACHE INTERNAL "")
set(WITH_HPX                    OFF CACHE INTERNAL "")
set(WITH_IMGCODEC_HDR           OFF CACHE INTERNAL "")
set(WITH_IMGCODEC_PFM           OFF CACHE INTERNAL "")
set(WITH_IMGCODEC_PXM           OFF CACHE INTERNAL "")
set(WITH_IMGCODEC_SUNRASTER     OFF CACHE INTERNAL "")
set(WITH_INF_ENGINE             OFF CACHE INTERNAL "")
set(WITH_IPP                    OFF CACHE INTERNAL "")
set(WITH_ITT                    OFF CACHE INTERNAL "")
set(WITH_JASPER                 OFF CACHE INTERNAL "")
set(WITH_JPEG                   OFF CACHE INTERNAL "")
set(WITH_MSMF                   OFF CACHE INTERNAL "")
set(WITH_NGRAPH                 OFF CACHE INTERNAL "")
set(WITH_ONNX                   OFF CACHE INTERNAL "")
set(WITH_OPENCL                 OFF CACHE INTERNAL "")
set(WITH_OPENCL_SVM             OFF CACHE INTERNAL "")
set(WITH_OPENEXR                OFF CACHE INTERNAL "")
set(WITH_OPENJPEG               OFF CACHE INTERNAL "")
set(WITH_OPENMP                 OFF CACHE INTERNAL "")
set(WITH_OPENVX                 OFF CACHE INTERNAL "")
set(WITH_PNG                    OFF CACHE INTERNAL "")
set(WITH_PROTOBUF               OFF CACHE INTERNAL "")
set(WITH_PTHREADS_PF            OFF CACHE INTERNAL "")
set(WITH_QUIRC                  OFF CACHE INTERNAL "")
set(WITH_TBB                    OFF CACHE INTERNAL "")
set(WITH_TENGINE                OFF CACHE INTERNAL "")
set(WITH_TIFF                   OFF CACHE INTERNAL "")
set(WITH_V4L                    OFF CACHE INTERNAL "")
set(WITH_VULKAN                 OFF CACHE INTERNAL "")
set(WITH_WEBP                   OFF CACHE INTERNAL "")
set(WITH_WIN32UI                OFF CACHE INTERNAL "")

if (OCOS_ENABLE_OPENCV_CODECS)
  set(BUILD_opencv_imgcodecs  ON CACHE INTERNAL "")

  set(BUILD_JPEG              ON CACHE INTERNAL "")
  set(BUILD_PNG               ON CACHE INTERNAL "")

  set(WITH_JPEG               ON CACHE INTERNAL "")
  set(WITH_PNG                ON CACHE INTERNAL "")
endif()

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_DOCS        OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES    OFF CACHE INTERNAL "")
set(BUILD_TESTS       OFF CACHE INTERNAL "")
set(CV_TRACE          OFF CACHE INTERNAL "")

set(CV_DISABLE_OPTIMIZATION     ON CACHE INTERNAL "")

if(IOS)
  # copy what OpenCV's platforms/ios/build_framework.py does and set CPU_BASELINE=DETECT
  # https://github.com/opencv/opencv/blob/4223495e6cd67011f86b8ecd9be1fa105018f3b1/platforms/ios/build_framework.py#L253
  set(CPU_BASELINE DETECT)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # error   _png_do_read_transformations in liblibpng.a(pngrtran.c.o) not found for architecture arm64
  # workaround to disable NEON optimizations
  add_definitions(-DPNG_ARM_NEON_IMPLEMENTATION=0)
  add_definitions(-DPNG_ARM_NEON_OPT=0)
endif()

if (MSVC AND CMAKE_GENERATOR_PLATFORM)
  string(TOLOWER ${CMAKE_GENERATOR_PLATFORM} _GEN_PLATFORM)
  if (${_GEN_PLATFORM} MATCHES "arm|arm64")
    set(OPENCV_SKIP_SYSTEM_PROCESSOR_DETECTION ON)
  endif()
endif()

FetchContent_Declare(
    opencv
    GIT_REPOSITORY https://github.com/opencv/opencv.git
    SOURCE_DIR opencv_source
    GIT_TAG        4.5.4
    GIT_SHALLOW    TRUE
    -DBUILD_DOCS:BOOL=FALSE
    -DBUILD_EXAMPLES:BOOL=FALSE
    -DBUILD_TESTS:BOOL=FALSE
    -DBUILD_SHARED_LIBS:BOOL=FALSE
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/opencv
    PATCH_COMMAND git checkout . && git apply --whitespace=fix --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/cmake/externals/opencv-no-rtti.patch
)

string(TOLOWER "opencv" lcName)
if(NOT ${lcName}_POPULATED)
  FetchContent_Populate(opencv)

  # Set variablles to paths of libspng and libjpeg (we only these 2 folders for our vision operators in Extensions)
  set(libspng ${opencv_source}/3rdparty/libspng/)
  set(libjpeg ${opencv_source}/3rdparty/libjpeg/)

  # Get a list of all files in the folders
  file(GLOB_RECURSE LIBSPNG_FILES "${libspng}/*")
  file(GLOB_RECURSE LIBJPEG_FILES "${libjpeg}/*")
endif()

set(opencv_INCLUDE_DIRS "")
list(APPEND opencv_INCLUDE_DIRS ${OPENCV_CONFIG_FILE_INCLUDE_DIR})
list(APPEND opencv_INCLUDE_DIRS
    ${OPENCV_MODULE_opencv_core_LOCATION}/include
    ${OPENCV_MODULE_opencv_imgproc_LOCATION}/include)
set(opencv_LIBS "")
list(APPEND opencv_LIBS opencv_core opencv_imgproc)

if (OCOS_ENABLE_OPENCV_CODECS)
    list(APPEND opencv_INCLUDE_DIRS ${OPENCV_MODULE_opencv_imgcodecs_LOCATION}/include)
    list(APPEND opencv_LIBS opencv_imgcodecs)

    # Add libspng and libjpeg files to our build target
    list(APPEND opencv_LIBS LIBSPNG_FILES)
    list(APPEND opencv_LIBS LIBJPEG_FILES)
endif()

# unset it to avoid affecting other projects.
unset(EXECUTABLE_OUTPUT_PATH CACHE)