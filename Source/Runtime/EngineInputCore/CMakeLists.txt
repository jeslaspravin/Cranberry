include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_modules
    ProgramCore
    Application
)

set(private_includes
    ${cpp_libs_path}/glm
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)