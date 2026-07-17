# 算法模块层 -> 共享库 libalgo (.so / .dll)，依赖 base
# 每个 .c 是一个独立的自研算法能力，客户项目(flow)按需挑用。
add_library(algo SHARED
    detect.c    # 表面质量评分
    angle.c     # 角度测量
)

target_include_directories(algo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 单向依赖：algo -> base。base 不许反向引用 algo。
target_link_libraries(algo PUBLIC base)
