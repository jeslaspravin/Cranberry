# 
# \file EngineFileUtilities.cmake
# 
# \author Jeslas Pravin
# \date January 2022
# \copyright
#  Copyright (C) Jeslas Pravin, Since 2022
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

# Appends some cmake searching directories to given path
function(make_find_package_hints package_name base_dir out_hint_paths)
    set(${out_hint_paths} 
        ${base_dir}/lib/cmake/
        ${base_dir}/cmake/
        ${base_dir}/${package_name}/lib/cmake/
        ${base_dir}/${package_name}/cmake/
        PARENT_SCOPE
    )
endfunction()

function(write_cmdline_args out_file)
    set(multi_value_args CMDLINE)
    cmake_parse_arguments(cmdline_args "" "" "${multi_value_args}" ${ARGN})

    set(cmdline_args "")
    foreach (cmdline_arg ${cmdline_args_CMDLINE})
        string (APPEND cmdline_args "${cmdline_arg}\n")
    endforeach ()
    configure_file(${cmake_script_dir}/ConfigureFiles/CmdLineArgs.in ${out_file} @ONLY)
endfunction()

# Root must be absolute path
function(get_first_level_cmake_lists root_dir out_sub_dirs)
    set (sub_dirs )
    set (dir_tree ${root_dir})
    while (ON)
        # new sub directories to check if current subdirectory did not had any CMakeLists.txt
        set(new_dir_tree )
        foreach (curr_dir ${dir_tree})        
            # Why recurse when going to filter only immediate files and directories
            # file (GLOB_RECURSE all_child_dir LIST_DIRECTORIES true
            #     ${curr_dir}/*)
            # # filter all directories with depth >= 1 from curr_dir and files with .*.extensions
            # list(FILTER all_child_dir EXCLUDE REGEX "${curr_dir}/(.*[./].*)")
            file (GLOB all_child_dir LIST_DIRECTORIES true
                ${curr_dir}/*)
            
            foreach (path ${all_child_dir})
                if (IS_DIRECTORY ${path})
                    # message("Directory : ${path}")
                    # If this sub dir has CMakeLists.txt then this must be sub-directory for current CMakeLists.txt, else this sub-directory must be scanned further
                    if (EXISTS ${path}/CMakeLists.txt)
                        message("-- Found CMakeList under ${path}")
                        list(APPEND sub_dirs ${path})
                    else (EXISTS ${path}/CMakeLists.txt)
                        list(APPEND new_dir_tree ${path})
                    endif (EXISTS ${path}/CMakeLists.txt)
                endif (IS_DIRECTORY ${path})
            endforeach ()
        endforeach ()
        # message("New dirs ${new_dir_tree}")
        set(dir_tree ${new_dir_tree})
        # If no more directories break
        list(LENGTH dir_tree dirs_count)
        if (${dirs_count} EQUAL 0)
            break ()
        endif ()
    endwhile ()
    
    set (rel_sub_dirs )
    foreach(sub_dir ${sub_dirs})
        file(RELATIVE_PATH rel_sub_dir ${root_dir} ${sub_dir})  
        # message("Relative Sub dir ${rel_sub_dir}")
        list(APPEND rel_sub_dirs ${rel_sub_dir})
    endforeach()
    
    set (${out_sub_dirs} ${rel_sub_dirs} PARENT_SCOPE)
endfunction()

# gather source files are done under CMAKE_CURRENT_LIST_DIR directory
function(gather_all_csharp_srcs out_file_list)
    file (GLOB_RECURSE file_list 
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        CONFIGURE_DEPENDS
        *.cs)
    set(${out_file_list} ${file_list} PARENT_SCOPE)
endfunction()

function(gather_all_cpp_srcs out_file_list)
    file (GLOB_RECURSE file_list 
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        CONFIGURE_DEPENDS
        *.c[px][px] *.c *.hpp *.h *.inl)
    set(${out_file_list} ${file_list} PARENT_SCOPE)
endfunction()
function(get_all_cpp_files_in_dirs out_file_list)
    set(one_value_args RELATIVE)
    set(multi_value_args DIRS)
    cmake_parse_arguments(get_cpp_files "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (all_srcs )
    foreach(dir_path ${get_cpp_files_DIRS})
        file (GLOB_RECURSE file_list 
        RELATIVE ${get_cpp_files_RELATIVE}
        LIST_DIRECTORIES false
        ${dir_path}/*.c[px][px] ${dir_path}/*.c ${dir_path}/*.hpp ${dir_path}/*.h ${dir_path}/*.inl)
        
        list (APPEND all_srcs ${file_list})
    endforeach()
    set(${out_file_list} ${all_srcs} PARENT_SCOPE)
endfunction()

macro(add_cmake_subdirectories)
    get_first_level_cmake_lists(${CMAKE_CURRENT_LIST_DIR} cmake_sub_dirs)
    foreach (sub_dir ${cmake_sub_dirs})
        add_subdirectory(${sub_dir})
    endforeach()
endmacro()

# Copies dll and other runtime common dependencies
function(copy_target_exe_runtime_deps target_name)    
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${target_name}> $<TARGET_FILE_DIR:${target_name}>
        COMMAND_EXPAND_LISTS
    )
endfunction ()