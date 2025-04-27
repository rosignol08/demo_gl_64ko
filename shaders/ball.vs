#version 330

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
uniform int isWater;
uniform float waveStrength;
uniform float waveSpeed;
uniform float time;
uniform float movementFactor; // Facteur de vitesse pour le mouvement
uniform float amplFactor;     // Facteur d'amplitude
uniform float touche; //indicateur de contact

out vec3 normal;
out vec3 fragPos;
out vec2 texCoord;

void main() {
  mat4 modelView = view * model;
  
  vec3 pos = vPosition;
  if(isWater == 1) {
      // Calculate distance from center for ripple effect
      float dist = length(pos.xz);
      
      // Create impact effect that spreads outward
      float ripple = sin(dist * 5.0 - time * waveSpeed * movementFactor) * 
                    exp(-dist * 1.50); // Exponential decay away from impact
      
      // Add additional wave detail
      float smallerRipples = sin(dist * 5.0 - time * waveSpeed * movementFactor * 1.5) * 
                           exp(-dist * 2.0) * 0.3;
      
      // Combine waves with amplitude control
      float waveHeight = (ripple + smallerRipples) * waveStrength * amplFactor;
      
      // Apply to vertical position
      pos.y += waveHeight;
      
      // Small horizontal displacement for more realistic water movement
      vec2 normalizedDir = normalize(pos.xz + vec2(0.01));
      pos.xz += normalizedDir * waveHeight * 0.2;
  }

  gl_Position = projection * modelView * vec4(pos, 1.0);
  
  // Transform normal to world space
  normal = mat3(transpose(inverse(model))) * vNormal;
  
  // Pass fragment position in world space
  fragPos = vec3(model * vec4(vPosition, 1.0));
  
  // Pass texture coordinates
  texCoord = vTexCoord;
}