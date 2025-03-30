set(TARGET Workshop)

# /WX if warnings should be treated as errors
# /utf-8 is needed for dealing with following issue: [openimageio] /utf-8 is required on Windows, but not supplied by the cmake target · Issue #41405 · microsoft/vcpkg https://github.com/microsoft/vcpkg/issues/41405
# solution from: 'Unicode support requires compiling with /utf-8' compiling with msvc · Issue #42965 · microsoft/vcpkg https://github.com/microsoft/vcpkg/issues/42965
add_compile_options(/utf-8 /W4 /external:I${PROJECT_SOURCE_DIR}/third-party /external:W0)
# consider target_compile_options

add_executable(${TARGET}
  main.cpp
)

target_link_libraries(${TARGET} PUBLIC
  assimp
  glad
  glfw
  glm
  imgui
  OpenImageIO::OpenImageIO
)

# cxx_std_26 was added in CMake v3.30
target_compile_features(${TARGET} PRIVATE cxx_std_23)

add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo "Built Target file: $<TARGET_FILE:${TARGET}>")