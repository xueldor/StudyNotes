cmake_minimum_required(VERSION 3.5)
project(hello_library)

add_library(welcome_library STATIC
     welcome.cpp
)