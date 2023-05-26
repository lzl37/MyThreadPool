cmake_minimum_required(VERSION 3.26)
project(MyThreadPool)

set(CMAKE_CXX_STANDARD 17)

add_executable(MyThreadPool main.cpp ThreadPool.h)
