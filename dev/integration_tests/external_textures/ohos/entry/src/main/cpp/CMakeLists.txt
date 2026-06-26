cmake_minimum_required(VERSION 3.4.1)
project(nativerender)

add_library(nativerender SHARED nativerender.cpp)

target_include_directories(nativerender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(nativerender PUBLIC ace_napi.z native_window hilog_ndk.z)
