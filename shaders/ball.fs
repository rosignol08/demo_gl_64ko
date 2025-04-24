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
uniform vec4 secondLightPosition;     // Position/Direction de la lumière (W=1 pour positionnelle, W=0 pour directionnelle)
uniform int useSecondLight;           // 1 pour activer cette lumière, 0 sinon
uniform int secondLightType;          // 0 = directional, 1 = positional

uniform vec4 thirdLightColor;         // Couleur de la troisième lumière
uniform vec4 thirdLightPosition;      // Position/Direction de la lumière (W=1 pour positionnelle, W=0 pour directionnelle)
uniform int useThirdLight;            // 1 pour activer cette lumière, 0 sinon
uniform int thirdLightType;           // 0 = directional, 1 = positional

out vec4 fragColor;

void main() {
  // Normalize the normal vector
  vec3 norm = normalize(normal);
  
  // Calculate light direction for first light
  vec3 lightDir;
  float attenuation = 1.0;
  
  if (lightType == 1) { // Positional light
    // Calculer la direction depuis le fragment vers la source lumineuse
    lightDir = normalize(vec3(lightPosition) - fragPos);
    
    // Ajouter l'atténuation basée sur la distance
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
  float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess * 2.0);
  vec3 specular = 1.0 * spec * vec3(lightColor);
  
  diffuse *= attenuation;
  specular *= attenuation;
  
  // Ajout d'un effet de fresnel simple pour les bords
  float rim = 1.0 - max(dot(viewDir, norm), 0.0);
  rim = pow(rim, 3.0);
  vec3 rimLight = rim * vec3(lightColor) * 0.3 * attenuation; // Effet de contour lumineux subtil
  
  // Ajout de la seconde lumière (directionnelle OU positionnelle)
  vec3 secondaryLightContribution = vec3(0.0);
  
  if (useSecondLight == 1) {
    vec3 secondLightDir;
    float secondAttenuation = 1.0;
    
    if (secondLightType == 1) { // Positional light
      // Direction depuis le fragment vers la source lumineuse
      secondLightDir = normalize(vec3(secondLightPosition) - fragPos);
      
      // Atténuation basée sur la distance pour lumière positionnelle
      float secondDistance = length(vec3(secondLightPosition) - fragPos);
      secondAttenuation = 1.0 / (1.0 + 0.09 * secondDistance + 0.032 * secondDistance * secondDistance);
    }
    else { // Directional light
      // Direction constante pour tous les fragments
      secondLightDir = normalize(-vec3(secondLightPosition));
      // Pas d'atténuation pour lumière directionnelle
    }
    
    // Composante diffuse de la seconde lumière
    float secondDiff = max(dot(norm, secondLightDir), 0.0);
    vec3 secondDiffuse = secondDiff * vec3(secondLightColor);
    
    // Composante spéculaire de la seconde lumière
    vec3 secondHalfwayDir = normalize(secondLightDir + viewDir);
    float secondSpec = pow(max(dot(norm, secondHalfwayDir), 0.0), shininess * 2.0);
    vec3 secondSpecular = secondSpec * vec3(secondLightColor);
    
    // Appliquer l'atténuation aux composantes diffuse et spéculaire
    secondDiffuse *= secondAttenuation;
    secondSpecular *= secondAttenuation;
    
    // Ajouter la contribution de la seconde lumière
    secondaryLightContribution = secondDiffuse + secondSpecular;
  }
  
  // Ajout de la troisième lumière
  vec3 thirdLightContribution = vec3(0.0);
  
  if (useThirdLight == 1) {
    vec3 thirdLightDir;
    float thirdAttenuation = 1.0;
    
    if (thirdLightType == 1) { // Positional light
      thirdLightDir = normalize(vec3(thirdLightPosition) - fragPos);
      
      float thirdDistance = length(vec3(thirdLightPosition) - fragPos);
      thirdAttenuation = 1.0 / (1.0 + 0.09 * thirdDistance + 0.032 * thirdDistance * thirdDistance);
    } 
    else { // Directional light
      thirdLightDir = normalize(-vec3(thirdLightPosition));
    }
    
    // Composante diffuse
    float thirdDiff = max(dot(norm, thirdLightDir), 0.0);
    vec3 thirdDiffuse = thirdDiff * vec3(thirdLightColor);
    
    // Composante spéculaire
    vec3 thirdHalfwayDir = normalize(thirdLightDir + viewDir);
    float thirdSpec = pow(max(dot(norm, thirdHalfwayDir), 0.0), shininess * 2.0);
    vec3 thirdSpecular = thirdSpec * vec3(thirdLightColor);
    
    // Appliquer atténuation
    thirdDiffuse *= thirdAttenuation;
    thirdSpecular *= thirdAttenuation;
    
    thirdLightContribution = thirdDiffuse + thirdSpecular;
  }
  
  if (isEmissive == 1) {
    // Si l'objet est émissif, ignorer l'éclairage normal 
    // et utiliser directement sa couleur comme source lumineuse
    fragColor = vec4(vec3(ballColor) * 1.5, ballColor.a);
  }
  else {
    // Code normal pour les objets non-émissifs
    vec3 result = (ambient + diffuse + specular + rimLight + secondaryLightContribution + thirdLightContribution) * vec3(ballColor);
    fragColor = vec4(result, ballColor.a);
  }
}