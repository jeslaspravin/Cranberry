########################################################################################################
# C++ related functions
########################################################################################################

#
# CPP helper functions
#

# split_src_per_access : Splits input source files list into Public and Private based on matching if It has /Private/ folder in the path
function (split_src_per_access)    
    set(one_value_args OUTPUT_PRIVATE OUTPUT_PUBLIC)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(split_src_per_access "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (pub_srcs )
    set (pri_srcs )
    foreach (src ${split_src_per_access_SOURCES})
        if(${src} MATCHES ".*/*Private/.*")
            list (APPEND pri_srcs ${src})
        elseif (${src} MATCHES ".*/*Public/.*")
            list (APPEND pub_srcs ${src})
        else (${src} MATCHES ".*/*Private/.*") # Case where source is in directory other than private or public
            list (APPEND pri_srcs ${src})
        endif(${src} MATCHES ".*/*Private/.*")
    endforeach (src ${split_src_per_access_SOURCES})

    set (${split_src_per_access_OUTPUT_PRIVATE} ${pri_srcs} PARENT_SCOPE)
    set (${split_src_per_access_OUTPUT_PUBLIC} ${pub_srcs} PARENT_SCOPE)
endfunction ()

# filter_platform_sources : Filters src_files into current platform source files and non platform source files
#   ${CMAKE_CURRENT_LIST_DIR}/<Platform>/ - All files under this folder will be considered for current platform
#   ${CMAKE_CURRENT_LIST_DIR}/ - Everything else under this folder will be remaining non platform sources
# in_out_src_files - Input src_files variable name, Relative to ${CMAKE_CURRENT_LIST_DIR} folder
# out_platform_private_src_files - Platform selected private source files
# out_platform_public_src_files - Platform selected private public files
function (filter_platform_sources in_out_src_files out_platform_private_src_files out_platform_public_src_files)
    set (platform_files )
    set (out_src_files )
    # Separate Platform and none platform files
    foreach (src ${${in_out_src_files}})
        set (match_pattern "^${match_any_platform_folder}/.*")
        if (${src} MATCHES ${match_pattern})
            set (match_pattern "^${platform_folder}/.*")
            # Only current platform folders are added rest are skipped
            if (${src} MATCHES ${match_pattern})
                list (APPEND platform_files ${src})
            endif ()
        else (${src} MATCHES ${match_pattern})
            list (APPEND out_src_files ${src})
        endif (${src} MATCHES ${match_pattern})
    endforeach ()

    split_src_per_access(
        SOURCES ${platform_files}
        OUTPUT_PRIVATE private
        OUTPUT_PUBLIC public)
    
    set (${out_platform_private_src_files} ${private} PARENT_SCOPE)
    set (${out_platform_public_src_files} ${public} PARENT_SCOPE)
    set (${in_out_src_files} ${out_src_files} PARENT_SCOPE)    
endfunction ()

# target_src_files - Input src_files variable name, Relative to ${CMAKE_CURRENT_LIST_DIR} folder
function (set_target_sources)
    set(one_value_args TARGET_NAME)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(target_srcs "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    filter_platform_sources(target_srcs_SOURCES platform_pri platform_pub)
    # message ("Target : ${target_srcs_TARGET_NAME}")
    # message ("Platform Private : ${platform_pri}")
    # message ("Platform Public : ${platform_pub}")
    split_src_per_access(
        SOURCES ${target_srcs_SOURCES}
        OUTPUT_PRIVATE pri_srcs
        OUTPUT_PUBLIC pub_srcs)
        
    # message ("Private : ${pri_srcs}")
    # message ("Public : ${pub_srcs}")

    target_sources(${target_srcs_TARGET_NAME}
        PRIVATE
            ${platform_pri} ${pri_srcs}
        PUBLIC
            ${platform_pub} ${pub_srcs}
    )
endfunction ()

# Recursively finds all engine modules that this target depends on
# Use with
# configure_file(${cmake_script_dir}/${configure_file_folder}/ModuleDependencies.list.in ${CMAKE_CURRENT_BINARY_DIR}/ModuleDependencies.list
#     USE_SOURCE_PERMISSIONS 
#     @ONLY
# )
#
# DIRECT_MODULES Modules target depends on
# OUT_ALL_MODULES
function (find_all_modules_recursively)
    set(one_value_args OUT_ALL_MODULES)
    set(multi_value_args DIRECT_MODULES)
    cmake_parse_arguments(all_modules "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (out_modules )
    set (modules_tree ${all_modules_DIRECT_MODULES})
    while (ON)
        set (new_modules_tree )
        foreach (module ${modules_tree})
            # Skip if already visited module
            list(FIND out_modules ${module} found_idx)
            if (found_idx GREATER_EQUAL 0)
                continue ()
            endif ()
            
            list(APPEND out_modules ${module})
            get_target_property(module_bin_dir ${module} BINARY_DIR)            
            if (EXISTS ${module_bin_dir}/ModuleDependencies.list)
                file(READ ${module_bin_dir}/ModuleDependencies.list dep_module_list)
                foreach (new_item ${dep_module_list})
                    STRING(STRIP ${new_item} new_module)
                    # If not just empty spaces
                    if(${new_module} MATCHES ".*")
                        message ("Module ${new_module}")
                        list (APPEND new_modules_tree ${new_module})
                    endif ()
                endforeach ()
            endif ()
        endforeach ()
        
        set (modules_tree ${new_modules_tree})        
        list(LENGTH modules_tree modules_count)
        if (modules_count EQUAL 0)
            break ()
        endif ()
    endwhile ()
    
    set (${all_modules_OUT_ALL_MODULES} ${out_modules} PARENT_SCOPE)
endfunction ()

#
# CPP Project related functions
#
macro (cpp_common_options_and_defines)
    target_compile_definitions(${target_name}
        PRIVATE
            $<IF:${WIN32}, PLATFORM_WINDOWS=1, PLATFORM_WINDOWS=0>
            $<IF:${LINUX}, PLATFORM_LINUX=1, PLATFORM_LINUX=0>
            $<IF:${APPLE}, PLATFORM_APPLE=1, PLATFORM_APPLE=0>
            $<IF:$<BOOL:${engine_static_modules}>, STATIC_LINKED=1, STATIC_LINKED=0>
    )

    # POD/Variables in class has to initialized with {} to zero initialize if calling constructors that are not compiler generated
    # target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/sdl>)        
endmacro ()

macro (cpp_common_includes)
    # Includes 
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/Private)
        list (APPEND private_includes Private)
    endif()
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/Public)
        list (APPEND public_includes Public)
    endif ()
    # Platform includes
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${platform_folder}/Private)
        list (APPEND private_includes ${platform_folder}/Private)
    endif()
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${platform_folder}/Public)
        list (APPEND public_includes ${platform_folder}/Public)
    endif ()
    # Private includes
    list (REMOVE_DUPLICATES private_includes)
    list (LENGTH private_includes private_inl_count)
    if (${private_inl_count} GREATER 0)
        target_include_directories (${target_name} 
            PRIVATE
                ${private_includes}
        )
    endif ()
    # Public includes
    list (REMOVE_DUPLICATES public_includes)
    list (LENGTH public_includes public_inl_count)
    if (${public_inl_count} GREATER 0)
        target_include_directories (${target_name} 
            PUBLIC
                ${public_includes}
        )
    endif ()
