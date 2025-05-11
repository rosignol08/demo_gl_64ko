#version 330 core

in vec2 texCoords;
//out vec4 fragColor;
//out
layout(location = 0) out vec4 fragColor; // Couleur finale du fragment
layout(location = 1) out vec4 BrightColor; // Couleur pour le bloom

uniform sampler2D screenTexture;
uniform sampler2D bloomTexture; // Texture pour le bloom
uniform float time;
uniform vec2 resolution;

uniform int numLights; // Nombre réel de lumières à utiliser

void main() {
    //vec4 texColor = texture(screenTexture, texCoords);
    
    // Optimized Bloom
    vec4 baseColor = texture(screenTexture, texCoords);

    vec3 bloomTotal = vec3(0.0);
    float totalWeight = 0.0;

    float bloomStrength = 3.5;
    float bloomRadius = 2.0; // un peu plus large car moins d'échantillons
    int echantillons = 7;

    for (int x = -echantillons; x <= echantillons; x+= 1) {
        for (int y = -echantillons; y <= echantillons; y+= 1) {
            float dist = length(vec2(x, y));
            if (dist > echantillons) continue;

            //float weight = exp(-dist * dist * 0.002); // un peu plus serré pour compenser
                    float weight = 1.0 / (1.0 + dist * dist * 0.02);

            vec2 offset = vec2(x, y) * bloomRadius / resolution;

            bloomTotal += texture(bloomTexture, texCoords + offset).rgb * weight;
            totalWeight += weight;
        }
    }

    bloomTotal /= totalWeight;
    bloomTotal *= bloomStrength;

    vec3 result = baseColor.rgb + bloomTotal;
    result = result / (result + vec3(1.0)); // Simple tonemap
    fragColor = vec4(result, 1.0);

    BrightColor = vec4(0.0);
}