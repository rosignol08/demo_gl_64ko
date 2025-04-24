#version 330

in vec3 normal;
in vec3 fragPos;
in vec2 texCoord;

uniform vec4 ballColor;
uniform vec4 lightColor;
uniform vec4 lightPosition;
uniform vec4 ambientColor;
uniform float shininess;
uniform int isEmissive;
uniform int lightType; // 0 = directional, 1 = positional


uniform vec4 secondLightColor;        // Couleur de la seconde lumière
uniform vec4 secondLightDirection;    // Direction de la lumière (W=0 pour directionnelle)
uniform int useSecondLight;           // 1 pour activer cette lumière, 0 sinon



out vec4 fragColor;

void main() {
  // Normalize the normal vector
  vec3 norm = normalize(normal);
  
  // Calculate light direction
  vec3 lightDir;

  float attenuation = 1.0;
  if (lightType == 1) { // Positional light
    // Calculer la direction depuis le fragment vers la source lumineuse
    lightDir = normalize(vec3(lightPosition) - fragPos);
    
    // Optionnel: ajouter l'atténuation basée sur la distance
    float distance = length(vec3(lightPosition) - fragPos);
    attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
  }
  else { // Directional light
    // Direction constante pour tous les fragments
    lightDir = normalize(-vec3(lightPosition));
    // Pas d'atténuation pour une lumière directionnelle
  }
  // Ambient component
  vec3 ambient = vec3(ambientColor);
  
  // Diffuse component
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * vec3(lightColor);
  
  // Specular component - MODIFIÉE POUR BLINN-PHONG
  vec3 viewDir = normalize(vec3(0.0, 0.0, 5.0) - fragPos); // Direction de la caméra
  
  // Calcul du vecteur "halfway" - spécifique à Blinn-Phong
  vec3 halfwayDir = normalize(lightDir + viewDir);
  
  // Utilisation du produit scalaire avec le vecteur "halfway" au lieu du vecteur de réflexion
  float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess * 2.0); // Note: souvent on double la valeur de shininess avec Blinn-Phong
  vec3 specular = 1.0 * spec * vec3(lightColor);
  
  diffuse *= attenuation;
  specular *= attenuation;
  
  // Ajout d'un effet de fresnel simple pour les bords
  float rim = 1.0 - max(dot(viewDir, norm), 0.0);
  rim = pow(rim, 3.0);
  vec3 rimLight = rim * vec3(lightColor) * 0.3; // Effet de contour lumineux subtil
  // Ajout de la seconde lumière (directionnelle)
  vec3 secondaryLightContribution = vec3(0.0);
  
  if (useSecondLight == 1) {
    // Direction fixe pour la lumière directionnelle
    vec3 secondLightDir = normalize(-vec3(secondLightDirection));
    
    // Composante diffuse de la seconde lumière
    float secondDiff = max(dot(norm, secondLightDir), 0.0);
    vec3 secondDiffuse = secondDiff * vec3(secondLightColor);
    
    // Composante spéculaire de la seconde lumière
    vec3 secondHalfwayDir = normalize(secondLightDir + viewDir);
    float secondSpec = pow(max(dot(norm, secondHalfwayDir), 0.0), shininess * 2.0);
    vec3 secondSpecular = secondSpec * vec3(secondLightColor);
    
    // Ajouter la contribution de la seconde lumière
    secondaryLightContribution = secondDiffuse + secondSpecular;
  }
  if (isEmissive == 1) {
    // Si l'objet est émissif, ignorer l'éclairage normal 
    // et utiliser directement sa couleur comme source lumineuse
    fragColor = vec4(vec3(ballColor) * 1.5, ballColor.a);
  }
  else {
    // Code normal pour les objets non-émissifs
    vec3 result = (ambient + diffuse + specular + rimLight + secondaryLightContribution) * vec3(ballColor);
    fragColor = vec4(result, ballColor.a);
  }
  // Combine all lighting components
  //vec3 result = (ambient + diffuse + specular + rimLight) * vec3(ballColor);
  //fragColor = vec4(result, ballColor.a);
}