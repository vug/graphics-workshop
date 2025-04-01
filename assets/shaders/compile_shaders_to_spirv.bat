@echo off
setlocal

glslangValidator.exe --target-env opengl --client opengl100 --glsl-version 460 --entry-point main -o solid_color_vert.spv solid_color.vert
glslangValidator.exe --target-env opengl --client opengl100 --glsl-version 460 --entry-point main -o solid_color_frag.spv solid_color.frag

glslangValidator.exe --target-env opengl --client opengl100 --glsl-version 460 --entry-point main -o triangle_without_vbo_vert.spv triangle_without_vbo.vert
glslangValidator.exe --target-env opengl --client opengl100 --glsl-version 460 --entry-point main -o triangle_without_vbo_frag.spv triangle_without_vbo.frag

endlocal