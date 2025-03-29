# Could've added as a git submodule via following command:
#     git submodule add --branch glad2 https://github.com/Dav1dde/glad.git
# However, that'd require Python and jinja2 be installed.
# Instead just downloaded OpenGL v4.6 Core will all extensions and "alias" option checked
# from: https://gen.glad.sh/
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c PROPERTIES LANGUAGE C)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
add_custom_command(TARGET glad POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo "Built Target file: $<TARGET_FILE:glad>")