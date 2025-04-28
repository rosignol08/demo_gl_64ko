#version 330

in vec4 vsoColor;
in vec3 vsoPosition;
in vec3 vsoNormal;
in float vsoDepth;

uniform float time;
uniform vec3 lightDir;
uniform int season; // 0=été, 1=automne, 2=hiver

out vec4 fragColor;

//pour l'ombre
uniform vec3 lightPos;
uniform mat4 modelViewMatrix;
uniform int isFloor; // 1 pour le sol, 0 pour l'arbre
// Fonction pour ajouter un léger grain/texture
float noise(vec3 pos) {
    return fract(sin(dot(pos, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}
// Ajouter cette fonction pour calculer l'ombre sur le sol
float calculateShadow(vec3 pos) {
    // Distance entre le point du sol et l'arbre
    float distanceToCenter = length(vec2(pos.x, pos.z));
    
    // Projection simple de l'ombre basée sur la direction de la lumière
    vec3 shadowDirection = normalize(vec3(-lightDir.x, 0.0, -lightDir.z));
    
    // Calculer l'ombre en fonction de la distance et de la position
    float shadow = 0.0;
    
    // Créer un effet d'ombre douce et diffuse
    shadow = smoothstep(0.0, 1.0, 0.6 - distanceToCenter);
    
    // Ajouter de la variation basée sur la position
    shadow *= 0.8 - 0.3 * noise(pos * 10.0 + vec3(time * 0.2, 0, 0));
    
    // Ajuster l'ombre en fonction de la direction de la lumière
    float lightFactor = dot(normalize(pos), shadowDirection) * 0.3 + 0.7;
    shadow *= lightFactor;
    
    return clamp(shadow, 0.0, 0.9);
}

void main(void) {
    // Récupérer la couleur de base du vertex
    vec4 baseColor = vsoColor;
    // Traitement différent selon que c'est le sol ou l'arbre
    if (isFloor == 1) {
        // Calcul de l'ombre pour le sol
        float shadow = calculateShadow(vsoPosition);
        
        // Ajouter de la texture au sol (herbe)
        float grassNoise = noise(vsoPosition * 30.0) * 0.1;
        vec3 grassColor = baseColor.rgb + vec3(grassNoise) * vec3(0.0, 0.2, 0.0);
        
        // Ajouter des variations subtiles
        float variation = noise(vsoPosition * 5.0) * 0.15;
        grassColor += vec3(variation * 0.1, variation * 0.2, 0.0);
        
        // Assombrir le sol avec l'ombre
        vec3 finalColor = grassColor * (1.0 - shadow * 0.7);
        
        // Ajouter un léger dégradé sur les bords
        float edgeFactor = 1.0 - smoothstep(0.7, 1.4, length(vsoPosition.xz));
        finalColor *= mix(0.8, 1.0, edgeFactor);
        
        fragColor = vec4(finalColor, 1.0);
        return;
    }
    // Normaliser la direction de lumière
    vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.8)); // Par défaut si uniform non défini
    
    // Calcul d'une normale approximative basée sur la position
    vec3 normal = normalize(cross(dFdx(vsoPosition), dFdy(vsoPosition)));
    
    // Calcul de l'éclairage directionnel
    float diffuseLight = max(dot(normal, lightDirection), 0.0);
    float ambientLight = 0.3; // Lumière ambiante minimum
    
    // Facteur d'éclairage total
    float lightFactor = ambientLight + diffuseLight * 0.7;
    
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
    
    // ===== CALCUL D'UNE OMBRE DOUCE =====
    float shadowFactor = 1.0;
    if (isLeaf < 0.5) { // Pour le tronc
        // Les parties du tronc qui font face à la lumière sont plus éclairées
        shadowFactor = mix(0.6, 1.0, diffuseLight);
    }
    else { // Pour les feuilles
        // Variation d'ombre basée sur la position et le mouvement des feuilles
        float leafShadow = noise(vsoPosition * 5.0 + time * 0.1) * 0.3 + 0.7;
        shadowFactor = mix(leafShadow, 1.0, diffuseLight);
    }
    
    // Appliquer le facteur d'ombre
    finalColor *= shadowFactor;
    
    // Couleur finale avec tous les effets
    fragColor = vec4(finalColor, baseColor.a);
}