endmacro ()

# Both Shared/Static libraries or Engine module libraries
macro (cpp_common_dependencies)
    # Private libraries
    list (LENGTH private_libraries private_libraries_count)
    if (${private_libraries_count} GREATER 0)
        target_link_libraries (${target_name} PRIVATE ${private_libraries})
    endif ()
    # Public dependencies
    list (LENGTH public_libraries public_libraries_count)
    if (${public_libraries_count} GREATER 0)
        target_link_libraries (${target_name} PUBLIC ${public_libraries})
    endif ()
    # Interface Libraries
    list (LENGTH interface_libraries interface_libraries_count)
    if (${interface_libraries_count} GREATER 0)
        target_link_libraries (${target_name} INTERFACE ${interface_libraries})
    endif ()
endmacro()

macro (engine_module_dependencies)

    # Private dependencies
    list (LENGTH private_modules private_modules_count)
    if (${private_modules_count} GREATER 0)
        if (${engine_static_modules})
            target_link_libraries (${target_name} PUBLIC ${private_modules})
        else ()
            target_link_libraries (${target_name} PRIVATE ${private_modules})
        endif ()
    endif ()
    # Public dependencies
    list (LENGTH public_modules public_modules_count)
    if (${public_modules_count} GREATER 0)
        target_link_libraries (${target_name} PUBLIC ${public_modules})
    endif ()
    # Interface dependencies
    list (LENGTH interface_modules interface_modules_count)
    if (${interface_modules_count} GREATER 0)
        target_link_libraries (${target_name} INTERFACE ${interface_modules})
    endif ()
    
    # Since we do not want all symbols that are not referenced removed as some were left out in local context like static initialized factory registers    
    if (${engine_static_modules})
        foreach (module ${private_modules} ${public_modules})
            target_link_options(${target_name} 
                PRIVATE 
                    $<$<CXX_COMPILER_ID:MSVC>:/WHOLEARCHIVE:${module}> 
                    $<$<CXX_COMPILER_ID:GNU>:--whole-archive ${module}> 
                    $<$<CXX_COMPILER_ID:Clang>:-force_load ${module}>
            )
        endforeach ()
    endif ()
