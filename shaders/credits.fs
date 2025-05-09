#version 330
uniform sampler2D tex;
uniform float time;
in vec2 vsoTexCoord;
in float vsoTime;
out vec4 fragColor;

void main(){
  vec4 texColor = texture(tex, vsoTexCoord);
  
  // Défilement simple
  float scrollPosition = time * 0.2 - 1.0;
  scrollPosition = smoothstep(scrollPosition - .1, scrollPosition, vsoTexCoord.y);
  
  if(length(texColor.xyz) > 0.0) {
    // Texte en blanc simple avec défilement
    fragColor = vec4(1.0, 1.0, 1.0, texColor.w * scrollPosition);
  }
  else {
    // Fond noir simple
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
}