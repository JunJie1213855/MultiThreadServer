cmake_minimum_required(VERSION 3.10)
project(ThreadPoolServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost 库
find_package(Boost REQUIRED COMPONENTS system thread)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/include/jsoncpp)

# 源文件
set(SOURCES
    src/main.cpp
    src/AsioThreadPool.cpp
    src/CSession.cpp
    src/CServer.cpp
    src/LogicSystem.cpp
    src/MsgNode.cpp
)

add_executable(ThreadPoolServer ${SOURCES})

target_link_libraries(ThreadPoolServer
    Boost::system
    Boost::thread
    pthread
    jsoncpp
)