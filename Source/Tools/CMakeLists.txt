include(${cmake_script_dir}/EngineFileUtilities.cmake)

# Since we need tools to be placed in separate directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Tools)
set (CMAKE_FOLDER Tools)
add_cmake_subdirectories()

# clang-format
if (${enable_clang_format})    
    # We do not need to find_package and load all target just clang-format is enough
    find_program(clang_format_exe 
        NAMES "clang-format"
        HINTS ${llvm_install_path}/bin
        DOC "clang-format path"
    )

    if (clang_format_exe)
        get_all_cpp_files_in_dirs(
            file_list 
            RELATIVE ${CMAKE_SOURCE_DIR}/Source
            DIRS 
                ${CMAKE_SOURCE_DIR}/Source
        )
        # Ignore embedded third parties 
        list(FILTER file_list EXCLUDE REGEX ".*/SpirV/.*|.*/ImGuiLib/.*")
        list(LENGTH file_list len)
        
        set (cmdline
            -i
            -style=file
            # --verbose
        )
        set (formating_args_file ${CMAKE_CURRENT_BINARY_DIR}/clang_format_args)
        write_cmdline_args(
            ${formating_args_file}
            CMDLINE
                ${cmdline} ${file_list}
        )

        add_custom_target(FormatCPP
            DEPENDS ${formating_args_file}
            COMMAND ${clang_format_exe} "@${formating_args_file}"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source
            COMMENT "Formatting ${len} engine sources using clang-format..."
            VERBATIM
            SOURCES ${CMAKE_SOURCE_DIR}/.clang-format
        )

        # Exclude from build all
        set_target_properties(FormatCPP PROPERTIES
            EXCLUDE_FROM_ALL ON
        )

    endif ()

endif (${enable_clang_format})

