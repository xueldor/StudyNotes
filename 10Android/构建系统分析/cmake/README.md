## 目标
* 用CMake编译可执行程序, 包含源文件，指定头文件路径
* 指定编译参数，如C_Flag
* 编静态库、动态库
* 引用三分库（静态库、动态库）
* 模块化，引用子模块
* 使用protobuf
* 条件变量、传递宏给代码里
* 交叉编译，指定编译器

## 00-hellocmake:  

将一个main.cpp编译成可执行程序。 
> cmake_minimum_required(VERSION 3.5)

指定cmake最小版本要求

> project (hello_cmake)

定义工程名称

> add_executable(hello_cmake main.cpp)

生成可执行文件,第一个参数是生成的目标名。后面是源文件列表。

执行： 先cd到out目录，再执行`cmake ..`,这样生成的文件都在out目录下面。也可以在外面`cmake . -Bout`同样效果。  
默认会生成makefile。再执行make即可生成目标。

在add_executable后面追加源文件：

```
if(TEST_THREAD)
    message("target_sources test_thread.c")
    target_sources(c99c11new  PRIVATE test_thread.c)
endif()
```

要执行CMakeLists.txt, 到所在目录，执行cmake命令即可。

或者指定路径。

指定使用ninja构建：`cmake -G Ninja`.

生成compile_commands.json：  `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1`。

`-D`选项可以定义一个变量并为其赋值

导出系统变量：`cmake --system-information info.txt`,`CMAKE_EXPORT_COMPILE_COMMANDS`、`CMAKE_CXX_COMPILER`等都在里面。




## 01-includeheader：
```cmake
set(SOURCES
    src/Hello.cpp
    src/main.cpp
)
```
用set显式的定义一个变量,SOURCES的值是“src/Hello.cpp  src/main.cpp”，这样add_executable就只需要写成：
```cmake
add_executable(hello_headers ${SOURCES})
```
target_include_directories  ： 指定目标的头文件目录,使用时，这个target必须前面已经添加，也就是要放在add_executable的后面。

include_directories和target_include_directories的区别是，
include_directories针对当前CMakeList.txt中的所有目标，以及所有在其调用点之后添加的子目录中的所有目标将具有此头文件搜索路径。  
target_include_directories只针对指定目标。

建议不要使用include_directories

* INTERFACE：target对应的头文件使用
* PRIVATE：target对应的源文件使用
* PUBLIC：target对应的头文件、源文件都使用

(这里我把CMakeLists.txt中的PUBLIC改成PRIVATE,观察生成的Makefile，对比内容并没有任何区别，不知道是不是因为这个demo过于简单，区别体现不出来）

## 02-static_library：
编译静态库：  
`add_library(hello_library STATIC src/Hello.cpp)`    

然后编译可执行文件时，为了能找到这个静态库，需要指定链接：
```
target_link_libraries(hello_binary
    PRIVATE
        hello_library
)
```
这里要注意的是，target_link_libraries是设置链接关系，不是创建目标，所以执行这一句的时候，hello_binary和hello_library必须已经存在。也就是说，target_link_libraries语句要放在add_library和add_executable的后面。


## 03-dynamic_library
编译共享库：  
`add_library(hello_library SHARED src/Hello.cpp)`    

除了static换成shared，其它和静态库一样。  
另外，我们注意到，编译的产物文件名自动加了lib前缀和.so以及.a后缀。

## 04-condition
1. cmake里面用message()函数打印日志；  
2. if-else- else-endif语句
3. C/C++ 里的条件编译`#ifdef`，在cmake里定义条件变量，传递给代码里。

示例一：add_definitions

格式为：` add_definitions(-D${宏名字})`

```
set(TEST_THREAD true) #change to true or false
# add_definitions: 用于全局添加编译定义，作用范围是整个项目, 前面加-D
if(TEST_THREAD)
    message("include pthread")
    add_definitions(-DTEST_THREAD)
endif()
```

示例二：target_compile_definitions

```
# target_compile_definitions用于为特定的目标（例如可执行文件或库）设置编译定义
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_definitions(c99c11new PUBLIC MY_NAME="gcc")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "NVIDIA")
    target_compile_definitions(c99c11new PUBLIC MY_NAME="nvcc")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(c99c11new PUBLIC MY_NAME="clang")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(c99c11new PUBLIC MY_NAME="msvc")
else()
    target_compile_definitions(c99c11new PUBLIC MY_NAME="not defined")
endif()
```

然后可以在代码里使用MY_NAME和TEST_THREAD这两个宏，就好像代码里的“#define MY_NAME”

## 05-link_library

1. add_subdirectory(sub1)添加子模块。sub1可以是绝对路径，也可以是相对路径。
2. sub1和sub2目录下必须存在CMakeLists.txt文件
3. target_link_libraries链接到so文件和.a文件。
```cmake
target_link_libraries(module
    PRIVATE
        hello::library
        welcome_library
)
```
这里我把private改成public，生成的makefile我同样没对比处差异。由于生成物还有CMakeCache.txt、cmake_install.cmake等文件，也许区别在这些里面。总之既然都能编译通过、运行成功，这些先不管它。
