cmake_minimum_required(VERSION 3.5)
project(module_library)

add_subdirectory(sub1)
add_subdirectory(sub2)

add_executable(module main.cpp)

message(${PROJECT_SOURCE_DIR})

target_include_directories(module
 PUBLIC 
    ${PROJECT_SOURCE_DIR}/sub1
    ${PROJECT_SOURCE_DIR}/sub2
)

target_link_libraries(module
    PRIVATE
        hello::library
        welcome_library
)