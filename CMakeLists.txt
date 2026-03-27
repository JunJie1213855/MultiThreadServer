cmake_minimum_required(VERSION 3.10)
project(ThreadPoolServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GCC 需要 -fcoroutines 标志来支持协程
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()


# 查找 Boost 库
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(jsoncpp REQUIRED)

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


add_executable(main_jsonclient main_jsonclient.cpp)
target_link_libraries(main_jsonclient
    Boost::system
    Boost::thread
    pthread
    JsonCpp::JsonCpp
)