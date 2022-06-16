include(EngineCMakeHelpers)

setup_runtime_output_dir("${PROJECT_BINARY_DIR}" "Runtime")
set (CMAKE_FOLDER ${CMAKE_PROJECT_NAME})
add_cmake_subdirectories()