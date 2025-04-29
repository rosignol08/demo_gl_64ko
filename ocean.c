/* Ocean scene by romaric, adapted for GL4D */
/* Compatible with balle_song.c style demo */

#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "audioHelper.h"

#define ECHANTILLONS 1024

/* Function prototypes */
static void init(void);
static void resize(int width, int height);
static void draw(void);

/* Global variables */
static GLuint _wW = 1280, _wH = 720;
static GLuint _oceanGridId = 0;
static GLuint _pId = 0;

// Post-processing variables
static GLuint _fboId = 0;
static GLuint _texId = 0;
static GLuint _depthTexId = 0;
static GLuint _postProcessProgramId = 0;
static GLuint _screenQuadId = 0;
static GLuint _bloomtexture = 0;

// Audio response variable
float audioLevel = 0;

void ocean_scene(int state) {
    switch(state) {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        if(_fboId) {
            glDeleteFramebuffers(1, &_fboId);
            _fboId = 0;
        }

        if(_texId) {
            glDeleteTextures(1, &_texId);
            _texId = 0;
        }
        
        if(_depthTexId) {
            glDeleteTextures(1, &_depthTexId);
            _depthTexId = 0;
        }
        
        if(_bloomtexture) {
            glDeleteTextures(1, &_bloomtexture);
            _bloomtexture = 0;
        }
        
        if(_postProcessProgramId) {
            glDeleteProgram(_postProcessProgramId);
            _postProcessProgramId = 0;
        }
        
        if(_screenQuadId) {
            glDeleteVertexArrays(1, &_screenQuadId);
            _screenQuadId = 0;
        }
        
        if(_oceanGridId) {
            glDeleteVertexArrays(1, &_oceanGridId);
            _oceanGridId = 0;
        }
        
        if(_pId) {
            glDeleteProgram(_pId);
            _pId = 0;
        }
        
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        /* Update animation based on audio */
        {
            int i, len = ahGetAudioStreamLength();
            Sint16 *s = (Sint16 *)ahGetAudioStream();
            float volume = 0.0f;
            if(len >= 2 * ECHANTILLONS) {
                for(i = 0; i < ECHANTILLONS; i++) {
                    volume += fabsf(s[i] / 32768.0f);
                }
                volume /= ECHANTILLONS;
            }
            audioLevel = volume;
            return;
        }
    default: /* GL4DH_DRAW */
        draw();
        return;
    }
}

// Helper function to generate ocean grid mesh
GLuint generateOceanGrid(int gridSize) {
    // Calculate number of vertices and indices
    int numVertices = gridSize * gridSize;
    int numIndices = (gridSize - 1) * (gridSize - 1) * 6;
    
    // Allocate memory for vertex and index data
    float* vertices = malloc(numVertices * 5 * sizeof(float)); // 3 for pos, 2 for texcoords
    unsigned int* indices = malloc(numIndices * sizeof(unsigned int));
    
    // Generate grid vertices
    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            int index = (z * gridSize + x) * 5;
            float u = (float)x / (gridSize - 1);
            float v = (float)z / (gridSize - 1);
            
            vertices[index] = (u - 0.5f) * 10.0f;      // x
            vertices[index + 1] = 0.0f;                // y
            vertices[index + 2] = (v - 0.5f) * 10.0f;  // z
            vertices[index + 3] = u;                   // texCoord.x
            vertices[index + 4] = v;                   // texCoord.y
        }
    }
    
    // Generate indices for triangles
    int idx = 0;
    for (int z = 0; z < gridSize - 1; z++) {
        for (int x = 0; x < gridSize - 1; x++) {
            int topLeft = z * gridSize + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * gridSize + x;
            int bottomRight = bottomLeft + 1;
            
            // First triangle
            indices[idx++] = topLeft;
            indices[idx++] = bottomLeft;
            indices[idx++] = topRight;
            
            // Second triangle
            indices[idx++] = topRight;
            indices[idx++] = bottomLeft;
            indices[idx++] = bottomRight;
        }
    }
    
    // Create VAO using GL4D's lower-level functions
    GLuint vao, vbo, ibo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    
    glBindVertexArray(vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * 5 * sizeof(float), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Clean up
    free(vertices);
    free(indices);
    
    return vao;
}

