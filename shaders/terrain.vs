#version 330 core

layout(location = 0) in vec3 vsiPosition;
layout(location = 1) in vec3 vsiNormal;
layout(location = 2) in vec2 vsiTexCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 vsoNormal;
out vec3 vsoPosition;
out vec2 vsoTexCoord;

void main() {
    mat4 mvMatrix = viewMatrix * modelMatrix;
    vec4 vsiPosition4 = vec4(vsiPosition, 1.0);
    gl_Position = projectionMatrix * mvMatrix * vsiPosition4;
    vsoNormal = mat3(transpose(inverse(mvMatrix))) * vsiNormal;
    vsoPosition = vec3(mvMatrix * vsiPosition4);
    vsoTexCoord = vsiTexCoord;
}
