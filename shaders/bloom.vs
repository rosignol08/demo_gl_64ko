#version 330 core
layout(location = 0) in vec3 aPos;      // Même nom qu'avec gl4dgGenQuadf
layout(location = 2) in vec2 aTexCoords; // Location 2 au lieu de 1

out vec2 TexCoords;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}