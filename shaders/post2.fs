#version 330 core
in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D bloomTexture;
uniform vec2 resolution;
uniform int horizontal; // 1 = horizontal, 0 = vertical

void main() {
    float bloomRadius = 10.0;
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = -7; i <= 7; ++i) {
        float dist = abs(float(i));
        float weight = exp(-dist * dist * 0.05);

        vec2 offset = horizontal == 1
            ? vec2(i, 0.0) * bloomRadius / resolution
            : vec2(0.0, i) * bloomRadius / resolution;

        result += texture(bloomTexture, texCoords + offset).rgb * weight;
        totalWeight += weight;
    }
    result /= totalWeight;
    fragColor = vec4(result, 1.0);
}