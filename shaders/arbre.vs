#version 330

layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec4 vsiColor;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

out vec4 vsoColor;
out vec3 vsoPosition;
out float vsoDepth;

void main(void) {
  vec4 pos = modelViewMatrix * vec4(vsiPosition, 1.0);
  vsoPosition = pos.xyz;
  vsoDepth = -pos.z / 5.0; // Normaliser la profondeur pour l'éclairage
  gl_Position = projectionMatrix * pos;
  vsoColor = vsiColor;
}