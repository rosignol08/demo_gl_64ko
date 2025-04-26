#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D blurredTexture;
uniform float bloomIntensity = 1.0;
uniform float exposure = 1.0;

// Dans combine.fs - Améliore le rendu HDR
void main() {
    vec3 hdr = texture(sceneTexture, TexCoords).rgb;
    vec3 bloom = texture(blurredTexture, TexCoords).rgb;
    
    // Amplification du bloom pour plus d'effet
    bloom *= 1.5; // Pré-amplification
    
    // Mélange avec intensité
    vec3 result = hdr + bloom * bloomIntensity;
    
    // Tone mapping amélioré
    vec3 mapped = vec3(1.0) - exp(-result * exposure);
    
    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));
    
    FragColor = vec4(mapped, 1.0);
}