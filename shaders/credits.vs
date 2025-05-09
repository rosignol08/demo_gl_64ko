#version 330

layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;
uniform int inv; 
uniform float time;
uniform mat4 modelViewMatrix, projectionMatrix;
out vec2 vsoTexCoord;
out float vsoTime;

void main(void) {
  // Position de base
  vec3 pos = vsiPosition;
  
  // Position standard sans animation
  gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
  
  // Coordonnées de texture simples sans distorsion
  vec2 texCoord;
  if(inv != 0)
    texCoord = vec2(vsiTexCoord.s, 1.0 - vsiTexCoord.t);
  else
    texCoord = vec2(vsiTexCoord.s, vsiTexCoord.t);
  
  // Pas d'effet d'ondulation
  vsoTexCoord = texCoord;
  vsoTime = time;
}
