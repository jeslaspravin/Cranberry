
file(GLOB_RECURSE src_files LIST_DIRECTORIES false 
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    *)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${src_files})

add_custom_target(Scripts 
    SOURCES ${src_files}
)