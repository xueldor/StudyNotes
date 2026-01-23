cmake_minimum_required(VERSION 3.5)
project(hello_library)

#创建共享库
add_library(hello_library SHARED
     src/Hello.cpp
)
#给hello_library起个别名
add_library(hello::library ALIAS hello_library )

target_include_directories(hello_library
 PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(hello_binary src/main.cpp)

target_link_libraries(hello_binary
    PRIVATE
        hello::library
)