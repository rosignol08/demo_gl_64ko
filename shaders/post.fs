#version 330 core

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D screenTexture;
uniform float time;
uniform vec2 resolution;
uniform int effect; // 0=normal, 1=noir et blanc, 2=vignette, 3=aberration chromatique, 4=scanlines, 5=bloom, 6=flare et bloom

uniform vec2 lightPositions[3]; // Jusqu'à 3 positions de flares
uniform int numLights; // Nombre réel de lumières à utiliser

//const int effect = 2;

// Fonction pour extraire les zones lumineuses
vec3 extractBright(vec3 color, float threshold) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return color * smoothstep(threshold, threshold + 0.2, brightness);
}

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
vec4 bloomImproved(sampler2D tex, vec2 texCoord, float threshold) {
    vec4 color = texture(tex, texCoord);
    
    // Extraire seulement les parties brillantes
    vec3 brightPass = extractBright(color.rgb, threshold);
    
    // Application d'un flou gaussien
    vec3 blur = vec3(0.0);
    float blurSize = 0.006; // Taille du flou augmentée
    float total = 0.0;
    
    // Flou gaussien 9x9 avec poids variables selon la distance
    for(float x = -4.0; x <= 4.0; x++) {
        for(float y = -4.0; y <= 4.0; y++) {
            // Poids gaussien qui diminue avec la distance
            float weight = exp(-(x*x + y*y) / 8.0);
            vec2 offset = vec2(x, y) * blurSize;
            blur += texture(tex, texCoord + offset).rgb * weight;
            total += weight;
        }
    }
    
    blur /= total;
    
    // Appliquer l'effet bloom en ajoutant le flou des parties brillantes
    vec3 bloomEffect = blur * 1.0; // Intensité du bloom
    
    // Combiner avec l'original, en préservant les couleurs
    return vec4(color.rgb + bloomEffect * brightPass, color.a);
}


// Fonction simple pour dessiner un flare
vec3 simpleFlare(vec2 uv, vec2 pos) {
    // Calcul de la distance au centre du flare
    float dist = length(uv - pos);
    
    // Halo principal
    float halo = 0.5 / (dist * 16.0 + 0.01);
    
    // Quelques rayons
    float rays = pow(max(0.0, 0.5 - dist), 2.0) * 0.15;
    rays *= (sin(atan(uv.y - pos.y, uv.x - pos.x) * 8.0) * 0.5 + 0.5);
    
    // Combinaison du halo et des rayons avec une couleur de base jaune-orangée
    return (halo + rays) * vec3(1.0, 0.7, 0.3);
}

// Fonction pour créer des flares à partir de plusieurs sources lumineuses
vec4 multiLensFlare(sampler2D tex, vec2 texCoord) {
    // Image originale
    vec4 color = texture(tex, texCoord);
    
    // Coordonnées normalisées centrées
    vec2 uv = texCoord - 0.5;
    uv.x *= resolution.x / resolution.y; //correction de l'aspect ratio
    
    // Initialiser le flare total
    vec3 flare = vec3(0.0);
    
    // Boucler sur toutes les lumières actives
    for (int i = 0; i < numLights; i++) {
        vec2 pos = lightPositions[i] - 0.5;
        pos.x *= resolution.x / resolution.y;
        flare += simpleFlare(uv, pos);
    }
    
    // Intensité globale des flares
    flare *= 1.5; //l'intensité des flares
    
    // Ajouter le bloom simple
    vec3 brightPass = extractBright(color.rgb, 0.6);
    
    // Retourner l'image originale avec les flares
    return vec4(color.rgb + flare + brightPass * 0.5, color.a);
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
        // Bloom amélioré
        fragColor = bloomImproved(screenTexture, texCoords, 0.6);
    }
    else if (effect == 6) {
        // Bloom + Lens Flare combinés
        fragColor = lensFlareWithPosition(screenTexture, texCoords);
    }
    else {
        // Pas d'effet
        fragColor = texColor;
    }
}