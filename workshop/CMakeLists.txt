set(TARGET Workshop)

# /WX if warnings should be treated as errors
add_compile_options(/W4 /external:I${PROJECT_SOURCE_DIR}/third-party /external:W0)
# consider target_compile_options

add_executable(${TARGET}
  main.cpp
)

target_link_libraries(${TARGET} PUBLIC
  glad
  glfw
)

# cxx_std_26 was added in CMake v3.30
target_compile_features(${TARGET} PRIVATE cxx_std_23)

add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo "Built Target file: $<TARGET_FILE:${TARGET}>")