#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 texCoords;  // Changed to match the fragment shader input

uniform vec2 resolution;  // Added to match the fragment shader uniform

void main() {
    texCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}