endmacro ()

# Converts engine modules as just includes for this target
macro (engine_module_dependencies_includes)    
    set(engine_module_pri_incls )
    set(engine_module_pub_incls )
    set(engine_module_interface_incls )

    # Private dependencies
    list (LENGTH private_modules private_modules_count)
    if (${private_modules_count} GREATER 0)
        foreach (module ${private_modules})
            get_target_property(incl_src_dir ${module} SOURCE_DIR)
            if (EXISTS ${incl_src_dir}/Public)
                list (APPEND engine_module_pri_incls ${incl_src_dir}/Public)
            endif ()
            # Platform includes
            if (EXISTS ${incl_src_dir}/${platform_folder}/Public)
                list (APPEND engine_module_pri_incls ${incl_src_dir}/${platform_folder}/Public)
            endif ()
        endforeach ()
    endif ()
    # Public dependencies
    list (LENGTH public_modules public_modules_count)
    if (${public_modules_count} GREATER 0)        
        foreach (module ${public_modules})
            get_target_property(incl_src_dir ${module} SOURCE_DIR)
            if (EXISTS ${incl_src_dir}/Public)
                list (APPEND engine_module_pub_incls ${incl_src_dir}/Public)
            endif ()
            # Platform includes
            if (EXISTS ${incl_src_dir}/${platform_folder}/Public)
                list (APPEND engine_module_pub_incls ${incl_src_dir}/${platform_folder}/Public)
            endif ()
        endforeach ()
    endif ()
    # Interface dependencies
    list (LENGTH interface_modules interface_modules_count)
    if (${interface_modules_count} GREATER 0)
        foreach (module ${interface_modules})
            get_target_property(incl_src_dir ${module} SOURCE_DIR)
            if (EXISTS ${incl_src_dir}/Public)
                list (APPEND engine_module_pub_incls ${incl_src_dir}/Public)
            endif ()
            # Platform includes
            if (EXISTS ${incl_src_dir}/${platform_folder}/Public)
                list (APPEND engine_module_interface_incls ${incl_src_dir}/${platform_folder}/Public)
            endif ()
        endforeach ()
    endif ()
    
    target_include_directories(${target_name} 
        PRIVATE
            ${engine_module_pri_incls}
        PUBLIC
            ${engine_module_pub_incls}
        INTERFACE
            ${engine_module_interface_incls}
    )
endmacro ()

macro (mark_delay_loaded_dlls)
    set (delay_load_list )

    # If static linked then having engine modules as delay loaded does not makes sense
    if (NOT ${engine_static_modules})
        # Private dependencies
        list (LENGTH private_modules private_modules_count)
        if (${private_modules_count} GREATER 0)
            list (APPEND delay_load_list ${private_modules})
        endif ()
        # Public dependencies
        list (LENGTH public_modules public_modules_count)
        if (${public_modules_count} GREATER 0)
            list (APPEND delay_load_list ${public_modules})
        endif ()
        # Interface dependencies
        list (LENGTH interface_modules interface_modules_count)
        if (${interface_modules_count} GREATER 0)
            list (APPEND delay_load_list ${interface_modules})
        endif ()
        
        # ProgramCore module must be skipped as it has all base code for loading modules,
        # So instead we add POST_BUILD copy here
        set (program_core_module "ProgramCore")
        list (FIND delay_load_list ${program_core_module} program_core_idx)
        if (${program_core_idx} GREATER_EQUAL 0)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${program_core_module}> $<TARGET_FILE_DIR:${target_name}>
                COMMAND_EXPAND_LISTS
            )
            # Remove ProgramCore
            list (REMOVE_ITEM delay_load_list ${program_core_module})
        endif (${program_core_idx} GREATER_EQUAL 0)
    endif (NOT ${engine_static_modules})

    # delay load dlls
    list (APPEND delay_load_list ${delay_load_dlls})    

    if (${WIN32})        
        foreach (module ${delay_load_list})
            # .dll extension is needed for /DELAYLOAD linker option
            target_link_options(${target_name} 
                PRIVATE 
                    $<$<CXX_COMPILER_ID:MSVC>:/DELAYLOAD:${module}.dll>
            )
        endforeach ()
    endif ()
endmacro ()

macro (generate_cpp_console_project)
    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_all_cpp_files(src_files)

    source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX Sources FILES ${src_files})

    add_executable(${target_name})
    set_target_sources(
        TARGET_NAME ${target_name}
        SOURCES ${src_files})

    cpp_common_options_and_defines()
    cpp_common_includes()
    cpp_common_dependencies()

    # We add this just in case some executable uses engine modules
    engine_module_dependencies()
