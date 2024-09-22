cmake_minimum_required(VERSION 3.0)

project(WebDisk_OSS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -fno-rtti")

# 添加包含路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 设置可执行文件生成到哪里
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# add_executable(main_exe main.cpp src/Gun.cpp src/Soldier.cpp)
file(GLOB SOURCES "src/*.cpp")
add_executable(WebDisk ${SOURCES})

# 链接 pthread 库
target_link_libraries(WebDisk
    pthread
    workflow
    wfrest
    ssl
    crypt
    crypto
    alibabacloud-oss-cpp-sdk
    curl
    SimpleAmqpClient
    srpc
    protobuf
    ppconsul
)