#version 330 core

// Position d'entrée du vertex shader
in vec2 position;

// Matrices uniformes de transformation
uniform mat4 modelview;
uniform mat4 proj;

void main() {
    // Appliquer les transformations (projection et modelview)
    gl_Position = proj * modelview * vec4(position, 0.0, 1.0);
}