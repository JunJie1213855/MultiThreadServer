# 消费者冒烟测试：证明 install 出来的 mts 包可被第三方 find_package + 链接。
# 独立项目，不属于 mts 主构建；CI 用 -DCMAKE_PREFIX_PATH=<安装前缀> 单独配置。
cmake_minimum_required(VERSION 3.20)
project(mts_consumer_smoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(mts REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE mts::mts)
