.PHONY: xx

# 这是一个基于Bash的命令，用于在build目录下使用CMake构建项目
# 该命令使用了shell命令来查找g++和gcc编译器的路径，并将其作为CMake变量传递给构建过程
# 具体来说，-DCMAKE_CXX_COMPILER选项指定C++编译器路径，-DCMAKE_C_COMPILER选项指定C编译器路径
# 而$(shell which g++)和$(shell which gcc)命令用于查找g++和gcc编译器的路径
# 最后..表示上一级目录，即CMakeLists.txt所在目录，用于告诉CMake在哪里查找构建脚本
# cmake -DCMAKE_CXX_COMPILER:FILEPATH=$(shell which g++) -DCMAKE_C_COMPILER:FILEPATH=$(shell which gcc) ..;

# 执行 make 时匹配
"":
	if [ -d "build" ]; then \
		cd build && make; \
	else \
		mkdir build; \
		cd build && cmake ..; \
	fi

# 执行 make <target> 时匹配
%:
	if [ -d "build" ]; then \
		cd build && make $@; \
	else \
		mkdir build; \
		cd build && cmake ..; \
	fi