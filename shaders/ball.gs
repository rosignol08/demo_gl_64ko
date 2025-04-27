#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 vPosition[];
in vec3 vNormal[];
in vec2 vTexCoord[];

out vec3 normal;
out vec3 fragPos;
out vec2 texCoord;

// Uniforms for water effect
uniform int utiliseWater;      // 1 pour activer l'effet d'eau, 0 sinon
uniform float time;            // Pour l'animation des vagues
uniform float waveStrength;    // Controls wave intensity
uniform float waveSpeed;       // Controls wave speed
uniform int isWater;           // Toggle water effect (1 = water, 0 = normal material)

// Function to generate procedural waves
vec3 applyWaves(vec3 position, vec3 normal) {
    if (isWater != 1) return position;
    
    float frequency = 20.0;
    float amplitude = waveStrength * 0.05;
    
    // Create several overlapping wave patterns
    float wave1 = sin(frequency * position.x + time * waveSpeed) * 
                    cos(frequency * position.z + time * waveSpeed * 0.7) * amplitude;
                    
    float wave2 = sin(frequency * 1.3 * position.z + time * waveSpeed * 0.8) * 
                    cos(frequency * 1.7 * position.x + time * waveSpeed * 1.1) * amplitude * 0.8;
                    
    // For sphere water drop effect, add radial waves
    float dist = length(position.xz);
    float radialWave = sin(dist * 10.0 - time * waveSpeed * 1.2) * amplitude * 0.5;
    
    // Displace position based on waves
    vec3 newPosition = position;
    newPosition.y += wave1 + wave2 + radialWave;
    
    return newPosition;
}

// Calculate normal for the wavy surface
vec3 calculateWaveNormal(vec3 pos) {
    // Calculate derivative of the wave function in x and z directions
    float epsilon = 0.01;
    vec3 dx = applyWaves(pos + vec3(epsilon, 0.0, 0.0), vec3(0.0, 1.0, 0.0)) - 
                applyWaves(pos - vec3(epsilon, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
    vec3 dz = applyWaves(pos + vec3(0.0, 0.0, epsilon), vec3(0.0, 1.0, 0.0)) - 
                applyWaves(pos - vec3(0.0, 0.0, epsilon), vec3(0.0, 1.0, 0.0));
    
    // Cross product to find normal
    return normalize(cross(dx, dz));
}

void main() {
        // Initialize output variables
        
        for (int i = 0; i < 3; i++) {
                // Apply water effect if enabled
                vec3 position = vPosition[i];
                vec3 normal = vNormal[i];
                
                if (utiliseWater == 1) {
                        position = applyWaves(position, normal);
                        normal = calculateWaveNormal(vPosition[i]);
                }
                
                // Output modified vertex
                gl_Position = gl_in[i].gl_Position;
                
                // If it's a water drop (sphere), modify position according to vertex displacement
                if (isWater == 1) {
                        // Apply the displacement to the projected position
                        vec4 pos = gl_in[i].gl_Position;
                        pos.xyz = position;
                        gl_Position = pos;
                }
                
                //gPosition = position;
                //gNormal = normal;
                //gTexCoord = vTexCoord[i];
                
                normal = normal; // ou normal = ...;
                fragPos = position; // ou fragPos = ...;
                texCoord = vTexCoord[i];
                
                EmitVertex();
        }
        
        EndPrimitive();
}