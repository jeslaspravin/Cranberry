include(EngineProjectMacros)

set (private_modules
    ProgramCore
    EngineRenderer
    Application
)

# TODO : Move unnecessary public includes to private
set(public_includes
    ${Cranberry_VULKAN_SDK_PATH}/include
    ${Cranberry_CPP_LIBS_PATH}/glm
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_definitions(${target_name} 
    PRIVATE 
        _VERBOSE
        DEFER_DELETION=1)