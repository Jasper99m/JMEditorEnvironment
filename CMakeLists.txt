
file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(JMEditorEnvironment
    ${EDITOR_SRC}
)

target_link_libraries(JMEditorEnvironment
    PUBLIC JMgui
)

target_include_directories(JMEditorEnvironment
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(JMEditorEnvironment PRIVATE /bigobj)
endif()

if (WIN32)
    target_compile_definitions(JMEditorEnvironment PRIVATE UNICODE _UNICODE)
endif()