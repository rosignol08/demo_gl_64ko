#version 330
uniform sampler2D tex;
uniform float time;
in vec2 vsoTexCoord;
in float vsoTime;
out vec4 fragColor;

void main(){
    vec4 texColor = texture(tex, vsoTexCoord);
    
    if(length(texColor.xyz) > 0.0) {
        // Texte en blanc simple sans défilement
        fragColor = vec4(0.898, 0.898, 0.898, texColor.w);
    }
    else {
        // Fond noir simple
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
