#version 330

in vec3 normal;
in vec3 fragPos;
in vec2 texCoord;

//deux outputs
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;


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

//out vec4 fragColor;
// Uniforms for water effect
uniform int utiliseWater;        // 1 pour activer l'effet d'eau, 0 sinon
uniform float time;           // Pour l'animation des vagues
uniform float waveStrength;   // Controls wave intensity
uniform float waveSpeed;      // Controls wave speed
uniform int isWater;          // Toggle water effect (1 = water, 0 = normal material)

// Function to generate procedural waves
vec3 applyWaves(vec3 position, vec3 normal) {
  if (isWater != 1) return position;
  
  float frequency = 5.0;
  float amplitude = waveStrength * 0.05;
  
  // Create several overlapping wave patterns
  float wave1 = sin(frequency * position.x + time * waveSpeed) * 
          cos(frequency * position.z + time * waveSpeed * 0.7) * amplitude;
          
  float wave2 = sin(frequency * 1.3 * position.z + time * waveSpeed * 0.8) * 
          cos(frequency * 1.7 * position.x + time * waveSpeed * 1.1) * amplitude * 0.8;
          
  // Displace position based on waves
  vec3 newPosition = position;
  newPosition.y += wave1 + wave2;
  
  return newPosition;
}

// Calculate normal for the wavy surface
vec3 calculateWaveNormal(vec3 pos) {
  // Calculate derivative of the wave function in x and z directions
  float epsilon = 0.01;
  vec3 dx = applyWaves(pos + vec3(epsilon, 0.0, 0.0), vec3(0.0, 1.0, 0.0)) - 
        applyWaves(pos - vec3(epsilon, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
  vec3 dz = applyWaves(pos + vec3(0.0, 0.0, epsilon), vec3(0.0, 1.0, 0.0)) - 
        applyWaves(pos - vec3(0.0, 0.0, epsilon), vec3(0.0, 1.0, 0.0));
  
  // Cross product to find normal
  return normalize(cross(dx, dz));
}
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
    FragColor = vec4(vec3(ballColor) * 1.5, ballColor.a);
  }
  else {
    if(utiliseWater == 1) {
      // Appliquer l'effet d'eau
      vec3 wavyPos = applyWaves(fragPos, norm);
      vec3 wavyNorm = calculateWaveNormal(wavyPos);
      norm = normalize(wavyNorm);

      // Paramètres de l'eau dynamique
      float waterDepth = sin(time * 0.5) * 0.1 + 0.9; // Variation de profondeur
      float waterRipple = sin(length(texCoord * 10.0) - time * 2.0) * 0.05; // Effet d'ondulation

      // Calcul d'éclairage avec paramètres dynamiques
      vec3 wavyLightDir = normalize(vec3(lightPosition) - wavyPos);
      float wavyDiff = max(dot(norm, wavyLightDir), 0.0);
      diffuse = wavyDiff * vec3(lightColor);

      // Spécularité plus vive et variable
      float wavySpec = pow(max(dot(norm, halfwayDir), 0.0), shininess * (2.0 + sin(time) * 1.0));
      specular = (1.0 + waterRipple * 2.0) * wavySpec * vec3(lightColor);

      diffuse *= attenuation;
      specular *= attenuation;

      // Effet de fresnel amélioré
      float rim = 1.0 - max(dot(viewDir, norm), 0.0);
      rim = pow(rim, 2.0 + sin(time * 0.7) * 1.0); // Fresnel variable dans le temps
      rimLight = rim * vec3(lightColor) * (0.3 + sin(time * 1.3) * 0.1) * attenuation;

      // Couleur d'eau dynamique
      vec3 baseWaterColor = vec3(0.0, 0.5, 1.0);
      vec3 waterColor = mix(baseWaterColor, vec3(0.0, 0.7, 0.8), sin(time * 0.3) * 0.5 + 0.5);
      vec3 waterEffect = (ambient + diffuse * waterDepth + specular + rimLight) * waterColor;

      // Transparence variable
      float alpha = ballColor.a * (0.1 + waterRipple + rim * 0.2);
      FragColor = vec4(waterEffect, alpha);
    }else{
    // Code normal pour les objets non-émissifs
    vec3 result = (ambient + diffuse + specular + rimLight + secondaryLightContribution + thirdLightContribution) * vec3(ballColor);
    FragColor = vec4(result, ballColor.a);
    }
  }
  // Extraction des parties brillantes pour le bloom
  vec3 finalColor = FragColor.rgb;
if (isEmissive == 1) {
    // Pour les objets émissifs, utiliser une valeur plus intense pour le bloom
    BrightColor = vec4(finalColor * 3.0, 1.0);  // Amplification pour les objets émissifs
} else {
    // Pour les objets non émissifs, appliquer un seuil de luminosité
    float brightness = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.8) {
        // Uniquement les parties très lumineuses (spéculaires) des objets non-émissifs
        BrightColor = vec4(finalColor, 1.0);
    } else {
        // Aucune contribution au bloom pour les parties sombres
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
}
