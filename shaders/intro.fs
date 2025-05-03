#version 330 core

// Couleur uniforme définie dans le code C
uniform vec4 color;

// Couleur de sortie
out vec4 fragColor;

void main() {
    fragColor = color;
}