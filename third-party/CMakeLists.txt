add_compile_options(/W0)

# Could've added as a git submodule via following command:
#     git submodule add --branch glad2 https://github.com/Dav1dde/glad.git
# However, that'd require Python and jinja2 be installed.
# Instead just downloaded OpenGL v4.6 Core will all extensions and "alias" option checked
# from: https://gen.glad.sh/
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c PROPERTIES LANGUAGE C)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
add_custom_command(TARGET glad POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo "Built Target file: $<TARGET_FILE:glad>")

# git submodule add https://github.com/glfw/glfw.git
# The CACHE keyword ensures that the value is set globally and overrides any default values in the GLFW CMakeLists.txt
# The FORCE keyword ensures that the value is updated even if it was previously set in the cache
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# git submodule add https://github.com/g-truc/glm.git
add_subdirectory(glm)
target_compile_definitions(glm PUBLIC GLM_ENABLE_EXPERIMENTAL)

# git submodule add --branch docking https://github.com/ocornut/imgui.git
add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_demo.cpp # for ImGui::ShowDemoWindow()
  imgui/backends/imgui_impl_glfw.cpp
  imgui/backends/imgui_impl_opengl3.cpp
  imgui/misc/cpp/imgui_stdlib.cpp
)
target_compile_definitions(imgui PRIVATE
  -DGLFW_INCLUDE_NONE
)
target_link_libraries(imgui PRIVATE
  glfw
)
target_include_directories(
  imgui PUBLIC
  imgui/
  imgui/backends
  imgui/misc/cpp
)

# git submodule add https://github.com/assimp/assimp.git
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
# USD importing was experimental by the time we added assimp
set(ASSIMP_BUILD_USD_IMPORTER OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# Attempt 1
# Requires a lot of dependencies: https://github.com/AcademySoftwareFoundation/OpenImageIO/blob/main/INSTALL.md#building-on-windows
# https://github.com/AcademySoftwareFoundation/Imath
# https://github.com/AcademySoftwareFoundation/openexr
# https://github.com/madler/zlib
# https://gitlab.com/libtiff/libtiff
# https://github.com/libjpeg-turbo/libjpeg-turbo
# couldn't make it work...
#
# Attempt 2
# install vcpkg
# vcpkg install openimageio:x64-windows-static
# copy lib file and headers manually here
# add_library(OpenImageIO STATIC IMPORTED)
# set_target_properties(OpenImageIO PROPERTIES IMPORTED_LOCATION C:/Users/veliu/repos/graphics-workshop/third-party/OpenImageIO/OpenImageIO.lib)
# target_include_directories(OpenImageIO INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/OpenImageIO/include)
# couldn't make it work...
#
# Attempt 3
# vcpkg install openimageio
# Now find_package(OpenImageIO REQUIRED) can be added to the project root CMakeLists.txt
# message(OpenImageIO_INCLUDE_DIR " ${OpenImageIO_INCLUDE_DIR}")

# TODO: Bring xatlas