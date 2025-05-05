#version 330 core

in vec3 vsoNormal;
in vec3 vsoPosition;
in vec2 vsoTexCoord;

uniform vec4 objectColor;
uniform vec4 lightPos;
uniform vec4 lightColor;

out vec4 fragColor;

void main() {
    // Simple lighting calculation
    vec3 norm = normalize(vsoNormal);
    vec3 lightDir = normalize(lightPos.xyz - vsoPosition);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor.rgb;

    vec3 ambient = 0.3 * lightColor.rgb;

    vec3 result = (ambient + diffuse) * objectColor.rgb;
    fragColor = vec4(result, 1.0);
}
