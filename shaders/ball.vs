#version 330

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

out vec3 normal;
out vec3 fragPos;
out vec2 texCoord;

void main() {
  mat4 modelView = view * model;
  gl_Position = projection * modelView * vec4(vPosition, 1.0);
  
  // Transform normal to world space
  normal = mat3(transpose(inverse(model))) * vNormal;
  
  // Pass fragment position in world space
  fragPos = vec3(model * vec4(vPosition, 1.0));
  
  // Pass texture coordinates
  texCoord = vTexCoord;
}