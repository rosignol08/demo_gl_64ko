#version 330

layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec4 vsiColor;
layout (location = 2) in vec3 vsiNormal; // Normales pour le sol

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform float temps;

out vec4 vsoColor;
out vec3 vsoPosition;
out vec3 vsoNormal;
out float vsoDepth;

void main(void) {
  vec4 pos = modelViewMatrix * vec4(vsiPosition, 1.0);
  vsoPosition = pos.xyz;
  if (length(vsiNormal) > 0.0) {
    vsoNormal = normalize(mat3(modelViewMatrix) * vsiNormal);
  } else {
  vec3 seed = vsiPosition;
  float t = temps;
  vsoNormal = normalize(vec3(
    sin(seed.x * 10.0 + t),
    cos(seed.y * 10.0 + t * 0.5),
    sin(seed.z * 10.0 + t * 0.7)
  ));
  }
  vsoDepth = -pos.z / 5.0; // Normaliser la profondeur pour l'éclairage
  gl_Position = projectionMatrix * pos;
  vsoColor = vsiColor;
}