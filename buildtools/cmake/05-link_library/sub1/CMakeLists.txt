cmake_minimum_required(VERSION 3.5)
project(hello_library)

#创建共享库
add_library(hello_library SHARED
     hello.cpp
)
#给hello_library起个别名
add_library(hello::library ALIAS hello_library )