endmacro()

# Only maximum 1 argument is allowed, 1st argument if is true then build as object library
macro (generate_cpp_static_lib)
    include(GenerateExportHeader)

    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_all_cpp_files(src_files)

    source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX Sources FILES ${src_files})

    add_library(${target_name} STATIC)

    generate_export_header(${target_name} 
        EXPORT_FILE_NAME ${target_generated_path}/Public/${target_name}Exports.h
    )
    # Make export macros void since this is static library
    target_compile_definitions(${target_name} 
        PUBLIC 
            $<UPPER_CASE:${target_name}>_STATIC_DEFINE
    )
    # Since we need generated headers to be visible outside
    list(APPEND public_includes ${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}/Public)
    
    set_target_sources(
        TARGET_NAME ${target_name}
        SOURCES ${src_files})

    cpp_common_options_and_defines()
    cpp_common_includes()
    cpp_common_dependencies()
endmacro()

macro (generate_cpp_shared_lib)
    include(GenerateExportHeader)

    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_all_cpp_files(src_files)

    source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX Sources FILES ${src_files})
    
    add_library(${target_name} SHARED)

    generate_export_header(${target_name} 
        EXPORT_FILE_NAME ${target_generated_path}/Public/${target_name}Exports.h
    )
    # Since we need generated headers to be visible outside
    list(APPEND public_includes ${CMAKE_CURRENT_BINARY_DIR}/${target_generated_path}/Public)

    # After generating
    set_target_sources(
        TARGET_NAME ${target_name}
        SOURCES ${src_files})
        
    cpp_common_options_and_defines()
    cpp_common_includes()
    cpp_common_dependencies()
endmacro()

# Used for engine modules to determine whether to build as shared or static library
macro (generate_engine_library)
    if (${engine_static_modules})
        generate_cpp_static_lib()
    else ()
        generate_cpp_shared_lib()
    endif ()
    engine_module_dependencies()
    set_target_properties(${target_name} PROPERTIES 
        FOLDER ${CMAKE_PROJECT_NAME})
endmacro()

########################################################################################################
# C# related functions
########################################################################################################
macro(generate_csharp_console_project)
    include(${cmake_script_dir}/EngineFileUtilities.cmake)

    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)

    file (GLOB_RECURSE file_list 
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        CONFIGURE_DEPENDS
        *)
    add_executable(${target_name}
        ${file_list}
    )

    # Add in the .NET reference libraries. Append to some commonly needed references
    set (all_references 
        Microsoft.CSharp
        System
        System.Core
        System.Data.DataSetExtensions
        System.Data
        System.Xml
        System.Xml.Linq
        System.Net.Http
        ${references})
    # Set the .NET Framework version for the executable.
    set_target_properties(${target_name}
        PROPERTIES
            VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
            # If necessary, link in other library/DLL references, such as 3rd party libraries.
            # VS_DOTNET_REFERENCE_MyThirdPartyLib /path/to/libs/MyThirdPartyLib.dll
        )
    
    set_property(TARGET ${target_name} PROPERTY VS_DOTNET_REFERENCES ${all_references})
    # Set the C# language version.
    set(CMAKE_CSharp_FLAGS "/langversion:7.3")

    # If necessary, link in other library dependencies that were built locally in this source tree.
    # target_link_libraries(MyWinFormApp MyLocalLib)
endmacro()

########################################################################################################
# Shader related functions
########################################################################################################

function(shader_outputs)    
    set(one_value_args OUTPUT_DIR OUTPUTS)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(shader_outputs "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(unique_shaders )
    foreach(src_file ${shader_outputs_SOURCES})
        # Matching files that ends with .glsl
        if(${src_file} MATCHES ".*[.glsl]$")
            # In order to remove all directories, And expose inner extension
            get_filename_component(src_wo_glsl ${src_file} NAME_WLE)
            # If include file then ignore it
            if(NOT ${src_wo_glsl} MATCHES ".*[.inl]$")
                # Replace any .vert, .frag ... but capturing matches before .extension
                string(REGEX REPLACE "(.*)[.].*" "\\1" src_wo_ext ${src_wo_glsl})
                list(APPEND unique_shaders ${shader_outputs_OUTPUT_DIR}/$<CONFIG>/Shaders/${src_wo_ext}.ref ${shader_outputs_OUTPUT_DIR}/$<CONFIG>/Shaders/${src_wo_ext}.shader)
            endif()
        endif(${src_file} MATCHES ".*[.glsl]$")
    endforeach()
    list(REMOVE_DUPLICATES unique_shaders)
    set(${shader_outputs_OUTPUTS} ${unique_shaders} PARENT_SCOPE)
endfunction()