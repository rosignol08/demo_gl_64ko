#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;

void main() {
    vec3 color = texture(sceneTexture, TexCoords).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Capture toutes les couleurs suffisamment lumineuses
    // Possible aussi d'utiliser max(max(color.r, color.g), color.b) > 1.0
    if (brightness > 0.0)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}