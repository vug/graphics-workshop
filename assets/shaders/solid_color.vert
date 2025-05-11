#version 460

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord1;

layout(std140, binding = 0) uniform PerFrameData {
    mat4 viewFromWorld;
    mat4 projectionFromView;
    float fishEyeStrength;
} u_FrameData;

layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord1;

void main() {
  const mat4 worldFromObject = mat4(1.0);
  const vec4 worldPos = worldFromObject * vec4(a_Position, 1.0);
  const vec4 viewPos = u_FrameData.viewFromWorld * worldPos;
  const vec4 projPos = u_FrameData.projectionFromView * viewPos;

  const vec2 ndc = vec2(projPos.xy / projPos.w);
  const float r = length(ndc);
  const float distFactor = 1 - u_FrameData.fishEyeStrength * (r * r);
  const vec2 ndcFish = ndc * distFactor;
  const vec4 fishEyePos = vec4(ndcFish * projPos.w, projPos.zw);

  v_WorldPosition = vec3(worldPos);
  v_Normal = a_Normal;
  v_TexCoord1 = a_TexCoord1;

  gl_Position = fishEyePos;
}