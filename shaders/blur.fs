#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool horizontal;
uniform float weight[5] = float[5](0.227027027, 0.194594595, 0.121621622, 0.054054054, 0.016216216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(screenTexture, 0); // Taille de la texture
    vec3 result = texture(screenTexture, TexCoords).rgb * weight[0]; // Poids central
    if (horizontal){
        for (int i = 1; i < 5; i++) {
            result += texture(screenTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }else{
        for (int i = 1; i < 5; i++) {
            result += texture(screenTexture, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    
    FragColor = vec4(result, 1.0);

}