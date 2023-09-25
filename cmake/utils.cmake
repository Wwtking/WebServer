# utils.cmake

# 将每个源文件的__FILE__宏重新定义为相对于项目目录的路径
function(redefine_file_macro targetname)

    # 获取目标文件${targetname}的所有源文件，并将结果存储在变量source_files中
    get_target_property(source_files "${targetname}" SOURCES)
    # 对每个源文件进行循环遍历
    foreach(sourcefile ${source_files})
        # 获取源文件${sourcefile}的当前编译参数，并将结果存储在变量defs中
        get_property(defs SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS)
        # 获取源文件${sourcefile}的绝对路径，并将结果存储在变量filepath中
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        # 将源文件的绝对路径${filepath}中的项目目录${PROJECT_SOURCE_DIR}替换为空字符串，
        # 得到相对于项目目录的路径，并将结果存储在变量relpath中
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        # 将我们要加的编译参数(__FILE__定义)添加到原来的编译参数里面
        list(APPEND defs "__FILE__=\"${relpath}\"")
        # 重新设置源文件的编译参数
        set_property(
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs}
            )
        # 即将__FILE__宏重新定义为相对于项目目录的路径
    endforeach()

endfunction(redefine_file_macro)


function(ragelmaker src_rl outputlist outputdir)

    # 获取 src_rl 的文件名(不包含扩展名,即后缀)，并将其保存到 src_file 变量中
    get_filename_component(src_file ${src_rl} NAME_WE)

    # 将 rl_out 变量设置为输出文件的路径/名称
    set(rl_out ${outputdir}/${src_file}.rl.cpp)

    # 将 rl_out 添加到 outputlist 变量中
    # CMake 中函数内部创建的变量作用域是局部的，需要使用 PARENT_SCOPE 参数将 outputlist 的作用域提升到全局
    set(${outputlist} ${${outputlist}} ${rl_out} PARENT_SCOPE)

    # 创建一个自定义的构建步骤
    # 执行 ragel 命令将 ${src_rl} 文件编译成 ${rl_out} 文件
    add_custom_command(
        # OUTPUT 参数用于指定构建步骤的输出文件
        # 如果输出文件不存在或者比输入文件旧，那么该构建步骤就会被执行
        OUTPUT ${rl_out}
        COMMAND cd ${outputdir}
        COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl} -o ${rl_out} -l -C -G2  --error-format=msvc
        # DEPENDS 参数用于指定构建步骤所依赖的文件
        # 任何一个依赖文件的修改时间比输出文件新，那么该构建步骤就会被重新执行
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
    )

    # 将 ${rl_out} 文件标记为生成文件，当 CMake 构建项目时，将自动生成 ${rl_out} 文件
    set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
    
endfunction(ragelmaker)


function(sylar_add_executable exe_name srcs depends libs)

    add_executable(${exe_name} ${srcs})         # 生成可执行文件
    add_dependencies(${exe_name} ${depends})    # 添加依赖(依赖sylar库的符号定义)
    redefine_file_macro(${exe_name})            # 为输出目标添加__FILE__宏重定义功能
    target_link_libraries(${exe_name} ${libs})  # 为目标可执行文件添加需要链接的共享库(动态库)

endfunction(sylar_add_executable)

