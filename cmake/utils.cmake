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
endfunction()