# 
# \file GeneratedReflectionsMacros.cmake
# 
# \author Jeslas Pravin
# \date January 2022
# \copyright
#  Copyright (C) Jeslas Pravin, Since 2022
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

function (determine_reflect_gen_files out_generated target_name target_src_dir target_binary_dir)
    file (GLOB_RECURSE headers_list 
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${target_src_dir}/*.h)
    set (out_gen_files)
    # Determine total size including all header files that has ${HeaderName}.gen.h include
    set (total_size 0)
    foreach (header_file ${headers_list})
        file (SIZE ${header_file} header_size)
        # get header's name without last extension and directory
        get_filename_component(header_name ${header_file} NAME_WLE)
        set (match_include "#include [\"<]${header_name}.gen.h[\">]")
        # read the file with matching include statement 
        file (STRINGS ${header_file} header_content REGEX ${match_include})
        # The returned list in header_content will not be empty if match is found
        list (LENGTH header_content matched_len)
        # if include is found then add to total size
        if (${matched_len} GREATER 0)
            math (EXPR total_size "${total_size} + ${header_size}" OUTPUT_FORMAT DECIMAL)
        endif ()
    endforeach ()

    # Right now we set maximum size of generated codes per file to be 500KB
    set (max_gencode_file_size 512000 CACHE INTERNAL STRING)
    math (EXPR num_gen_files "${total_size} / ${max_gencode_file_size}" OUTPUT_FORMAT DECIMAL)
    math (EXPR num_gen_remainder "${total_size} % ${max_gencode_file_size}" OUTPUT_FORMAT DECIMAL)
    # If no remainder then we need to negate 1 as foreach range goes until less than or equal to
    if (${num_gen_remainder} EQUAL 0)
        math (EXPR num_gen_files "${num_gen_files} - 1" OUTPUT_FORMAT DECIMAL)
    endif ()
    # Condition is needed in case of 0 files needs to be generated
    if (${num_gen_files} GREATER_EQUAL 0)
        foreach (file_idx RANGE 0 ${num_gen_files})
            list (APPEND out_gen_files "${target_binary_dir}/${target_generated_path}/Private/${target_name}.gen_${file_idx}.cpp")
        endforeach ()
    endif ()

    set (${out_generated} ${out_gen_files} PARENT_SCOPE)
endfunction ()

function (reflect_target)    
    set(one_value_args TARGET_NAME)
    set(multi_value_args GEN_FILES)
    cmake_parse_arguments(reflect_target "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(gen_files_arg "")
    foreach (gen_file ${reflect_target_GEN_FILES})
        string (APPEND gen_files_arg "\"${gen_file}\" ")
    endforeach ()
    
    # Why generate in first place? Some defines has quoted strings and they cannot be passed as commandline args and also sometime cmdline becomes huge because of these two properties
    # Generating intermediate files that store compile definitions and include directories for reflection generator tool
    # It is okay to be configure generate only as content written is only change when editing cmake files and it in turn regenerates new list
    file (GENERATE OUTPUT $<CONFIG>/${reflect_target_TARGET_NAME}_incls.list
        CONTENT "$<TARGET_PROPERTY:INCLUDE_DIRECTORIES>"
        TARGET ${reflect_target_TARGET_NAME}
    )
    file (GENERATE OUTPUT $<CONFIG>/${reflect_target_TARGET_NAME}_defs.list
        CONTENT "$<TARGET_PROPERTY:COMPILE_DEFINITIONS>"
        TARGET ${reflect_target_TARGET_NAME}
    )

    # Did not used file(GENERATE) with custom dependency via custom target as it was dirty(Shows up as a project in solution), Maybe find a way to create target without showing in solution
    add_custom_command(TARGET ${reflect_target_TARGET_NAME} PRE_BUILD
        COMMAND ModuleReflectTool   --generatedList ${gen_files_arg} --filterDiagnostics
                                    --generatedDir "${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}" 
                                    --moduleSrcDir "${CMAKE_CURRENT_LIST_DIR}" 
                                    --moduleName ${reflect_target_TARGET_NAME}
                                    --moduleExportMacro $<UPPER_CASE:${reflect_target_TARGET_NAME}>_EXPORT
                                    --intermediateDir "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>" 
                                    --includeList "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${reflect_target_TARGET_NAME}_incls.list" 
                                    --compileDefList "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${reflect_target_TARGET_NAME}_defs.list"
                                    --logFileName ${reflect_target_TARGET_NAME}
                                    # --logVerbose
        BYPRODUCTS ${gen_files}
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        # VERBATIM not needed as we took care of quotes manually
        COMMENT "${reflect_target_TARGET_NAME} : Generating reflection codes..."
    )

    # Public is already included along with export header generation, It is okay however
    target_include_directories(${reflect_target_TARGET_NAME}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}/Private
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}/Public
    )
    # add generated files to target_sources
    target_sources(${reflect_target_TARGET_NAME}
        PRIVATE
            ${gen_files}
    )
endfunction()

macro (generate_reflection)
    # ${CMAKE_CURRENT_SOURCE_DIR}; ${CMAKE_CURRENT_LIST_DIR} both points to same in this case, and more over list will always gives this files dir while source points to included file's dir
    determine_reflect_gen_files(gen_files ${target_name} "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    list (LENGTH gen_files gen_files_count)
    if (${gen_files_count} GREATER 0)
        reflect_target(TARGET_NAME ${target_name} GEN_FILES ${gen_files})
    endif ()
endmacro ()