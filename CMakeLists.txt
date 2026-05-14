cmake_minimum_required(VERSION 3.20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(mts VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GCC 需要 -fcoroutines 标志来支持协程
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()

option(BUILD_WITH_URING "build with io_uring ?" ON)
option(BUILD_OPTIM "enable optimization (O3, SIMD auto-vectorization)" ON)

# 优化标志：以 PRIVATE 方式附加到具体 target，
# 避免 -march=native 泄漏进库的导出接口。
set(MTS_OPTIM_FLAGS "")
if(BUILD_OPTIM)
    set(MTS_OPTIM_FLAGS -O3 -march=native -ffast-math -ftree-vectorize)
    message(STATUS "enable optimization")
endif()

if(BUILD_WITH_URING)
    message(STATUS "build with io_uring")
endif()

# 查找 Boost 库
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)

# RapidJSON 是 header-only，只需找头文件路径。
# TODO Phase 3: 移除 —— 库核心不应依赖 RapidJSON（目前仅 LogicSystem.cpp 临时使用），
# 届时 RapidJSON 的查找只保留在 examples/ 中。
find_path(RAPIDJSON_INCLUDE_DIR rapidjson/document.h REQUIRED)
message(STATUS "RapidJSON include dir: ${RAPIDJSON_INCLUDE_DIR}")

# 库源文件（Phase 1 仍沿用旧文件名）
set(MTS_SOURCES
    src/AsioThreadPool.cpp
    src/CSession.cpp
    src/CServer.cpp
    src/LogicSystem.cpp
    src/MsgNode.cpp
)

# ── 静态库 + 动态库，二者均输出为 libmts ─────────────────────────────
add_library(mts-static STATIC ${MTS_SOURCES})
add_library(mts-shared SHARED ${MTS_SOURCES})
add_library(mts::mts ALIAS mts-shared)

set_target_properties(mts-static mts-shared PROPERTIES OUTPUT_NAME mts)
set_target_properties(mts-shared PROPERTIES POSITION_INDEPENDENT_CODE ON)

foreach(tgt mts-static mts-shared)
    target_include_directories(${tgt} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    # TODO Phase 3: 移除 —— RapidJSON 仅供 LogicSystem.cpp 临时使用，
    # 以 PRIVATE 方式限定在库目标内，不进入导出接口。
    target_include_directories(${tgt} PRIVATE ${RAPIDJSON_INCLUDE_DIR})
    target_link_libraries(${tgt} PUBLIC Boost::system Boost::thread Threads::Threads)
    target_compile_options(${tgt} PRIVATE ${MTS_OPTIM_FLAGS})
    if(BUILD_WITH_URING)
        target_compile_definitions(${tgt} PUBLIC BOOST_ASIO_HAS_IO_URING)
    endif()
endforeach()

# 示例程序（库的消费者）
add_subdirectory(examples)
