# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(unittest_ohos)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

# NAPI 共享库（被 ArkTS 调用）
add_library(entry SHARED napi_init.cpp)
target_link_libraries(entry PUBLIC libace_napi.z.so)
target_link_libraries(entry PUBLIC dl)
target_link_libraries(entry PUBLIC libhilog_ndk.z.so)