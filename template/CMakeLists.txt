cmake_minimum_required(VERSION 2.8)
project(project_name)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/WebServer)

include (WebServer/cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

# 向工程添加多个特定的头文件搜索路径
include_directories(.)
include_directories(template)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/WebServer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/WebServer/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/WebServer/include/http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/WebServer/include/stream)
# 向工程添加多个特定的库文件搜索路径(静态库.a和动态库.so)
link_directories(/home/wwt/project_name/WebServer/lib)  #用绝对路径

# 查找并引入Boost库
find_package(Boost REQUIRED)
# 如果找到Boost库,将找到的Boost库的头文件目录添加到包含路径中
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(SRC template/my_module.cpp)

# 通过变量SRC生成libproject_name.so共享库
add_library(project_name SHARED ${SRC})
# 为输出目标添加__FILE__宏重定义功能
redefine_file_macro(project_name)

# 设置可执行文件的输出路径,其中 PROJECT_SOURCE_DIR 为项目根目录的路径(CMakeLists.txt所在路径)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置生成库文件的存放路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

