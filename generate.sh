#!bin/sh

# $0	    shell文件本身的文件名
# $1～$n	添加到shell的各参数值
# $?	    最后运行命令的结束代码(返回值)，成功返回0，不成功返回非零值
# $$	    Shell本身的PID
# $!	    Shell最后运行的后台Process的PID
# $*	    所有参数列表
# $@	    所有参数列表
# $#	    添加到Shell的参数个数

# -eq	equal	            等于
# -ne	not equal	        不等于
# -gt	greater than	    大于
# -lt	lesser than	        小于
# -ge	greater or equal	大于或等于
# -le	lesser or equal	    小于或等于


# 检查传递给脚本的参数数量是否小于2
if [ $# -lt 2 ]
then
    # 输出使用说明，告诉用户正确的脚本使用方法
    echo "use [$0 project_name namespace]"
    exit 0
fi

project_name=$1
namespace=$2

# 定义函数:执行传递给它的命令并检查返回值
command_error_exit() {
    # 相当于该函数调用时的所有参数
    $*
    # 检查上一条命令的返回值
    if [ $? -ne 0 ] 
    then
        echo "$* error"
        exit 1
    fi
}

# mkdir $project_name 是函数 command_error_exit 调用时的参数
# 相当于执行
# mkdir $project_name
# if [$? -ne 0] 
# then
#     echo "$* error"
#     exit 0
# fi
command_error_exit mkdir $project_name
command_error_exit cd $project_name
command_error_exit git clone git@github.com:Wwtking/WebServer.git
command_error_exit cp WebServer/Makefile .
command_error_exit cp -rf WebServer/template/* .
command_error_exit mv template $namespace
# sed 可以从标准输入或文件中读取文本，对文本进行编辑，并将结果输出到标准输出或文件中
# sed 命令的 -i 选项表示直接修改文件，而不是输出到标准输出
# sed -i "s/原字符串/新字符串/"  文件路径
# sed -i "s/原字符串/新字符串/g" 文件路径
# 加了 g 就是每行都全局替换，没有 g 就是每行只替换第一个匹配的
command_error_exit sed -i "s/project_name/$project_name/g" CMakeLists.txt
command_error_exit sed -i "s/template/$namespace/g" CMakeLists.txt
command_error_exit sed -i "s/project_name/$project_name/g" move.sh
command_error_exit cd $namespace
command_error_exit sed -i "s/project_name/$project_name/g" `ls .`
command_error_exit sed -i "s/name_space/$namespace/g" `ls .`
command_error_exit cd ../conf
command_error_exit sed -i "s/project_name/$project_name/g" `ls .`
command_error_exit sed -i "s/name_space/$namespace/g" `ls .`

echo "create module $project_name -- $namespace ok" 





