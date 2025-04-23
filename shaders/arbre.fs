#version 330

in vec4 vsoColor;
in vec3 vsoPosition;
in float vsoDepth;

uniform float time;
uniform int season; // 0=été, 1=automne, 2=hiver

out vec4 fragColor;

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
    
    // Détection des feuilles (basée sur la composante verte dominante)
    float isLeaf = step(0.2, baseColor.g - max(baseColor.r, baseColor.b));
    
    // ===== COULEURS VERTES POUR LES FEUILLES =====
    // Couleurs vertes naturelles qui changent légèrement avec le temps
    vec3 greenColor1 = vec3(0.2, 0.8, 0.1); // Vert clair
    vec3 greenColor2 = vec3(0.1, 0.6, 0.0); // Vert foncé
    
    // Transition entre les couleurs vertes
    float colorMix = sin(time * 0.2) * 0.5 + 0.5;
    vec3 leafColor = mix(greenColor1, greenColor2, colorMix);
    
    // ===== EFFET DE PULSATION AMÉLIORÉ =====
    // Pulsation légère pour donner vie aux feuilles
    float basePulse = sin(time * 1.0) * 0.1 + 0.9;
    float quickPulse = sin(time * 2.0) * 0.1 + 0.9;
    float leafPulse = basePulse * quickPulse;
    
    // ===== ÉCLAIRAGE DU TRONC PAR LES FEUILLES =====
    // Calculer l'illumination du tronc par les feuilles
    float trunkIllumination = 0.0;
    if (isLeaf < 0.5) { // Si c'est le tronc
        // Plus on est proche des feuilles (en Y), plus l'éclairage est fort
        float heightFactor = smoothstep(0.0, 0.8, vsoPosition.y);
        
        // Ajouter des variations temporelles et spatiales
        trunkIllumination = heightFactor * 0.1;
        
        // Légère illumination verte sur le tronc
        finalColor += leafColor * trunkIllumination;
    }
    
    // ===== APPLICATION DES COULEURS VERTES AUX FEUILLES =====
    if (isLeaf > 0.5) { // Si c'est une feuille
        // Remplacer par la couleur verte naturelle
        finalColor = leafColor * leafPulse;
        
        // Ajouter un peu de variation de texture
        float leafTexture = noise(vsoPosition * 10.0 + time * 0.05) * 0.15 + 0.85;
        finalColor *= leafTexture;
    }
    
    // ===== EFFETS DE LUMIÈRE DOUCE =====
    // Éclairage ambiant naturel
    float ambientGlow = sin(time * 0.1) * 0.05 + 1.0;
    
    // Légère variation de luminosité
    float flicker = noise(vec3(vsoPosition.xy * 2.0, time * 1.0)) * 0.1 + 0.9;
    
    // Application des effets lumineux
    finalColor *= ambientGlow * flicker;
    
    // Couleur finale avec tous les effets
    fragColor = vec4(finalColor, baseColor.a);
}
