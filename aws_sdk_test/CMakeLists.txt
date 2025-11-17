cmake_minimum_required(VERSION 3.13)
project(aws_sdk_test)

set(CMAKE_CXX_STANDARD 17)

find_package(AWSSDK REQUIRED COMPONENTS s3 core)

add_executable(aws_sdk_test main.cpp)

target_link_libraries(aws_sdk_test ${AWSSDK_LINK_LIBRARIES})
