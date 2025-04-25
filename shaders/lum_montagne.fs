/*!\file light.fs
 * \brief fragment shader progressif sur l'éclairage. */
#version 330
/* caractéristiques diffus et ambient de la surface et des lumières */
uniform vec4 sdiffus, sambient, sspeculaire, l0diffus, l0speculaire, l1diffus, l1speculaire, lambient;
uniform mat4 projectionMatrix, viewMatrix, modelMatrix;
uniform vec4 Lp0, Lp1;
uniform float temps;
uniform int has_noise, sky;
in vec4 vmPos;
in vec3 vmNormal;
in vec2 tc;
/* sortie du frament shader : une couleur */
out vec4 fragColor;

/*
 * 2D, 3D and 4D Perlin noise (classic) in a GLSL fragment shader.
 *
 * Classic noise is implemented by the functions:
 * float noise(vec2 P)
 * float noise(vec3 P)
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 *
 * You may use, modify and redistribute this code free of charge,
 * provided that the author's names and this notice appear intact.
 */

uniform sampler2D permTexture;
uniform sampler2D gradTexture;

#define ONE 0.00390625
#define ONEHALF 0.001953125

/*
 * The interpolation function. 
 */
float fade(float t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
}

/*
 * 2D classic Perlin noise. Fast, but less useful than 3D noise.
 */
