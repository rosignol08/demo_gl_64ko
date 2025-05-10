#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

uniform float waveStrength;
uniform float waveSpeed;
uniform float time;
uniform float movementFactor; // Facteur de vitesse pour le mouvement
uniform float amplFactor;     // Facteur d'amplitude

out vec3 normal;
out vec3 fragPos;
out vec2 texCoord;

void main() {
    // No transformations on position
    vec3 pos = vPosition;
    
    // Simply pass through the vertex data
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(pos, 1.0);

    // Pass through normals
    normal = vNormal;

    // Pass through fragment position
    fragPos = vec3(modelMatrix * vec4(pos, 1.0));

    // Pass through texture coordinates
    texCoord = vTexCoord;
}