/* Initialize the scene */
void init(void) {
    // Create ocean shader program - we'll use vertex and fragment shaders
    const char* vertexShaderSource = 
    "<imfs>ocean.fs</imfs>\n"
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "uniform float time;\n"
        "uniform float audioLevel;\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 modelMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "out vec2 TexCoord;\n"
        "out float height;\n"
        "void main() {\n"
        "   vec3 pos = aPos;\n"
        "   float amp = 0.1 + audioLevel * 0.5;\n"
        "   float freq = 0.2;\n"
        "   pos.y = amp * sin(pos.x * freq + time) * cos(pos.z * freq + time * 0.7);\n"
        "   height = pos.y;\n"
        "   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(pos, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\n";

    const char* fragmentShaderSource = 
    "<imfs>ocean.fs</imfs>\n"
        "#version 330 core\n"
        "in vec2 TexCoord;\n"
        "in float height;\n"
        "out vec4 FragColor;\n"
        "out vec4 BrightColor;\n"
        "uniform float time;\n"
        "uniform float audioLevel;\n"
        "void main() {\n"
        "   vec3 shallow = vec3(0.0, 0.5, 0.8);\n"
        "   vec3 deep = vec3(0.0, 0.0, 0.35);\n"
        "   vec3 color = mix(deep, shallow, height * 5.0 + 0.5);\n"
        "   \n"
        "   // Add some audio-reactive highlights\n"
        "   if(height > 0.05 - audioLevel * 0.1) {\n"
        "       color = mix(color, vec3(1.0, 1.0, 1.0), audioLevel * 3.0);\n"
        "   }\n"
        "   \n"
        "   FragColor = vec4(color, 0.9);\n"
        "   \n"
        "   // Calculate bright parts for bloom\n"
        "   float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));\n"
        "   if(brightness > 0.7 || height > 0.05 - audioLevel * 0.1)\n"
        "       BrightColor = vec4(FragColor.rgb, 1.0);\n"
        "   else\n"
        "       BrightColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "}\n";

    // Create shader program using GL4D functions
    //GLuint vs = gl4dCompileShader(vertexShaderSource, GL_VERTEX_SHADER);
    //GLuint fs = gl4dCompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    _pId = gl4duCreateProgram(vertexShaderSource, fragmentShaderSource, NULL);
    //glDeleteShader(vs);
    //glDeleteShader(fs);

    // Create post-processing shader
    _postProcessProgramId = gl4duCreateProgram("<vs>shaders/post.vs", "<fs>shaders/post.fs", NULL);

    // Create a full-screen quad for post-processing
    _screenQuadId = gl4dgGenQuadf();

    // Create FBO and textures
    glGenFramebuffers(1, &_fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, _fboId);

    // Create color texture
    glGenTextures(1, &_texId);
    glBindTexture(GL_TEXTURE_2D, _texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texId, 0);

    // Create depth texture
    glGenTextures(1, &_depthTexId);
    glBindTexture(GL_TEXTURE_2D, _depthTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexId, 0);

    // Create bloom texture
    glGenTextures(1, &_bloomtexture);
    glBindTexture(GL_TEXTURE_2D, _bloomtexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _wW, _wH, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Attach to FBO as COLOR_ATTACHMENT1
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _bloomtexture, 0);
    
    // Tell OpenGL we're using two color outputs
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    
    // Check FBO is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer not complete!\n");
    }
    
    // Return to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create ocean mesh using a geometry function
    _oceanGridId = generateOceanGrid(50); // Use 50x50 grid

    // Create matrices
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    gl4duGenMatrix(GL_FLOAT, "viewMatrix");
    gl4duGenMatrix(GL_FLOAT, "modelMatrix");

    // Enable depth testing and blending
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up initial window size
    resize(_wW, _wH);
}


/* Handle window resize */
static void resize(int width, int height) {
    GLfloat ratio;
    _wW = width;
    _wH = height;

    // Resize FBO textures
    if (_texId) {
        glBindTexture(GL_TEXTURE_2D, _texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }

    if (_depthTexId) {
        glBindTexture(GL_TEXTURE_2D, _depthTexId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    
    if (_bloomtexture) {
        glBindTexture(GL_TEXTURE_2D, _bloomtexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _wW, _wH, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    glViewport(0, 0, _wW, _wH);
    ratio = _wW / ((GLfloat)_wH);

    // Set up perspective projection
    gl4duBindMatrix("projectionMatrix");
    gl4duLoadIdentityf();
    gl4duFrustumf(-ratio, ratio, -1.0f, 1.0f, 2.0f, 100.0f);
}

/* Draw the ocean scene */
static void draw(void) {
    static float lastTime = 0;
    float currentTime = gl4dGetElapsedTime() / 1000.0f;
    float dt = currentTime - lastTime;
    lastTime = currentTime;
    
    // Amplify audio level for more visible effect
    float amplifiedAudio = audioLevel * 10.0f;
    if (amplifiedAudio > 2.0f) amplifiedAudio = 2.0f;
    
    // First pass - render to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
    
    // Important: specify the two output buffers
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    
    // Clear buffer
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use ocean shader
    glUseProgram(_pId);
    
    // Set up camera
    gl4duBindMatrix("viewMatrix");
    gl4duLoadIdentityf();
    gl4duLookAtf(0.0f, 3.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    
    // Set up model transformation
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duRotatef(-60.0f, 1, 0, 0); // Tilt ocean plane to face camera
    gl4duScalef(1.0f, 1.0f, 1.0f);
    
    // Set uniforms
    glUniform1f(glGetUniformLocation(_pId, "time"), currentTime);
    glUniform1f(glGetUniformLocation(_pId, "audioLevel"), amplifiedAudio);
    
    // Send matrices to shader
    gl4duSendMatrices();
    
    // Calculate number of indices
    int gridSize = 50;
    int numIndices = (gridSize - 1) * (gridSize - 1) * 6;
    
    // Draw ocean mesh
    glBindVertexArray(_oceanGridId);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
    
    // Second pass - post-processing
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use post-processing shader
    glUseProgram(_postProcessProgramId);
    
    // Configure post-processor to combine original scene and bloom
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texId); // Original scene texture
    glUniform1i(glGetUniformLocation(_postProcessProgramId, "screenTexture"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _bloomtexture);
    glUniform1i(glGetUniformLocation(_postProcessProgramId, "bloomTexture"), 1);
    
    // Set other uniforms
    glUniform1f(glGetUniformLocation(_postProcessProgramId, "time"), currentTime);
    glUniform2f(glGetUniformLocation(_postProcessProgramId, "resolution"), _wW, _wH);
    glUniform1f(glGetUniformLocation(_postProcessProgramId, "audioLevel"), audioLevel);
    
    // Draw full-screen quad
    gl4dgDraw(_screenQuadId);
    
    // Disable shader
    glUseProgram(0);
}