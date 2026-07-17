# 算法模块层 -> 共享库 libalgo (.so / .dll)，依赖 base
add_library(algo SHARED
    detect.c
)

target_include_directories(algo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 单向依赖：algo -> base。base 不许反向引用 algo。
target_link_libraries(algo PUBLIC base)
