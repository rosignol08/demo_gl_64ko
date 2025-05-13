#version 330
out vec4 fragColor;
in vec2 fcoord;

uniform vec4 rectangles[10]; // Rectangles : (x, y, largeur, hauteur)
uniform vec4 rect_color;       // Couleur des rectangles
uniform int nb_rects;          // Nombre de rectangles
uniform float rect_angles[10]; // Angles des rectangles

uniform vec4 poisson[8];
uniform vec4 poisson_color;
uniform int nb_poisson;
uniform float poisson_angles; // Angles des poissons

void main() {
  //fragColor = vec4(gl_FragCoord.xy / 600.0, 0.0, 1.0);
  //si le fragment est dans un rectangle
  for (int i = 0; i < nb_rects; i++) {
    vec4 rect = rectangles[i];
    float angle = rect_angles[i];
    // + vec2(rect.z / 2.0, rect.w / 2.0); // centre du rectangle
    vec2 center = rect.xy;
    // Translate the fragment coordinate to the rectangle's local space
    vec2 localCoord = fcoord - center;

    // Apply rotation
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    vec2 rotatedCoord = vec2(
      cosAngle * localCoord.x + sinAngle * localCoord.y,
      -sinAngle * localCoord.x + cosAngle * localCoord.y
    );
    if (rotatedCoord.x >= 0.0 && rotatedCoord.x <= rect.z &&
      rotatedCoord.y >= 0.0 && rotatedCoord.y <= rect.w) {
      
      fragColor = rect_color;
      return;
    }
  }
  // Vérification des poissons
  for (int i = 0; i < nb_poisson; i++) {
    vec4 fish = poisson[i];
    vec2 center = fish.xy;
    float angle = poisson_angles;

    // Translate fragment to fish's local space
    vec2 localCoord = fcoord - center;
    
    // Rotate to align with fish orientation
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    vec2 rotatedCoord = vec2(
      cosAngle * localCoord.x + sinAngle * localCoord.y,
      -sinAngle * localCoord.x + cosAngle * localCoord.y
    );

    // Check if inside ellipse (fish body)
    float normalizedX = rotatedCoord.x * rotatedCoord.x / (fish.z * fish.z / 4.0);
    float normalizedY = rotatedCoord.y * rotatedCoord.y / (fish.w * fish.w / 4.0);

    if (normalizedX + normalizedY <= 1.0) {
      fragColor = poisson_color;
      
      // Add eye (small circle at back part of fish)
      vec2 eyePos = vec2(fish.z * 0.2,-fish.z * 0.350); // Position eye toward back
      float eyeRadius = fish.w * 0.1;
      if (length(rotatedCoord - eyePos) < eyeRadius) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black eye
      }
      
      // Add dorsal fin (triangle on bottom)
      float finHeight = fish.w * 0.5;
      vec2 finBase1 = vec2(-fish.z * 0.1, 0);
      vec2 finBase2 = vec2(fish.z * 0.2, 0);
      vec2 finPeak = vec2(fish.z * 0.05, finHeight); // Positive for downward fin
      
      // Check if point is in triangle
      vec2 v0 = finBase2 - finBase1;
      vec2 v1 = finPeak - finBase1;
      vec2 v2 = rotatedCoord - finBase1;
      
      float dot00 = dot(v0, v0);
      float dot01 = dot(v0, v1);
      float dot02 = dot(v0, v2);
      float dot11 = dot(v1, v1);
      float dot12 = dot(v1, v2);
      
      float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
      float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
      float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

      
      if (u >= 0.0 && v >= 0.0 && u + v <= 1.0) {
        fragColor = poisson_color * 0.6; // Slightly darker fin color
      }
      
      return;
    }
  }
  
    // Si rien n'est trouvé, la couleur par défaut est transparente
    fragColor = vec4(0.0, 0.0, 0.0, 0.0); 
}
  /*
  for (int i = 0; i < nb_poisson; i++){
    vec4 fish = poisson[i];
    float angle = poisson_angles;
    vec2 center = fish.xy;

    // Translate fragment to fish's local space
    vec2 localCoord = fcoord - center;

    // Rotate to align with fish orientation
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
        vec2 rotatedCoord = vec2(
        cosAngle * localCoord.x - sinAngle * localCoord.y,
        -sinAngle * localCoord.x + cosAngle * localCoord.y
    );

    // Check if inside ellipse
    // fish.z = width, fish.w = height
    float normalizedX = rotatedCoord.x * rotatedCoord.x / (fish.z * fish.z / 4.0);
    float normalizedY = rotatedCoord.y * rotatedCoord.y / (fish.w * fish.w / 4.0);

    if (normalizedX + normalizedY <= 1.0) {
      fragColor = poisson_color;
      return;
    }
  }
 */
 