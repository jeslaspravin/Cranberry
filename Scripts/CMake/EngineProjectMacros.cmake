# 
# \file EngineProjectMacros.cmake
# 
# \author Jeslas Pravin
# \date January 2022
# \copyright
#  Copyright (C) Jeslas Pravin, 2022-2023
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

########################################################################################################
# C++ related functions
#
# Each module is added with common compile options defines as below
#
# Compile Defines
# PLATFORM_WINDOWS in WIN32
# PLATFORM_LINUX in LINUX
# PLATFORM_APPLE in APPLE
# PLATFORM_64 if 64 bit platform
# PLATFORM_32 if 32 bit platform
# STATIC_LINKED if we are building as statically linked module
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
        if(${src} MATCHES ".*/*Private/.*|.*\\.cpp") # Any sources in folder containing 'Private' name or if it is cpp file we mark it as private
            list (APPEND pri_srcs ${src})
        elseif (${src} MATCHES ".*/*Public/.*")
            list (APPEND pub_srcs ${src})
        endif(${src} MATCHES ".*/*Private/.*|.*\\.cpp")
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

#
# CPP Project related functions
#
macro (cpp_common_options_and_defines)
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<BOOL:${Cranberry_STATIC_MODULES}>:STATIC_LINKED=1>
            $<${WIN32}:PLATFORM_WINDOWS=1>
            $<${LINUX}:PLATFORM_LINUX=1>
            $<${APPLE}:PLATFORM_APPLE=1>
            $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:PLATFORM_64=1>
            $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:PLATFORM_32=1>
    )

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

# Generates codes or file that will be used by modules/other tools regarding module dependencies. RIGHT NOW ITS NOT USED
macro (generate_enginelib_depends)    
    # Generating dependent module's directories. This is to walk the dependency tree if neccessary
    # each line contains directories of a module in file named ${target_name}_PrivateEngineLibs.depend, ${target_name}_EngineLibs.depend
    # Change in ModuleReflectTool if changing file description or name
    set (private_engine_lib_dirs "")
    set (public_engine_lib_dirs "")
    foreach (module ${private_modules})
        string(APPEND private_engine_lib_dirs "${module}=$<TARGET_PROPERTY:${module},BINARY_DIR>/$<CONFIG>;$<TARGET_FILE_DIR:${module}>\n")
    endforeach()
    foreach (module ${transitive_modules})
        string(APPEND public_engine_lib_dirs "${module}=$<TARGET_PROPERTY:${module},BINARY_DIR>/$<CONFIG>;$<TARGET_FILE_DIR:${module}>\n")
    endforeach()
    file(GENERATE OUTPUT $<CONFIG>/${target_name}_PrivateEngineLibs.depend
        CONTENT "${private_engine_lib_dirs}"
        TARGET ${target_name}
    )
    file(GENERATE OUTPUT $<CONFIG>/${target_name}_EngineLibs.depend
        CONTENT "${public_engine_lib_dirs}"
        TARGET ${target_name}
    )
endmacro ()

macro (engine_module_dependencies)
    # Private dependencies
    list (LENGTH private_modules private_modules_count)
    if (${private_modules_count} GREATER 0)
        if (${Cranberry_STATIC_MODULES})
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

    # For each INTERFACE exposed modules for transitive dependencies we have to explicitly add include and compile definitions to help when reflecting codes
    set(transitive_modules ${public_modules} ${interface_modules})
    # private_modules are exposed as well in static builds
    if (${Cranberry_STATIC_MODULES})
        list (APPEND transitive_modules ${private_modules})
    endif ()
    foreach (module ${transitive_modules})
        target_include_directories(${target_name}
            PUBLIC
                $<TARGET_PROPERTY:${module},INTERFACE_INCLUDE_DIRECTORIES>
        )
        target_compile_definitions(${target_name}
            PUBLIC
                $<TARGET_PROPERTY:${module},INTERFACE_COMPILE_DEFINITIONS>
        )
    endforeach ()
    # generate_enginelib_depends()
    
    # Since we do not want all symbols that are not referenced removed as some were left out in local context like static initialized factory registers    
    if (${Cranberry_STATIC_MODULES})
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

macro (mark_delay_loaded_dlls)
    
    set(option_args IGNORE_MODULES)
    cmake_parse_arguments(delay_load_arg "${option_args}" "" "" ${ARGN})

    set (delay_load_list )

    # If static linked then having engine modules as delay loaded does not makes sense
    # and if IGNORE_MODULES option is not enabled
    if (NOT ${Cranberry_STATIC_MODULES} AND (NOT ${delay_load_arg_IGNORE_MODULES}))
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
        # So instead we add POST_BUILD copy in ProgramCore
        set (program_core_module "ProgramCore")
        list (FIND delay_load_list ${program_core_module} program_core_idx)
        if (${program_core_idx} GREATER_EQUAL 0)
            # Remove ProgramCore
            list (REMOVE_ITEM delay_load_list ${program_core_module})
        endif (${program_core_idx} GREATER_EQUAL 0)
    endif (NOT ${Cranberry_STATIC_MODULES} AND (NOT ${delay_load_arg_IGNORE_MODULES}))

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

# TARGET_NAME arg to override target_name obtained from directory name
macro (generate_cpp_console_project)
    set(one_value_args TARGET_NAME)
    cmake_parse_arguments(console_project "" "${one_value_args}" "" ${ARGN})
    
    # For C++ application alone we could allow overriding target_name
    if (DEFINED console_project_TARGET_NAME)
        set (target_name ${console_project_TARGET_NAME})
    else (DEFINED console_project_TARGET_NAME)
        get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    endif (DEFINED console_project_TARGET_NAME)

    gather_all_cpp_srcs(src_files)

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
    gather_all_cpp_srcs(src_files)

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
    gather_all_cpp_srcs(src_files)

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
    if (${Cranberry_STATIC_MODULES})
        generate_cpp_static_lib()
    else ()
        generate_cpp_shared_lib()
    endif ()
    engine_module_dependencies()
endmacro()
macro (generate_engine_editor_library)
    generate_engine_library()
    
    # Exclude from all build if we are not building editor target
    if (NOT ${Cranberry_EDITOR_BUILD})
        set_target_properties(${target_name} PROPERTIES
            EXCLUDE_FROM_ALL ON
            EXCLUDE_FROM_DEFAULT_BUILD ON
        )
    endif ()
endmacro()

########################################################################################################
# C# related functions
########################################################################################################
macro(generate_csharp_console_project)
    include(EngineFileUtilities)

    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)

    gather_all_csharp_srcs(file_list)

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
                list(APPEND unique_shaders "${shader_outputs_OUTPUT_DIR}/Shaders/${src_wo_ext}.ref" "${shader_outputs_OUTPUT_DIR}/Shaders/${src_wo_ext}.shader")
            endif()
        endif(${src_file} MATCHES ".*[.glsl]$")
    endforeach()
    list(REMOVE_DUPLICATES unique_shaders)
    set(${shader_outputs_OUTPUTS} ${unique_shaders} PARENT_SCOPE)
endfunction()