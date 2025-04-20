#version 330

in vec4 vsoColor;
in vec3 vsoPosition;
in float vsoDepth;

uniform float time;
uniform int season; // 0=été, 1=automne, 2=hiver

out vec4 fragColor;

// Fonction pour ajouter un léger effet de wind
vec3 applyWind(vec3 pos, float time) {
    // Amplitude plus importante pour les parties élevées (feuilles et branches fines)
    float windStrength = 0.03 * max(0.0, (pos.y + 0.8) / 1.6);
    
    // Animation de vent légèrement chaotique
    float windX = sin(time * 1.5 + pos.y * 2.0) * windStrength;
    float windY = cos(time * 0.7 + pos.x * 3.0) * windStrength * 0.3;
    
    return vec3(pos.x + windX, pos.y + windY, pos.z);
}

// Fonction pour ajouter un léger grain/texture
float noise(vec3 pos) {
    return fract(sin(dot(pos, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

void main(void) {
    // Récupérer la couleur de base du vertex
    vec4 baseColor = vsoColor;
    
    // Effet de profondeur (ombres légères)
    float depthFactor = mix(0.7, 1.0, vsoDepth);
    
    // Effet de grain/texture naturelle
    float grainNoise = noise(vsoPosition * 50.0) * 0.1;
    
    // Couleur finale avec effet de profondeur et grain
    vec3 finalColor = baseColor.rgb * depthFactor + vec3(grainNoise);
    
    // Ajout d'une légère teinte selon la saison
    if (season == 0) {
        // Été: légère teinte verte
        finalColor += vec3(0.0, 0.05, 0.0);
    } else if (season == 1) {
        // Automne: légère teinte orangée
        finalColor += vec3(0.05, 0.02, 0.0);
    } else if (season == 2) {
        // Hiver: légère teinte bleue/grise
        finalColor = mix(finalColor, vec3(0.6, 0.7, 0.8), 0.1);
    }
    
    // Luminosité variable avec le temps pour simuler les nuages
    float ambientVariation = sin(time * 0.2) * 0.05 + 0.95;
    
    // Couleur finale avec tous les effets
    fragColor = vec4(finalColor * ambientVariation, baseColor.a);
}