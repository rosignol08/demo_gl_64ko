#version 330
out vec4 fragColor;
in vec2 fcoord;
uniform vec4 positions[1024];
uniform vec4 couleurs[1024];
uniform int nbe;

uniform vec4 rectangles[8]; // Rectangles : (x, y, largeur, hauteur)
uniform vec4 rect_color;       // Couleur des rectangles
uniform int nb_rects;          // Nombre de rectangles
uniform float rect_angles[8]; // Angles des rectangles

uniform vec4 poisson[8];
uniform vec4 poisson_color;
uniform int nb_poisson;
uniform float poisson_angles[8]; // Angles des poissons

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

    // Check if the rotated coordinate is within the rectangle's bounds
    if (rotatedCoord.x >= 0.0 && rotatedCoord.x <= rect.z &&
      rotatedCoord.y >= 0.0 && rotatedCoord.y <= rect.w) {
      
      fragColor = rect_color;
      return;
    }
  }
  for (int i = 0; i < nb_poisson; i++){
    vec4 fish = poisson[i];
    float angle = poisson_angles[i];
    vec2 center = fish.xy;

    // Translate fragment to fish's local space
    vec2 localCoord = fcoord - center;

    // Rotate to align with fish orientation
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    vec2 rotatedCoord = vec2(
      cosAngle * localCoord.x + sinAngle * localCoord.y,
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
  //sinon 
//  for (int i = 0; i < nbe; ++i) {
//    vec2 delta = positions[i].xy - fcoord;
//    
//    // Early discard si le fragment est trop loin
//    if (abs(delta.x) > 0.01 || abs(delta.y) > 0.01) continue;
//
//    if (dot(delta, delta) < 0.01 * 0.01) {
//        fragColor = vec4(0.0, 0.0, 1.0, 1.0);
//        return;
//    }
//}
}

  //for (int i = 0; i < nbe; ++i) {
  //  float dist = distance(positions[i].xy, fcoord);
  //  float radius = positions[i].z; // le rayon du mobile i
  //  
  //  if(dist < radius) {
  //    // Soft edge effect: 1.0 at center, fading towards edge
  //    float softness = 0.01; // Adjust this value to control blur amount (0.0-1.0)
  //    float alpha = smoothstep(radius, radius * (1.0 - softness), dist);
  //    
  //    // Mix with background color based on alpha
  //    vec4 bgColor = vec4(0.0, 0.0, 1.0, 1.0); // The background color
  //    fragColor = mix(bgColor, couleurs[i], alpha);
  //    return;
  //  }      
  //}

