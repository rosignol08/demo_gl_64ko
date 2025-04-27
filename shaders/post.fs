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
uniform int effect; // 0=normal, 1=noir et blanc, 2=vignette, 3=aberration chromatique, 4=scanlines, 5=bloom, 6=flare et bloom

uniform vec2 flarePosition1; // Jusqu'à 3 positions de flares
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

// Fonction équivalente à bloomTile mais adaptée à notre cas
// Applique différents niveaux de flou avec des décalages
vec3 bloomLevel(sampler2D tex, float lod, vec2 offset, float threshold) {
    float scale = exp2(-lod);
    vec2 uv = texCoords * scale + offset;
    return extractBright(texture(tex, uv).rgb, threshold);
}

// Bloom réaliste avec plusieurs niveaux de flou
vec4 bloomImproved(sampler2D tex, vec2 texCoord, float threshold) {
    vec4 color = texture(tex, texCoord);
    vec3 bloom = vec3(0.0);
    // Combine plusieurs niveaux de bloom avec des intensités et offsets différents
    bloom += bloomLevel(tex, 2.0, vec2(0.0, 0.0), threshold) * 1.0;
    bloom += bloomLevel(tex, 3.0, vec2(0.3, 0.0), threshold) * 1.3;
    bloom += bloomLevel(tex, 4.0, vec2(0.0, 0.3), threshold) * 1.6;
    bloom += bloomLevel(tex, 5.0, vec2(0.1, 0.3), threshold) * 1.9;
    bloom += bloomLevel(tex, 6.0, vec2(0.2, 0.3), threshold) * 2.2;
    
    // Tone mapping inspiré de Reinhard
    vec3 result = color.rgb;
    
    // Ajoute le bloom à l'image originale
    result += bloom * 1.5; // Intensité ajustable
    
    // Tone mapping simple pour éviter la saturation
    float luminance = dot(result, vec3(0.2126, 0.7152, 0.0722));
    vec3 tonemapped = result / (result + 1.0);
    result = mix(result / (luminance + 1.0), tonemapped, tonemapped);
    
    return vec4(result, color.a);
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

// Nouvelle fonction qui utilise la position passée directement
vec4 lensFlareWithPosition(sampler2D tex, vec2 texCoord) {
    // Image originale
    vec4 color = texture(tex, texCoord);
    
    // Coordonnées normalisées centrées
    vec2 uv = texCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Correction aspect ratio
    
    // Position du flare (convertie de coordonnées [0-1] à centrées [-0.5,0.5])
    vec2 flarePos = flarePosition1 - 0.5;
    flarePos.x *= resolution.x / resolution.y; // Correction aspect ratio
    
    // Créer le flare
    vec3 flare = simpleFlare(uv, flarePos) * 2.0; // Intensité x2
    
    // Ajouter le bloom simple
    vec3 brightPass = extractBright(color.rgb, 0.6);
    
    // Retourner l'image originale avec le flare
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
        fragColor = bloomImproved(screenTexture, texCoords, 1.0);
    }
    else if (effect == 6) {
        // Bloom + Lens Flare combinés
        fragColor = lensFlareWithPosition(screenTexture, texCoords);
    }
    else if (effect == 7) {
        // flou gaussien
        // Appliquer un flou gaussien simple
        vec2 texOffset = 1.0 / resolution; // Offset pour le flou
        vec4 color = vec4(0.0);
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                vec2 offset = vec2(float(x), float(y)) * texOffset;
                color += texture(screenTexture, texCoords + offset) * 0.11111; // Coefficient pour le flou
            }
        }
        fragColor = color;
    }
    else if (effect == 8) { // Optimized Bloom
    vec4 baseColor = texture(screenTexture, texCoords);

    vec3 bloomTotal = vec3(0.0);
    float totalWeight = 0.0;

    float bloomStrength = 1.5;
    float bloomRadius = 10.0; // un peu plus large car moins d'échantillons

    for (int x = -7; x <= 7; x++) {
        for (int y = -7; y <= 7; y++) {
            float dist = length(vec2(x, y));
            if (dist > 7.0) continue;

            float weight = exp(-dist * dist * 0.05); // un peu plus serré pour compenser
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


    //else if (effect == 9) { //marche pas mais je met de coté
    //    fragColor = texColor;
    //    // Pas d'effet
    //    if (fragColor.r > 0.5){
    //        fragColor.r *= 5.0;
    //    }
    //    float brightness = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    //    if (brightness > 0.0){
    //        BrightColor = vec4(texColor.rgb, 1.0);
    //    }
    //    else{
    //        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    //    }
    //}
    else {
        // Effet normal
        fragColor = texColor;
    }
}