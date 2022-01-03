
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
    foreach (file_idx RANGE 0 ${num_gen_files})
        list (APPEND out_gen_files "${target_binary_dir}/${target_generated_path}/Private/${target_name}.gen_${file_idx}.cpp")
    endforeach ()

    set (${out_generated} ${out_gen_files} PARENT_SCOPE)
endfunction ()

macro (generate_reflection)
    # ${CMAKE_CURRENT_SOURCE_DIR}; ${CMAKE_CURRENT_LIST_DIR} both points to same in this case
    determine_reflect_gen_files(gen_files ${target_name} "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

    set(gen_files_arg "")
    foreach (gen_file ${gen_files})
        string (APPEND gen_files_arg "\"${gen_file}\" ")
    endforeach ()
    
    # Why generate in first place? Some defines has quoted strings and they cannot be passed as commandline args and also sometime cmdline becomes huge because of these two properties
    # Generating intermediate files that store compile definitions and include directories for reflection generator tool
    # It is okay to be configure generate only as content written is only change when editing cmake files and it in turn regenerates new list
    file (GENERATE OUTPUT $<CONFIG>/${target_name}_incls.list
        CONTENT "$<TARGET_PROPERTY:INCLUDE_DIRECTORIES>"
        TARGET ${target_name}
    )
    file (GENERATE OUTPUT $<CONFIG>/${target_name}_defs.list
        CONTENT "$<TARGET_PROPERTY:COMPILE_DEFINITIONS>"
        TARGET ${target_name}
    )
    # Did not used file(GENERATE) with custom dependency via custom target as it was dirty(Shows up as a project in solution), Maybe find a way to create target without showing in solution
    add_custom_command(TARGET ${target_name} PRE_BUILD
        COMMAND ModuleReflectTool   --generatedList ${gen_files_arg} --filterDiagnostics
                                    --generatedDir "${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}" 
                                    --moduleSrcDir "${CMAKE_CURRENT_LIST_DIR}" 
                                    --moduleExportMacro $<UPPER_CASE:${target_name}>_EXPORT
                                    --intermediateDir "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>" 
                                    --includeList "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${target_name}_incls.list" 
                                    --compileDefList "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${target_name}_defs.list"
        BYPRODUCTS ${gen_files}
        USES_TERMINAL
        # VERBATIM not needed as we took care of quotes manually
        COMMENT "${target_name} : Generating reflection codes ..."
    )

    # Public is already included along with export header generation, It is okay however
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}/Private
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}/Public
    )
    # add generated files to target_sources
    target_sources(${target_name}
        PRIVATE
            ${gen_files}
    )
endmacro ()