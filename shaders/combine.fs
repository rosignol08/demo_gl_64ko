#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D blurredTexture;
uniform float bloomIntensity = 1.0;
uniform float exposure = 1.0;

// Dans combine.fs - Améliore le rendu HDR
void main() {
    // Échantillonner les textures
    vec3 hdr = texture(sceneTexture, TexCoords).rgb;
    vec3 bloom = texture(blurredTexture, TexCoords).rgb;
    
    // Accentuation du bloom (amplification)
    bloom = bloom * 1.5; // Boost supplémentaire
    
    // Mélange additif avec intensité
    vec3 result = hdr + bloom * bloomIntensity;
    
    // Tone mapping - modification importante
    vec3 mapped = vec3(1.0) - exp(-result * exposure);
    
    // Ajustement des couleurs pour plus d'éclat
    mapped = pow(mapped, vec3(0.9)); // Gamma correction légère
    
    // Sortie
    FragColor = vec4(mapped, 1.0);
}