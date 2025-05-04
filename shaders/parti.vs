#version 330
layout(location = 0) in vec2 in_pos;
layout(location = 1) in float in_radius;
layout(location = 2) in vec4 in_color;

out vec4 fragColor;

void main() {
    gl_Position = vec4(in_pos, 0.0, 1.0);
    gl_PointSize = in_radius * 720; // échelle selon résolution
    fragColor = in_color;
}