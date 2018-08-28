cmake_minimum_required(VERSION 3.0.2)

project(Windows-UMS)

add_executable(ums-example ums.cpp)
add_executable(thread-example thread.cpp)
add_executable(fiber-example fiber.cpp)