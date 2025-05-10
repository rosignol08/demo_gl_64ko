#version 330 core

uniform float time;
uniform float waveStrength;
uniform float waveSpeed;
uniform int isWater;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float shininess;
uniform float noiseScale;

//pour le bruit
uniform sampler2D permTexture;
uniform sampler2D gradTexture;

in vec3 normal;
in vec3 fragPos;
in vec2 texCoord;

out vec4 FragColor;

vec3 applyWaves(vec3 position, vec3 normal) {
    float amplitude = waveStrength * 0.05;
    // Utiliser la texture de bruit pour influencer les vagues
    vec4 noiseValue = texture(permTexture, texCoord * noiseScale + vec2(time * 0.05, -time * 0.03));
    float noiseInfluence = (noiseValue.r * 2.0 - 1.0) * 0.3; // Convertir [0,1] en [-0.3,0.3]
    
    // Vagues uniquement de haut en bas avec influence du bruit
    float wave = sin(position.z * 10.0 - time * waveSpeed);
    float turbulence = sin(position.x * 5.0 + time * waveSpeed);

    // Ajouter l'influence du bruit aux vagues
    position.y += (wave * amplitude * 1.5 + turbulence * amplitude * 0.3) * (1.0 + noiseInfluence);
    
    return position;
}
void main() {
    vec3 norm = normalize(normal);
    vec3 wavyPos = applyWaves(fragPos, norm);
    vec3 viewDir = normalize(-wavyPos);
    vec3 lightDir = normalize(lightPosition - wavyPos);
    //des valeurs de bruit à différentes positions pour calculer la normale
    vec2 noiseUV = texCoord * noiseScale + vec2(cos(time * 0.2) * 0.005, -time * 0.005);
    vec4 noise = texture(permTexture, noiseUV);
    vec4 noiseDx = texture(permTexture, noiseUV + vec2(0.01, 0.0));
    vec4 noiseDy = texture(permTexture, noiseUV + vec2(0.0, 0.01));

    // Calculer les dérivées pour la normal map
    float dx = (noiseDx.r - noise.r) * waveStrength * 10.0;
    float dy = (noiseDy.r - noise.r) * waveStrength * 10.0;
    
    // Créer un vecteur de perturbation pour la normale
    vec3 normalPerturbation = normalize(vec3(-dx, 1.0, -dy));
    
    

    //norm = normalize(mix(norm, normalPerturbation, 0.3));
// Calcul d'une normale dérivée des vagues (géométrie réelle)
vec3 waveNormal = normalize(cross(
    applyWaves(fragPos + vec3(0.005, 0.0, 0.0), vec3(0,1,0)) - applyWaves(fragPos - vec3(0.005, 0.0, 0.0), vec3(0,1,0)),
    applyWaves(fragPos + vec3(0.0, 0.0, 0.005), vec3(0,1,0)) - applyWaves(fragPos - vec3(0.0, 0.0, 0.005), vec3(0,1,0))
));

// Fusion des normales : 50% vagues physiques + 30% perturbation bruit + 20% normale de base
norm = normalize(
    mix(
        mix(normal, waveNormal, 0.5),
        normalPerturbation,
        0.3
    )
);
    float waterRipple = noise.r * 0.2;  // Utiliser directement le bruit pour les ondulations

    // Vecteur vers l'arrière de l'objet (opposé à la vue)
    vec3 backVector = -viewDir;
    
    // Lumière ambiante venant de l'arrière
    float backLight = max(0.0, dot(backVector, norm)) * 0.5;
    vec3 ambient = 0.2 * lightColor + (backLight * vec3(0.2, 0.3, 0.5));
    
    // Ajout d'une variation temporelle à l'ambiance
    ambient += vec3(0.05, 0.05, 0.1) * sin(time * 0.5) * 0.5 + 0.5;

    // Diffuse et speculaire avec intensité réduite
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = 0.5 * diff * lightColor; // 50% de l'intensité originale

    float spec = pow(max(dot(norm, lightDir), 0.0), shininess * (2.0 + sin(time)));
    vec3 specular = 0.3 * (1.0 + waterRipple * 2.0) * spec * lightColor; // 30% de l'intensité originale

    vec3 rimLight = diffuse * (0.3 + sin(time * 1.3) * 0.1);
    vec3 baseColor = mix(vec3(0.0, 0.4, 0.8), vec3(0.0, 0.6, 1.0), waterRipple);

    // Somme finale avec lumière ambiante venant de l'arrière
    vec3 finalColor = baseColor * (ambient + diffuse) + specular + rimLight;

    FragColor = vec4(finalColor, 1.0);
}
