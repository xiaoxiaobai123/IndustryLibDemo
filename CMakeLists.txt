cmake_minimum_required(VERSION 3.15)
project(IndustryLibDemo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows 下自动导出所有符号，省掉 __declspec(dllexport) 宏
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# 客户流程开关：关掉的流程源码完全不参与编译
option(FLOW_A "Build customer A flow into the core" OFF)
option(FLOW_B "Build customer B flow into the core" OFF)

# 库和可执行文件都放进 build/bin：Windows 下 exe 能直接找到同目录 dll
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(NOT WIN32)
    set(CMAKE_BUILD_RPATH "$ORIGIN")   # Linux 下 exe 从自身目录找 .so
endif()

add_subdirectory(core/base)   # -> libbase.so / base.dll
add_subdirectory(core/algo)   # -> libalgo.so / algo.dll

# ---- 业务流程层：不做成库，条件编译后直接编进主程序 ----
set(FLOW_SOURCES core/flow/flow_common.c)
set(BUILD_TAG "common")

if(FLOW_A)
    list(APPEND FLOW_SOURCES core/flow/flow_customer_a.c)
    set(BUILD_TAG "${BUILD_TAG}+A")
endif()
if(FLOW_B)
    list(APPEND FLOW_SOURCES core/flow/flow_customer_b.c)
    set(BUILD_TAG "${BUILD_TAG}+B")
endif()

add_executable(core core/main.c ${FLOW_SOURCES})
target_include_directories(core PRIVATE core/flow)
target_link_libraries(core PRIVATE algo)   # 只连 algo；base 由 algo 传递而来

target_compile_definitions(core PRIVATE BUILD_TAG="${BUILD_TAG}")
if(FLOW_A)
    target_compile_definitions(core PRIVATE FLOW_A)
endif()
if(FLOW_B)
    target_compile_definitions(core PRIVATE FLOW_B)
endif()

message(STATUS "BUILD_TAG = ${BUILD_TAG}")
