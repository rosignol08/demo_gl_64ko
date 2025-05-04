#version 330

in vec4 fragColor;
out vec4 outColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    if (dot(coord, coord) > 1.0) discard;
    outColor = fragColor;
}
