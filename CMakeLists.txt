# cmake的最低版本号
cmake_minimum_required(VERSION 3.0)

# 项目名称
project(lazy_base)

# 设置支持C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置包含目录
set(ROOT_DIR "./")
set(TEST_DIR "./test/")
include_directories(${ROOT_DIR})
include_directories(${TEST_DIR})

file(GLOB SOURCES "${ROOT_DIR}/*.cc" "${ROOT_DIR}/*.cpp" "${TEST_DIR}/*.cc" "${TEST_DIR}/*.cpp")
file(GLOB HEADERS "${ROOT_DIR}/*.h" "${TEST_DIR}/*.h")

message(STATUS "-- print all sources -- begin")
message(STATUS ${SOURCES})
#message(STATUS ${HEADERS})
message(STATUS "-- print all sources -- end")
# 可执行文件名
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})