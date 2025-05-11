#version 460

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord1;

layout (location = 0) out vec4 outColor;

void main() {
  // outColor = vec4(1, 0, 0, 1);
  outColor = vec4(normalize(v_Normal) * 0.5 + 0.5, 1);
}