float noise(vec2 P)
{
  vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
  vec2 Pf = fract(P);             // Fractional part for interpolation

  // Noise contribution from lower left corner
  vec2 grad00 = texture(permTexture, Pi).rg * 4.0 - 1.0;
  float n00 = dot(grad00, Pf);

  // Noise contribution from lower right corner
  vec2 grad10 = texture(permTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
  float n10 = dot(grad10, Pf - vec2(1.0, 0.0));

  // Noise contribution from upper left corner
  vec2 grad01 = texture(permTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
  float n01 = dot(grad01, Pf - vec2(0.0, 1.0));

  // Noise contribution from upper right corner
  vec2 grad11 = texture(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
  float n11 = dot(grad11, Pf - vec2(1.0, 1.0));

  // Blend contributions along x
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade(Pf.x));

  // Blend contributions along y
  float n_xy = mix(n_x.x, n_x.y, fade(Pf.y));

  // We're done, return the final noise value.
  return n_xy;
}

/*
 * 3D classic noise. Slower, but a lot more useful than 2D noise.
 */
float noise(vec3 P)
{
  vec3 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled so +1 moves one texel
                                  // and offset 1/2 texel to sample texel centers
  vec3 Pf = fract(P);     // Fractional part for interpolation

  // Noise contributions from (x=0, y=0), z=0 and z=1
  float perm00 = texture(permTexture, Pi.xy).a ;
  vec3  grad000 = texture(permTexture, vec2(perm00, Pi.z)).rgb * 4.0 - 1.0;
  float n000 = dot(grad000, Pf);
  vec3  grad001 = texture(permTexture, vec2(perm00, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n001 = dot(grad001, Pf - vec3(0.0, 0.0, 1.0));

  // Noise contributions from (x=0, y=1), z=0 and z=1
  float perm01 = texture(permTexture, Pi.xy + vec2(0.0, ONE)).a ;
  vec3  grad010 = texture(permTexture, vec2(perm01, Pi.z)).rgb * 4.0 - 1.0;
  float n010 = dot(grad010, Pf - vec3(0.0, 1.0, 0.0));
  vec3  grad011 = texture(permTexture, vec2(perm01, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n011 = dot(grad011, Pf - vec3(0.0, 1.0, 1.0));

  // Noise contributions from (x=1, y=0), z=0 and z=1
  float perm10 = texture(permTexture, Pi.xy + vec2(ONE, 0.0)).a ;
  vec3  grad100 = texture(permTexture, vec2(perm10, Pi.z)).rgb * 4.0 - 1.0;
  float n100 = dot(grad100, Pf - vec3(1.0, 0.0, 0.0));
  vec3  grad101 = texture(permTexture, vec2(perm10, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n101 = dot(grad101, Pf - vec3(1.0, 0.0, 1.0));

  // Noise contributions from (x=1, y=1), z=0 and z=1
  float perm11 = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a ;
  vec3  grad110 = texture(permTexture, vec2(perm11, Pi.z)).rgb * 4.0 - 1.0;
  float n110 = dot(grad110, Pf - vec3(1.0, 1.0, 0.0));
  vec3  grad111 = texture(permTexture, vec2(perm11, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n111 = dot(grad111, Pf - vec3(1.0, 1.0, 1.0));

  // Blend contributions along x
  vec4 n_x = mix(vec4(n000, n001, n010, n011),
                 vec4(n100, n101, n110, n111), fade(Pf.x));

  // Blend contributions along y
  vec2 n_xy = mix(n_x.xy, n_x.zw, fade(Pf.y));

  // Blend contributions along z
  float n_xyz = mix(n_xy.x, n_xy.y, fade(Pf.z));

  // We're done, return the final noise value.
  return n_xyz;
}

vec2 rug(void) {
  const float zoom = 30.0;
  vec2 no = vec2(0.0);
  for(float freq = 1.0, amp = 1.0; freq < 33.0; freq *= 2.0, amp /= 2.0)
    no += vec2(amp * noise(freq * zoom * tc.xy), amp * noise(freq * zoom * tc.yx));
  return no;
}

float rug22(void) {
  const float zoom = 0.30; // Nuages plus larges avec un zoom plus petit
  float no = 0.0;
  // Utilisez moins d'octaves pour un rendu plus rapide et fluide
  for(float freq = 1.0, amp = 1.0; freq < 8.0; freq *= 2.0, amp /= 2.0)
    no += amp * noise(freq * zoom * vmPos.xyz + vec3(0.0, 0.0, temps)); // Ajout du temps pour animation
  
  // Ajustement de la distribution pour avoir des zones vides et des zones denses
  no = smoothstep(0.1, 0.9, no);
  return no;
}
float rug2(void) {
  const float zoom = 0.30; // Nuages plus larges avec un zoom plus petit
  float no = 0.0;
  // Animation basée sur le temps
  for(float freq = 1.0, amp = 1.0; freq < 8.0; freq *= 2.0, amp /= 2.0)
    no += amp * noise(freq * zoom * vmPos.xyz + vec3(0.0, 0.0, temps * 1)); //le * 1 c'est la vitesse de défilement des nuages
  
  // Augmenter progressivement la densité des nuages avec le temps
  float cloudCoverage = clamp(temps * 0.05, 0.1, 0.9); // Contrôle la couverture du ciel
  
  // Ajustement de la distribution avec seuil variable
  no = smoothstep(cloudCoverage, cloudCoverage + 0.2, no);
  return no;
}

void main(void) {
  if(sky != 0) {
    // Augmenter la valeur du seuil inférieur dans smoothstep pour avoir plus de nuages
    float nu = rug2();
    vec4 cloudColor = vec4(1.0, 1.0, 1.0, 1.0);
    // Bleu ciel pour le fond
    vec4 skyColor = vec4(0.5, 0.7, 1.0, 1.0);
    
    // Ajuster le seuil pour augmenter la densité des nuages (de 0.1 à 0.0)
    // Diminuer l'écart entre les valeurs pour des nuages plus grands
    float cloudIntensity = smoothstep(0.0, 1.0, nu);
    
    // Mélanger le ciel et les nuages
    fragColor = mix(skyColor, cloudColor, cloudIntensity);
    return;
  }
  const vec3 vue = vec3(0.0, 0.0, -1.0);
  vec4 vLp0 = viewMatrix * Lp0;
  vec3 Ld0  = normalize((vmPos - vLp0).xyz);
  vec4 vLp1 = viewMatrix * Lp1;
  vec3 Ld1  = normalize((vmPos - vLp1).xyz);
  vec3 n = normalize(vmNormal);
  /* simulation de normal map */
  //if(has_noise != 0) {
  vec3 T = vec3(1.0, 0.0, 0.0);
  vec3 B = cross(T, n);
  vec2 no = rug();
  n = normalize(4.0 * n + no.x * T + no.y * B);
  //}
  /* fin simulation */

  float intensite_diffus0 = clamp(dot(n, -Ld0), 0.0, 1.0);
  float intensite_diffus1 = clamp(dot(n, -Ld1), 0.0, 1.0);
  vec3 R0 = reflect(Ld0, n);
  vec3 R1 = reflect(Ld1, n);
  float intensite_speculaire0 = pow(clamp(dot(R0, -vue), 0.0, 1.0), 400.0);
  float intensite_speculaire1 = pow(clamp(dot(R1, -vue), 0.0, 1.0), 400.0);
  vec4 diffus = intensite_diffus0 * sdiffus * l0diffus + intensite_diffus1 * sdiffus * l1diffus;
  /* diffus = vec4(ivec4(diffus * 4.0)) / 4.0; */ /* cell shading (toon shading) */
  vec4 ambient = sambient * lambient;
  fragColor = mix(ambient, diffus, 0.75) + intensite_speculaire0 * l0speculaire * sspeculaire + intensite_speculaire1 * l1speculaire * sspeculaire;
}
