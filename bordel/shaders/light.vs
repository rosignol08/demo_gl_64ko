/*!\file light.vs
 * \brief vertex shader progressif sur l'éclairage */
#version 330
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;
uniform mat4 projectionMatrix, viewMatrix, modelMatrix;
out vec4 vmPos;
out vec3 vmNormal;
void main(void) {
  vmPos = viewMatrix * modelMatrix * vec4(vsiPosition, 1.0);
  vmNormal = (transpose(inverse(viewMatrix * modelMatrix)) * vec4(vsiNormal, 0.0)).xyz;
  gl_Position = projectionMatrix * vmPos;
}
