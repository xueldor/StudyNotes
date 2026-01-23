cmake_minimum_required(VERSION 3.5)
project(test_condition CXX)


# 如果是Linux环境，编译Linux.cpp
set(IS_LINUX false) #change to true or false
if(IS_LINUX)
    message("run on linux")
    add_definitions(-DI_AM_LINUX)
endif()

add_executable(test_condition main.cpp  )
if(IS_LINUX)
    message("target_sources linux.cpp")
    target_sources(test_condition  PRIVATE linux.cpp)
endif()