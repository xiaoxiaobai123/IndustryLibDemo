# 基础库层 -> 共享库 libbase (.so / .dll)
add_library(base SHARED
    log.c
    net.c
    plc_sim.c
    imgproc.c
)

target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(base PRIVATE ws2_32)
else()
    target_link_libraries(base PRIVATE m)
endif()
