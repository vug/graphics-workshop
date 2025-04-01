#version 460

//#include "/lib/DefaultVertexAttributes.glsl"
//#include "/lib/SceneUniforms.glsl"

//uniform mat4 u_WorldFromObject;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord1;

void main() {
  //gl_Position = su.u_ProjectionFromView * su.u_ViewFromWorld * u_WorldFromObject * vec4(a_Position, 1.0);
  gl_Position = vec4(a_Position, 1.0);
}