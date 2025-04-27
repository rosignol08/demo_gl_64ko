#version 330 core
out vec4 FragColor;
in vec2 texCoords;  // Assurez-vous que le nom correspond à vos vertex shaders

uniform sampler2D bloomTexture;  // Changé pour correspondre à votre code
uniform int horizontal;          // Changé en int pour correspondre à votre code C
uniform vec2 resolution;         // Ajout utile pour calculer les offsets

// Noyau gaussien pré-calculé
const float weight[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    // Calcul précis de l'offset des texels
    vec2 tex_offset = 1.0 / resolution;
    
    // Échantillonnage du texel central
    vec3 result = texture(bloomTexture, texCoords).rgb * weight[0];
    
    if (horizontal == 1) {  // Flou horizontal
        for (int i = 1; i < 5; i++) {
            result += texture(bloomTexture, texCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(bloomTexture, texCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    } else {  // Flou vertical
        for (int i = 1; i < 5; i++) {
            result += texture(bloomTexture, texCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(bloomTexture, texCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    
    // Pour le débogage, vous pouvez vérifier si quelque chose est mal défini
    if (isnan(result.r) || isnan(result.g) || isnan(result.b)) {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0);  // Rose vif pour signaler une erreur
    } else {
        FragColor = vec4(result, 1.0);
    }
}