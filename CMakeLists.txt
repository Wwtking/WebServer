cmake_minimum_required(VERSION 2.8)
project(sylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 向工程添加多个特定的头文件搜索路径
include_directories(include)
include_directories(include/http)
include_directories(include/stream)
include_directories(include/util)
# 向工程添加多个特定的库文件搜索路径(静态库.a和动态库.so)
link_directories(/home/wwt/sylar/lib)  #用绝对路径

# BUILD_TEST 选项将使用默认值 OFF
# cmake .. 时，BUILD_TEST=OFF
# cmake -DBUILD_TEST=ON .. 时，BUILD_TEST=ON
option(BUILD_TEST "ON for complile test" OFF)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(SRC src/log.cpp 
        src/util.cpp 
        src/util/hash_util.cpp 
        src/util/json_util.cpp 
        src/config.cpp 
        src/thread.cpp
        src/fiber.cpp
        src/scheduler.cpp
        src/iomanager.cpp
        src/timer.cpp
        src/fd_manager.cpp
        src/hook.cpp
        src/address.cpp
        src/socket.cpp
        src/bytearray.cpp
        src/http/http.cpp
        src/http/http_parser.cpp
        src/http/http_connection.cpp
        src/http/http_session.cpp
        src/http/http_server.cpp
        src/http/http_servlet.cpp
        src/http/ws_connection.cpp
        src/http/ws_session.cpp
        src/http/ws_server.cpp
        src/http/ws_servlet.cpp
        src/tcp_server.cpp
        src/stream/stream.cpp
        src/stream/socket_stream.cpp
        src/daemon.cpp
        src/env.cpp
        src/application.cpp
        src/module.cpp
        src/library.cpp
        src/worker.cpp)

ragelmaker(src/http/httpserver_parser.rl SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/http/httpclient_parser.rl SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/uri.rl SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(sylar SHARED ${SRC})  #通过变量SRC生成libsylar.so共享库
redefine_file_macro(sylar)  #为输出目标添加__FILE__宏重定义功能

set(LIB sylar
        dl
        yaml-cpp
        jsoncpp
        pthread
        ${OPENSSL_LIBRARIES})

if(BUILD_TEST)

sylar_add_executable(log_test tests/log_test.cpp sylar "${LIB}")
sylar_add_executable(config_test tests/config_test.cpp sylar "${LIB}")
sylar_add_executable(thread_test tests/thread_test.cpp sylar "${LIB}")
sylar_add_executable(util_test tests/util_test.cpp sylar "${LIB}")
sylar_add_executable(fiber_test tests/fiber_test.cpp sylar "${LIB}")
sylar_add_executable(scheduler_test tests/scheduler_test.cpp sylar "${LIB}")
sylar_add_executable(iomanager_test tests/iomanager_test.cpp sylar "${LIB}")
sylar_add_executable(hook_test tests/hook_test.cpp sylar "${LIB}")
sylar_add_executable(address_test tests/address_test.cpp sylar "${LIB}")
sylar_add_executable(socket_test tests/socket_test.cpp sylar "${LIB}")
sylar_add_executable(bytearray_test tests/bytearray_test.cpp sylar "${LIB}")
sylar_add_executable(http_test tests/http_test.cpp sylar "${LIB}")
sylar_add_executable(http_parser_test tests/http_parser_test.cpp sylar "${LIB}")
sylar_add_executable(tcp_server_test tests/tcp_server_test.cpp sylar "${LIB}")
sylar_add_executable(echo_server examples/echo_server.cpp sylar "${LIB}")
sylar_add_executable(http_server_test tests/http_server_test.cpp sylar "${LIB}")
sylar_add_executable(http_connection_test tests/http_connection_test.cpp sylar "${LIB}")
sylar_add_executable(uri_test tests/uri_test.cpp sylar "${LIB}")
sylar_add_executable(my_http_server samples/my_http_server.cpp sylar "${LIB}")
sylar_add_executable(daemon_test tests/daemon_test.cpp sylar "${LIB}")
sylar_add_executable(env_test tests/env_test.cpp sylar "${LIB}")
sylar_add_executable(application_test tests/application_test.cpp sylar "${LIB}")
sylar_add_executable(ws_server_test tests/ws_server_test.cpp sylar "${LIB}")
sylar_add_executable(ws_client_test tests/ws_client_test.cpp sylar "${LIB}")

endif()

sylar_add_executable(bin_sylar src/main.cpp sylar "${LIB}")
# 将 bin_sylar 目标的输出名称设置为 sylar
set_target_properties(bin_sylar PROPERTIES OUTPUT_NAME "sylar")

# 设置可执行文件的输出路径,其中 PROJECT_SOURCE_DIR 为项目根目录的路径(CMakeLists.txt所在路径)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置生成库文件的存放路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

