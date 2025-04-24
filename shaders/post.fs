#version 330 core

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D screenTexture;
uniform float time;
uniform vec2 resolution;
uniform int effect; // 0=normal, 1=noir et blanc, 2=vignette, 3=aberration chromatique, 4=scanlines
// Effet à appliquer: 0=normal, 1=noir et blanc, 2=vignette, 3=aberration chromatique
//const int effect = 2;

// Effet noir et blanc
vec4 blackAndWhite(vec4 color) {
    float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    return vec4(average, average, average, 1.0);
}

// Effet vignette
vec4 vignette(vec4 color, vec2 texCoord) {
    float dist = distance(texCoord, vec2(0.5, 0.5));
    float vignette = smoothstep(0.5, 0.2, dist);
    return color * vignette;
}

// Effet aberration chromatique
vec4 chromaticAberration(sampler2D tex, vec2 texCoord) {
    float amount = 0.005;
    vec2 offset = amount * (texCoord - 0.5);
    
    float r = texture(tex, texCoord - offset).r;
    float g = texture(tex, texCoord).g;
    float b = texture(tex, texCoord + offset).b;
    
    return vec4(r, g, b, 1.0);
}

// Scanlines
vec4 scanlines(vec4 color, vec2 texCoord) {
    float scanline = sin(texCoord.y * 400.0) * 0.04 + 1.0;
    return color * scanline;
}

//bloom
vec4 bloom(sampler2D tex, vec2 texCoord, float threshold) {
    vec4 color = texture(tex, texCoord);
    
    // Extraire seulement les parties brillantes
    vec3 brightPass = max(color.rgb - threshold, 0.0);
    
    // Appliquer un flou simple 5x5
    vec3 blur = vec3(0.0);
    float blurSize = 0.004;
    
    for(float x = -2.0; x <= 2.0; x++) {
        for(float y = -2.0; y <= 2.0; y++) {
            vec2 offset = vec2(x, y) * blurSize;
            blur += texture(tex, texCoord + offset).rgb;
        }
    }
    
    blur /= 50.0; // Moyenne du flou 5x5
    
    // Ajouter le flou des parties brillantes à l'image originale
    return vec4(color.rgb + blur * 0.5, color.a);
}

void main() {
    vec4 texColor = texture(screenTexture, texCoords);
    
    if (effect == 1) {
        // Noir et blanc
        fragColor = blackAndWhite(texColor);
    } 
    else if (effect == 2) {
        // Vignette
        fragColor = vignette(texColor, texCoords);
    } 
    else if (effect == 3) {
        // Aberration chromatique
        fragColor = chromaticAberration(screenTexture, texCoords);
    }
    else if (effect == 4) {
        // Scanlines
        fragColor = scanlines(texColor, texCoords);
    }
    else if (effect == 5) {
        // Bloom
        fragColor = bloom(screenTexture, texCoords, 1.0);
    }
    else {
        // Pas d'effet
        fragColor = texColor;
    }
}