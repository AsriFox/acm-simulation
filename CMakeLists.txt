cmake_minimum_required (VERSION 3.14)
cmake_policy (SET CMP0054 NEW)
cmake_policy (SET CMP0135 NEW)
project (acm-simulation)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch AFF3CT library
include (FetchContent)
option (ACM_AFF3CT_FROM_SOURCE "Build AFF3CT library from source, otherwise download prebuilt binaries" OFF)
option (ACM_AFF3CT_AVX2 "Use AFF3CT library compiled for CPUs supporting AVX2, otherwise use SSE4.2" OFF)
if (ACM_AFF3CT_FROM_SOURCE)
    set (AFF3CT_COMPILE_EXE OFF)
    set (AFF3CT_COMPILE_STATIC_LIB ON)
    set (AFF3CT_COMPILE_SHARED_LIB ON)
    set (AFF3CT_LIB "aff3ct-3.0.2")
    FetchContent_Declare (${AFF3CT_LIB}
        GIT_REPOSITORY https://github.com/aff3ct/aff3ct.git
        GIT_TAG 8fa65a3ca9b0dcdd3d544363bc692d4f85f6f718 # Release 3.0.2
    )
else ()
    if (ACM_AFF3CT_AVX2)
        set (ACM_AFF3CT_CPU "avx2")
    else ()
        set (ACM_AFF3CT_CPU "sse4_2")
    endif()
    set (AFF3CT_LIB "aff3ct-3.0.2-9-gbb649c4")
    FetchContent_Declare (${AFF3CT_LIB}
        URL https://gitlab.com/aff3ct/aff3ct/-/jobs/3026591802/artifacts/raw/builds/aff3ct_development_linux_gcc_x64_${ACM_AFF3CT_CPU}_bb649c4b1.zip
    )
endif()
FetchContent_MakeAvailable (${AFF3CT_LIB})

# Compile the main executable
add_executable (acm-simulation 
    main.cpp
    # modules.cpp
    # modules.hpp
)

# Threads library
set (CMAKE_THREAD_PREFER_PTHREAD ON)
set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

# Include and link AFF3CT library
set (AFF3CT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/${AFF3CT_LIB}-src/lib/cmake/${AFF3CT_LIB}")
find_package (AFF3CT CONFIG 3.0.2 REQUIRED)
target_link_libraries (acm-simulation PRIVATE aff3ct::aff3ct-static-lib)

# OpenMP library
find_package (OpenMP)
if (OpenMP_FOUND)
    target_link_libraries (acm-simulation PRIVATE OpenMP::OpenMP_CXX)
endif()
