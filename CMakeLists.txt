cmake_minimum_required(VERSION 2.8)
project(sylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

# 向工程添加多个特定的头文件搜索路径
include_directories(include)
include_directories(include/http)
include_directories(include/stream)
# 向工程添加多个特定的库文件搜索路径(静态库.a和动态库.so)
link_directories(/home/wwt/sylar/lib)  #用绝对路径

set(SRC src/log.cpp 
        src/util.cpp 
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
        src/http/httpserver_parser.rl.cpp
        src/http/httpclient_parser.rl.cpp
        src/http/http_parser.cpp
        src/http/http_connection.cpp
        src/http/http_session.cpp
        src/http/http_server.cpp
        src/http/http_servlet.cpp
        src/tcp_server.cpp
        src/stream/stream.cpp
        src/stream/socket_stream.cpp
        src/uri.cpp
        src/daemon.cpp
        src/env.cpp
        src/application.cpp)

add_library(sylar SHARED ${SRC})  #通过变量SRC生成libsylar.so共享库
redefine_file_macro(sylar)  #为输出目标添加__FILE__宏重定义功能

set(LIB sylar
        dl
        yaml-cpp
        pthread)


add_executable(log_test tests/log_test.cpp)  #生成可执行文件
add_dependencies(log_test sylar)             #添加依赖(依赖sylar库的符号定义)
redefine_file_macro(log_test)                #为输出目标添加__FILE__宏重定义功能
target_link_libraries(log_test ${LIB})       #为目标可执行文件添加需要链接的共享库(动态库)

add_executable(config_test tests/config_test.cpp)
add_dependencies(config_test sylar)
redefine_file_macro(config_test)
target_link_libraries(config_test ${LIB})

add_executable(thread_test tests/thread_test.cpp)
add_dependencies(thread_test sylar)
redefine_file_macro(thread_test)
target_link_libraries(thread_test ${LIB})

add_executable(util_test tests/util_test.cpp)
add_dependencies(util_test sylar)
redefine_file_macro(util_test)
target_link_libraries(util_test ${LIB})

add_executable(fiber_test tests/fiber_test.cpp)
add_dependencies(fiber_test sylar)
redefine_file_macro(fiber_test)
target_link_libraries(fiber_test ${LIB})

add_executable(scheduler_test tests/scheduler_test.cpp)
add_dependencies(scheduler_test sylar)
redefine_file_macro(scheduler_test)
target_link_libraries(scheduler_test ${LIB})

add_executable(iomanager_test tests/iomanager_test.cpp)
add_dependencies(iomanager_test sylar)
redefine_file_macro(iomanager_test)
target_link_libraries(iomanager_test ${LIB})

add_executable(hook_test tests/hook_test.cpp)
add_dependencies(hook_test sylar)
redefine_file_macro(hook_test)
target_link_libraries(hook_test ${LIB})

add_executable(address_test tests/address_test.cpp)
add_dependencies(address_test sylar)
redefine_file_macro(address_test)
target_link_libraries(address_test ${LIB})

add_executable(socket_test tests/socket_test.cpp)
add_dependencies(socket_test sylar)
redefine_file_macro(socket_test)
target_link_libraries(socket_test ${LIB})

add_executable(bytearray_test tests/bytearray_test.cpp)
add_dependencies(bytearray_test sylar)
redefine_file_macro(bytearray_test)
target_link_libraries(bytearray_test ${LIB})

add_executable(http_test tests/http_test.cpp)
add_dependencies(http_test sylar)
redefine_file_macro(http_test)
target_link_libraries(http_test ${LIB})

add_executable(http_parser_test tests/http_parser_test.cpp)
add_dependencies(http_parser_test sylar)
redefine_file_macro(http_parser_test)
target_link_libraries(http_parser_test ${LIB})

add_executable(tcp_server_test tests/tcp_server_test.cpp)
add_dependencies(tcp_server_test sylar)
redefine_file_macro(tcp_server_test)
target_link_libraries(tcp_server_test ${LIB})

add_executable(echo_server examples/echo_server.cpp)
add_dependencies(echo_server sylar)
redefine_file_macro(echo_server)
target_link_libraries(echo_server ${LIB})

add_executable(http_server_test tests/http_server_test.cpp)
add_dependencies(http_server_test sylar)
redefine_file_macro(http_server_test)
target_link_libraries(http_server_test ${LIB})

add_executable(http_connection_test tests/http_connection_test.cpp)
add_dependencies(http_connection_test sylar)
redefine_file_macro(http_connection_test)
target_link_libraries(http_connection_test ${LIB})

add_executable(uri_test tests/uri_test.cpp)
add_dependencies(uri_test sylar)
redefine_file_macro(uri_test)
target_link_libraries(uri_test ${LIB})

add_executable(my_http_server samples/my_http_server.cpp)
add_dependencies(my_http_server sylar)
redefine_file_macro(my_http_server)
target_link_libraries(my_http_server ${LIB})

add_executable(daemon_test tests/daemon_test.cpp)
add_dependencies(daemon_test sylar)
redefine_file_macro(daemon_test)
target_link_libraries(daemon_test ${LIB})

add_executable(env_test tests/env_test.cpp)
add_dependencies(env_test sylar)
redefine_file_macro(env_test)
target_link_libraries(env_test ${LIB})

add_executable(application_test tests/application_test.cpp)
add_dependencies(application_test sylar)
redefine_file_macro(application_test)
target_link_libraries(application_test ${LIB})

# 设置可执行文件的输出路径,其中 PROJECT_SOURCE_DIR 为项目根目录的路径(CMakeLists.txt所在路径)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
# 设置生成库文件的存放路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

