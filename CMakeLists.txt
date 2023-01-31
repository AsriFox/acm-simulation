cmake_minimum_required(VERSION 3.14)
project(acm-simulation)

include(FetchContent)
set(AFF3CT_LIB "aff3ct-3.0.2-9-gbb649c4")
FetchContent_Declare(
    ${AFF3CT_LIB}
    URL https://gitlab.com/aff3ct/aff3ct/-/jobs/3026591802/artifacts/raw/builds/aff3ct_development_linux_gcc_x64_sse4_2_bb649c4b1.zip
)
FetchContent_MakeAvailable(${AFF3CT_LIB})

add_executable(acm-simulation main.cpp)

set(AFF3CT_INCLUDE_LIB "${CMAKE_SOURCE_DIR}/build/_deps/${AFF3CT_LIB}-src/include/${AFF3CT_LIB}")
target_include_directories(acm-simulation PRIVATE ${AFF3CT_INCLUDE_LIB}/aff3ct)
target_include_directories(acm-simulation PRIVATE ${AFF3CT_INCLUDE_LIB}/cli)
target_include_directories(acm-simulation PRIVATE ${AFF3CT_INCLUDE_LIB}/date)
target_include_directories(acm-simulation PRIVATE ${AFF3CT_INCLUDE_LIB}/MIPP)
target_include_directories(acm-simulation PRIVATE ${AFF3CT_INCLUDE_LIB}/rang)
target_link_directories(acm-simulation PRIVATE ${CMAKE_SOURCE_DIR}/build/_deps/${AFF3CT_LIB}-src/lib)
target_link_libraries(acm-simulation PRIVATE ${AFF3CT_LIB})