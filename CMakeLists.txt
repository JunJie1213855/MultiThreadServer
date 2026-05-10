cmake_minimum_required(VERSION 3.20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(ThreadPoolServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GCC 需要 -fcoroutines 标志来支持协程
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()


option(BUILD_WITH_URING "build with io_uring ?" ON)
option(BUILD_OPTIM "enable optimization (O3, SIMD auto-vectorization)" ON)

if(BUILD_WITH_URING)
    # 启用 io_uring
    add_definitions(-DBOOST_ASIO_HAS_IO_URING)
    message("build with io_uring")
endif()

if(BUILD_OPTIM)
    # 高级优化
    add_compile_options(-O3 -march=native -ffast-math -ftree-vectorize)
    message("enable optimization")
endif()


# 查找 Boost 库
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(jsoncpp REQUIRED)

# 查找 jemalloc（通过 find_library）
find_library(JEMALLOC_LIBRARY NAMES jemalloc)
if(JEMALLOC_LIBRARY)
    message(STATUS "jemalloc found: ${JEMALLOC_LIBRARY}")
else()
    message(STATUS "jemalloc not found, using system malloc")
endif()


include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# 源文件
set(SOURCES
    src/AsioThreadPool.cpp
    src/CSession.cpp
    src/CServer.cpp
    src/LogicSystem.cpp
    src/MsgNode.cpp
)

add_executable(ThreadPoolServer main_server.cpp ${SOURCES})

target_link_libraries(ThreadPoolServer
    Boost::system
    Boost::thread
    pthread
    JsonCpp::JsonCpp
)
if(JEMALLOC_LIBRARY)
    target_link_libraries(ThreadPoolServer ${JEMALLOC_LIBRARY})
endif()


add_executable(main_jsonclient main_jsonclient.cpp)
target_link_libraries(main_jsonclient
    Boost::system
    Boost::thread
    pthread
    JsonCpp::JsonCpp
)