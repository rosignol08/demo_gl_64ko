#version 330

in vec3 normal;
in vec3 fragPos;
in vec2 texCoord;

uniform vec4 ballColor;
uniform vec4 lightColor;
uniform vec4 lightPosition;
uniform vec4 ambientColor;
uniform float shininess;

out vec4 fragColor;

void main() {
  // Normalize the normal vector
  vec3 norm = normalize(normal);
  
  // Calculate light direction
  vec3 lightDir = normalize(vec3(lightPosition) - fragPos);
  
  // Ambient component
  vec3 ambient = vec3(ambientColor);
  
  // Diffuse component
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * vec3(lightColor);
  
  // Specular component - AUGMENTÉE pour des reflets plus visibles
  vec3 viewDir = normalize(vec3(0.0, 0.0, 5.0) - fragPos); // Direction de la caméra
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess); // Exposant plus élevé pour un reflet plus concentré
  vec3 specular = 1.0 * spec * vec3(lightColor); // Coefficient augmenté à 1.0
  
  // Ajout d'un effet de fresnel simple pour les bords
  float rim = 1.0 - max(dot(viewDir, norm), 0.0);
  rim = pow(rim, 3.0);
  vec3 rimLight = rim * vec3(lightColor) * 0.3; // Effet de contour lumineux subtil
  
  // Combine all lighting components
  vec3 result = (ambient + diffuse + specular + rimLight) * vec3(ballColor);
  
  fragColor = vec4(result, ballColor.a);
}