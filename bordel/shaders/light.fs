/*!\file light.fs
 * \brief fragment shader progressif sur l'éclairage. */
#version 330
/* caractéristiques diffus et ambient de la surface */
uniform vec4 sdiffus, sambient;
uniform mat4 projectionMatrix, viewMatrix, modelMatrix;
uniform vec4 Lp;
in vec4 vmPos;
in vec3 vmNormal;
/* sortie du frament shader : une couleur */
out vec4 fragColor;

void main(void) {
  vec4 vLp = viewMatrix * Lp;
  vec3 Ld  = normalize((vmPos - vLp).xyz);
  float intensite_diffus = clamp(dot(normalize(vmNormal), -Ld), 0.0, 1.0);
  fragColor = 0.25 /* 25% d'ambient */ * sambient + 0.75 * intensite_diffus * sdiffus;
}
