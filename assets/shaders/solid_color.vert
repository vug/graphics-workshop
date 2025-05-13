#version 460

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord1;

layout(std140, binding = 0) uniform PerFrameData {
    mat4 viewFromWorld;
    mat4 projectionFromView;
    float fishEyeStrength;
} u_FrameData;

layout(std140, binding = 1) uniform PerObjectData {
    mat4 worldFromObject;
} u_ObjectData;

layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord1;

void main() {
  const vec4 worldPos = u_ObjectData.worldFromObject * vec4(a_Position, 1.0);
  const vec4 viewPos = u_FrameData.viewFromWorld * worldPos;
  const vec4 projPos = u_FrameData.projectionFromView * viewPos;

  const vec2 ndc = vec2(projPos.xy / projPos.w);

  const float r = length(ndc);
  const float theta = atan(ndc.y, ndc.x);
  const float fishR = tanh(u_FrameData.fishEyeStrength * 3.14159265 * r);
  const vec2 ndcFish = vec2(cos(theta), sin(theta)) * fishR;

  // const float distFactor = u_FrameData.fishEyeStrength * sqrt(1 - min(r * r, 1));
  // const vec2 ndcFish = ndc * distFactor;

  //const vec2 ndcFish = ndc + vec2(u_FrameData.fishEyeStrength, 0.0); // just shift right

  const vec4 fishEyePos = vec4(ndcFish * projPos.w, projPos.zw);

  v_WorldPosition = vec3(worldPos);
  v_Normal = a_Normal;
  v_TexCoord1 = a_TexCoord1;

  gl_Position = fishEyePos;
}