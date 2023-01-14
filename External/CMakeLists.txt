cmake_minimum_required(VERSION 3.22)

set (libs_archive_url "https://github.com/jeslaspravin/CranberryEngineDependenciesArchive/releases/latest/download/CranberryEngineDependenciesArchive.zip")

message ("Setting up CPP libraries...")
set (libs_path ${CMAKE_CURRENT_LIST_DIR})
set (setup_status_file ${libs_path}/SetupSuccess.txt)
file (REMOVE "${setup_status_file}")

set (libs_archive_file ${libs_path}/libsArchive.zip)
if (EXISTS ${libs_archive_file})
    message ("${libs_archive_file} Exists. Delete it to start a fresh download!")
else (EXISTS ${libs_archive_file})
    message ("Downloading from ${libs_archive_url}")
    file(DOWNLOAD "${libs_archive_url}" "${libs_archive_file}" 
        SHOW_PROGRESS
        STATUS download_status
    )

    list(GET download_status 0 status_code)
    if (NOT ${status_code} EQUAL 0)
        list(GET download_status 1 status_msg)
        message (FATAL_ERROR "${statuc_code} Error: ${status_msg}")
    endif ()
endif (EXISTS ${libs_archive_file})

message ("Success: Download finished!")
message ("Extracting into ${CMAKE_CURRENT_LIST_DIR}")

execute_process(COMMAND ${CMAKE_COMMAND}
    -E tar xf "${libs_archive_file}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    RESULT_VARIABLE exit_code
)
if (NOT ${exit_code} EQUAL 0)
    message(FATAL_ERROR "Failed to extract!")
endif()

file (WRITE "${setup_status_file}" "")
message("SUCCESS: Extraction completed!")