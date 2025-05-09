#version 330

in vec4 vsoColor;
in vec3 vsoPosition;
in vec3 vsoNormal;
in float vsoDepth;

uniform float time;
uniform vec3 lightDir;
uniform int season; // 0=été, 1=automne, 2=hiver
uniform vec4 objectColor; // New uniform for object color

out vec4 fragColor;

//pour l'ombre
uniform vec3 lightPos;
uniform mat4 modelViewMatrix;

// Fonction pour ajouter un léger grain/texture
float noise(vec3 pos) {
    return fract(sin(dot(pos, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

void main(void) {
    // Normaliser la direction de lumière depuis l'uniform
    vec3 lightDirection = normalize(lightDir);
    
    // Utiliser la normale du vertex
    vec3 normal = normalize(vsoNormal);
    
    // Calcul de l'éclairage directionnel simple
    float diffuseLight = max(dot(normal, lightDirection), 0.0);
    float ambientLight = 0.3; // Lumière ambiante minimum
    
    // Facteur d'éclairage total
    float lightFactor = ambientLight + diffuseLight * 0.7;
    
    // Utiliser la couleur de l'objet directement depuis l'uniform
    vec3 finalColor = objectColor.rgb;
    
    // Appliquer l'éclairage à la couleur
    finalColor *= lightFactor;
    
    // Couleur finale
    fragColor = vec4(finalColor, objectColor.a);
}
