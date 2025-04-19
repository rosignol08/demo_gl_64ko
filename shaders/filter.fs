#version 330
// https://www.shadertoy.com/view/MdffD7
// Fork of FMS_Cat's VCR distortion shader

uniform sampler2D screen_texture;

uniform vec2 vhs_resolution = vec2(320.0, 240.0);

uniform vec3 rgb_shift_horison = vec3(0.0, 0.0, 0.0);
uniform vec3 rgb_shift_verti = vec3(0.0, 0.0, 0.0);

// Basic shader inputs
uniform float time;
in vec2 vsoTexCoord;
#define PI 3.1415926535897932384626433832795
out vec4 fragColor;

uniform int samples = 0;

// Tape crease uniforms
uniform float tape_crease_smear = 1.0;
uniform float tape_crease_jitter = 0.10;
uniform float tape_crease_speed = 0.5;
uniform float tape_crease_discoloration = 1.0;

// Noise uniforms
//uniform vec2 blur_direction = vec2(1.0, 0.0); // Flou horizontal par défaut
//uniform float blur_intensity = 1.0; // Intensité du flou

vec3 apply_directional_blur(vec2 uv, sampler2D tex, vec2 direction, float intensity) {
	vec3 color = vec3(0.0);
	float total_weight = 0.0;
	for (int i = -samples; i <= samples; i++) {
		float weight = 1.0 - abs(float(i)) / float(samples); // Poids dégressif
		vec2 offset = direction * float(i) * intensity / vhs_resolution;
		color += texture(tex, uv + offset).rgb * weight;
		total_weight += weight;
	}
	return color / total_weight;
}


float v2random(vec2 uv) {
	//return 0.5;// texture(noise_texture, mod(uv, vec2(1.0))).x;
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	vec2 uvn = vsoTexCoord;
	vec3 col = vec3(0.0, 0.0, 0.0);

	// Tape wave.
	uvn.x += (v2random(vec2(uvn.y / 10.0, time / 10.0) / 1.0) - 0.5) / vhs_resolution.x * 1.0;
	uvn.x += (v2random(vec2(uvn.y, time * 10.0)) - 0.5) / vhs_resolution.x * 1.0;
	// tape crease
	float tc_phase = smoothstep(0.9, 0.96, sin(uvn.y * 8.0 - (time * tape_crease_speed + tape_crease_jitter * v2random(time * vec2(0.67, 0.59))) * PI * 1.2));
	float tc_noise = smoothstep(0.3, 1.0, v2random(vec2(uvn.y * 4.77, time)));
	float tc = tc_phase * tc_noise;
	uvn.x = uvn.x - tc / vhs_resolution.x * 8.0 * tape_crease_smear;
	// Décalage RGB
	vec2 red_uv = uvn + vec2(rgb_shift_horison.r, rgb_shift_verti.r);
	vec2 green_uv = uvn + vec2(rgb_shift_horison.g, rgb_shift_verti.g);
	vec2 blue_uv = uvn + vec2(rgb_shift_horison.b, rgb_shift_verti.b);
	float r = texture(screen_texture, red_uv).r;
	float g = texture(screen_texture, green_uv).g;
	float b = texture(screen_texture, blue_uv).b;
	col = vec3(r, g, b);
	fragColor = vec4(col, 1.0);
}