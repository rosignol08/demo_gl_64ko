#version 330 core

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D bloomTexture;  
uniform vec2 resolution;
uniform int horizontal;

void main()
{
    float radius = 2.0;
    vec3 bloomTotal = vec3(0.0);
    float totalWeight = 0.0;

    if(horizontal == 1) {
        // Flou horizontal
        for (int x = -7; x <= 7; x++) {
            float dist = abs(float(x));
            float weight = exp(-dist * dist * 0.1);
            vec2 offset = vec2(x * radius / resolution.x, 0.0);
            bloomTotal += texture(bloomTexture, texCoords + offset).rgb * weight;
            totalWeight += weight;
        }
    } else {
        // Flou vertical
        for (int y = -7; y <= 7; y++) {
            float dist = abs(float(y));
            float weight = exp(-dist * dist * 0.1);
            vec2 offset = vec2(0.0, y * radius / resolution.y);
            bloomTotal += texture(bloomTexture, texCoords + offset).rgb * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.0) {
        bloomTotal /= totalWeight;
    }

    fragColor = vec4(bloomTotal, 1.0);
}
