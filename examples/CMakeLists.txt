# 示例程序：mts 库的消费者
#
# Phase 1：入口文件仍位于仓库根目录（main_server.cpp / main_jsonclient.cpp），
# 此处通过绝对路径引用；Phase 4 会 git mv 到 examples/ 子目录。

# jemalloc（可选，仅示例使用 —— 已从库核心移出）
find_library(JEMALLOC_LIBRARY NAMES jemalloc)
if(JEMALLOC_LIBRARY)
    message(STATUS "jemalloc found: ${JEMALLOC_LIBRARY}")
else()
    message(STATUS "jemalloc not found, using system malloc")
endif()

# 示例二进制统一输出到构建树根目录，保持稳定的扁平路径（CI 依赖此路径）。
set(MTS_EXAMPLE_OUTPUT_DIR ${CMAKE_BINARY_DIR})

# ── echo_server：消费 mts 库 ──────────────────────────────────────
# Phase 1 业务逻辑（含 RapidJSON）仍在库内的 LogicSystem.cpp；
# Phase 3 会迁移到此处的 handlers.{h,cpp}。
add_executable(echo_server ${CMAKE_SOURCE_DIR}/main_server.cpp)
target_link_libraries(echo_server PRIVATE mts::mts)
target_include_directories(echo_server PRIVATE ${RAPIDJSON_INCLUDE_DIR})
target_compile_options(echo_server PRIVATE ${MTS_OPTIM_FLAGS})
set_target_properties(echo_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${MTS_EXAMPLE_OUTPUT_DIR})
if(JEMALLOC_LIBRARY)
    target_link_libraries(echo_server PRIVATE ${JEMALLOC_LIBRARY})
endif()

# ── load_client：独立的负载测试客户端（200 线程 × 500 次往返）──────
add_executable(load_client ${CMAKE_SOURCE_DIR}/main_jsonclient.cpp)
target_link_libraries(load_client PRIVATE Boost::system Boost::thread Threads::Threads)
target_include_directories(load_client PRIVATE ${RAPIDJSON_INCLUDE_DIR})
target_compile_options(load_client PRIVATE ${MTS_OPTIM_FLAGS})
set_target_properties(load_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${MTS_EXAMPLE_OUTPUT_DIR})
if(BUILD_WITH_URING)
    target_compile_definitions(load_client PRIVATE BOOST_ASIO_HAS_IO_URING)
endif()
