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
uniform int isLeaf; // 0=tronc, 1=feuille
// Fonction pour ajouter un léger grain/texture
float noise(vec3 pos) {
    return fract(sin(dot(pos, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}


void main(void) {
    // Récupérer la couleur de base du vertex
    vec4 baseColor = vsoColor;
    
    // Normaliser la direction de lumière depuis l'uniform
    vec3 lightDirection = normalize(lightDir);
    
    // Utiliser la normale du vertex
    vec3 normal = normalize(vsoNormal);
    
    // Calcul de l'éclairage directionnel simple
    float diffuseLight = max(dot(normal, lightDirection), 0.0);
    float ambientLight = 0.3; // Lumière ambiante minimum
    
    // Facteur d'éclairage total
    float lightFactor = ambientLight + diffuseLight * 0.7;
    
    // ===== COULEURS ROSES POUR LES FEUILLES =====
    // Couleurs roses qui changent légèrement avec le temps
    vec3 pinkColor1 = vec3(1.0, 0.718, 0.773); // Rose principal
    vec3 pinkColor2 = vec3(1.0, 0.6, 0.7);     // Rose légèrement plus foncé

    // Transition entre les couleurs roses
    float colorMix = sin(time * 0.2) * 0.5 + 0.5;
    vec3 leafColor = mix(pinkColor1, pinkColor2, colorMix);
    
    // Couleur finale avec éclairage simple
    vec3 finalColor = baseColor.rgb;
    
    // ===== APPLICATION DES COULEURS AUX FEUILLES =====
    if (isLeaf == 1) { // Si c'est une feuille
        // Remplacer par la couleur rose
        finalColor = leafColor;
    }else {
        // Couleur du tronc (par exemple, marron)
        finalColor = vec3(0.6, 0.4, 0.2); // Couleur marron pour le tronc
    }
    
    // Appliquer l'éclairage de manière uniforme
    finalColor *= lightFactor;
    
    // Couleur finale
    fragColor = vec4(finalColor, baseColor.a);
}
