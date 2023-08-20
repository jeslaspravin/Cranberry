cmake_minimum_required(VERSION 3.22)

set (deps_archive_url "https://onedrive.live.com/download?resid=C09B57D393088DFC%21953&authkey=!ANzygq39nI4gL4w")
set (deps_version_url "https://onedrive.live.com/download?resid=C09B57D393088DFC%21954&authkey=!AM92Kncb5ygvmE8")

set (temp_download_folder CranberryDeps-Temp)

set (external_libs_path ${CMAKE_CURRENT_LIST_DIR})
set (deps_setup_status_file ${external_libs_path}/DepsSetupSuccess.txt)

message ("Setting up CPP libraries...")

file (GLOB current_deps
     LIST_DIRECTORIES true 
     RELATIVE ${external_libs_path}
     "${external_libs_path}/*"
)
# Ignore this CMakeLists.txt
list (REMOVE_ITEM current_deps "CMakeLists.txt")

# Getting latest version number
set (deps_version_file ${external_libs_path}/${temp_download_folder}/DependenciesVersion.txt)
file(DOWNLOAD "${deps_version_url}" "${deps_version_file}"
    STATUS download_status
    LOG download_log
    HTTPHEADER "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36"
)
list(GET download_status 0 status_code)
if (NOT ${status_code} EQUAL 0)
    list(GET download_status 1 status_msg)
    message (FATAL_ERROR "Status code ${status_code} Error grabbing version file: ${status_msg}\nLOG ${download_log}")
endif ()
file (READ ${deps_version_file} new_deps_version)
set (old_deps_version "0.0.0")
if (EXISTS ${deps_setup_status_file})
    file (READ ${deps_setup_status_file} old_deps_version)
endif()

if (${new_deps_version} VERSION_LESS_EQUAL ${old_deps_version})
    message("Current setup dependency version is up to date! Setup completed!")
    file(REMOVE_RECURSE ${external_libs_path}/${temp_download_folder})
    return()
endif()

set (deps_archive_file ${external_libs_path}/${temp_download_folder}/DepsArchive.zip)
if (EXISTS ${deps_archive_file})
    message ("${deps_archive_file} Exists. Delete it to start a fresh download!")
else (EXISTS ${deps_archive_file})
    message ("Downloading dependencies archive... \nfrom ${deps_archive_url}\nto ${deps_archive_file}")
    file(DOWNLOAD "${deps_archive_url}" "${deps_archive_file}" 
        SHOW_PROGRESS
        STATUS download_status
        LOG download_log
        HTTPHEADER "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36"
    )

    list(GET download_status 0 status_code)
    if (NOT ${status_code} EQUAL 0)
        list(GET download_status 1 status_msg)
        message (FATAL_ERROR "Status code ${status_code} Error: ${status_msg}\nLOG ${download_log}")
    endif ()    
    message ("Success: Download finished!")
endif (EXISTS ${deps_archive_file})

message ("Extracting dependencies into ${external_libs_path}/${temp_download_folder}")
execute_process(COMMAND ${CMAKE_COMMAND}
    -E tar xf "${deps_archive_file}"
    WORKING_DIRECTORY "${external_libs_path}/${temp_download_folder}"
    RESULT_VARIABLE exit_code
)
if (NOT ${exit_code} EQUAL 0)
    message(FATAL_ERROR "Failed to extract dependencies!")
endif()
file (REMOVE "${deps_archive_file}")

list (LENGTH current_deps had_old_deps)
if (${had_old_deps} GREATER 0)
    list(TRANSFORM current_deps PREPEND "${external_libs_path}/")
    file (REMOVE_RECURSE ${current_deps})
endif()

message ("Copying dependencies...")
file (COPY_FILE ${deps_version_file} ${deps_setup_status_file})
execute_process(COMMAND ${CMAKE_COMMAND}
    -E copy_directory "${external_libs_path}/${temp_download_folder}" "${external_libs_path}"
    RESULT_VARIABLE exit_code
)
if (NOT ${exit_code} EQUAL 0)
    message(FATAL_ERROR "Failed to copy dependencies from temporary dir to ${external_libs_path}!")
endif()

file(REMOVE_RECURSE ${external_libs_path}/${temp_download_folder})

message("SUCCESS: Extraction completed!")