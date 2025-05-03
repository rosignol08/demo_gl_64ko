/*
 * Programme de simulation d'océan avec bruit de Perlin
 * Utilise OpenGL et GLSL pour déformer un maillage plat 
 * et y appliquer une texture d'eau basée sur le bruit de Perlin
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <GL/glew.h>
 #include <GLFW/glfw3.h>
 #include <math.h>
 
 // Dimensions de la fenêtre
 #define WINDOW_WIDTH 1024
 #define WINDOW_HEIGHT 768
 
 // Paramètres du maillage de l'océan
 #define GRID_SIZE 100 // Nombre de subdivisions du maillage
 #define GRID_LENGTH 20.0f // Taille réelle du maillage
 
 // Paramètres de la simulation
 #define WAVE_SPEED 0.5f
 #define WAVE_HEIGHT 0.8f
 
 // Variables globales
 GLFWwindow* window;
 GLuint vao, vbo, ebo;
 GLuint shaderProgram;
 GLuint perlinTexture;
 int numIndices;
 float time_value = 0.0f;
 
 // Prototypes des fonctions
 void init();
 void createMesh();
 void createPerlinTexture();
 void renderLoop();
 void cleanup();
 GLuint compileShaders();
 
 // Vertex shader pour la déformation du maillage
 const char* vertexShaderSource = R"(
 #version 330 core
 
 layout (location = 0) in vec3 aPos;
 layout (location = 1) in vec2 aTexCoord;
 
 out vec2 TexCoord;
 out vec3 FragPos;
 out vec3 Normal;
 
 uniform mat4 model;
 uniform mat4 view;
 uniform mat4 projection;
 uniform float time;
 uniform sampler2D perlinTex;
 
 // Fonction pour calculer les vagues
 vec3 calculateWaves(vec3 position, float time) {
     // Position originale
     vec3 pos = position;
     
     // Échantillonne le bruit de Perlin pour la hauteur
     vec2 texCoord = position.xz / 20.0 + 0.5;
     float noise1 = texture(perlinTex, texCoord * 0.1 + vec2(time * 0.02, time * 0.01)).r;
     float noise2 = texture(perlinTex, texCoord * 0.2 - vec2(time * 0.01, time * 0.02)).r;
     float noise3 = texture(perlinTex, texCoord * 0.05 + vec2(time * 0.01, -time * 0.015)).r;
     
     // Combinaison de plusieurs fréquences de bruit
     float height = noise1 * 0.6 + noise2 * 0.3 + noise3 * 0.1;
     
     // Ajoute quelques vagues sinusoïdales
     height += sin(position.x * 0.5 + time) * 0.1;
     height += cos(position.z * 0.3 + time * 0.8) * 0.1;
     
     // Applique la hauteur
     pos.y = height * 2.0 - 0.5;
     
     return pos;
 }
 
 // Calcul de la normale en fonction des dérivées partielles
 vec3 calculateNormal(vec3 position, float delta) {
     vec3 pos1 = calculateWaves(position + vec3(delta, 0.0, 0.0), time);
     vec3 pos2 = calculateWaves(position + vec3(0.0, 0.0, delta), time);
     vec3 pos0 = calculateWaves(position, time);
     
     vec3 tangent = pos1 - pos0;
     vec3 bitangent = pos2 - pos0;
     
     return normalize(cross(tangent, bitangent));
 }
 
 void main() {
     // Déformation du maillage par les vagues
     vec3 wavePos = calculateWaves(aPos, time);
     
     // Calcul de la normale
     Normal = calculateNormal(aPos, 0.1);
     
     // Coordonnées de texture
     TexCoord = aTexCoord;
     
     // Position dans l'espace fragment
     FragPos = vec3(model * vec4(wavePos, 1.0));
     
     // Position finale
     gl_Position = projection * view * model * vec4(wavePos, 1.0);
 }
 )";
 
 // Fragment shader pour le rendu de l'eau
 const char* fragmentShaderSource = R"(
 #version 330 core
 
 in vec2 TexCoord;
 in vec3 FragPos;
 in vec3 Normal;
 
 out vec4 FragColor;
 
 uniform float time;
 uniform sampler2D perlinTex;
 uniform vec3 lightPos;
 uniform vec3 viewPos;
 uniform vec3 waterDeepColor;
 uniform vec3 waterShallowColor;
 uniform vec3 specularColor;
 
 void main() {
     // Paramètres de l'eau
     vec3 deepColor = waterDeepColor;
     vec3 shallowColor = waterShallowColor;
     vec3 specColor = specularColor;
     
     // Animation des coordonnées de texture pour simuler le mouvement de l'eau
     vec2 texCoord1 = TexCoord * 2.0 + vec2(time * 0.03, time * 0.02);
     vec2 texCoord2 = TexCoord * 3.0 - vec2(time * 0.02, time * 0.01);
     
     // Échantillonnage du bruit de Perlin pour créer des détails de surface
     float noise1 = texture(perlinTex, texCoord1).r;
     float noise2 = texture(perlinTex, texCoord2).r;
     
     // Mélange des bruits pour créer des détails plus complexes
     float noiseMix = (noise1 * 0.7 + noise2 * 0.3) * 0.8;
     
     // Calcul de l'éclairage
     vec3 norm = normalize(Normal + vec3(noise1 * 0.1 - 0.05, 0.0, noise2 * 0.1 - 0.05));
     vec3 lightDir = normalize(lightPos - FragPos);
     vec3 viewDir = normalize(viewPos - FragPos);
     vec3 reflectDir = reflect(-lightDir, norm);
     
     // Composantes d'éclairage
     float diff = max(dot(norm, lightDir), 0.0);
     float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
     float fresnel = pow(1.0 - max(dot(norm, viewDir), 0.0), 4.0);
     
     // Couleur de l'eau basée sur la profondeur simulée et la direction de la normale
     vec3 waterColor = mix(deepColor, shallowColor, noiseMix);
     
     // Ajout de l'éclairage
     vec3 diffuse = diff * vec3(1.0, 1.0, 1.0) * 0.7;
     vec3 specular = spec * specColor;
     vec3 ambient = vec3(0.2, 0.3, 0.4);
     
     // Couleur finale avec réflexion de Fresnel
     vec3 result = waterColor * (ambient + diffuse) + specular + fresnel * specColor * 0.5;
     
     FragColor = vec4(result, 0.9);
 }
 )";
 
 // Fonction principale
 int main() {
     // Initialisation de GLFW et OpenGL
     if (!glfwInit()) {
         fprintf(stderr, "Erreur lors de l'initialisation de GLFW\n");
         return -1;
     }
     
     // Configuration de GLFW
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
     
     // Création de la fenêtre
     window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Simulation d'Océan avec Bruit de Perlin", NULL, NULL);
     if (window == NULL) {
         fprintf(stderr, "Erreur lors de la création de la fenêtre GLFW\n");
         glfwTerminate();
         return -1;
     }
     
     glfwMakeContextCurrent(window);
     
     // Initialisation de GLEW
     if (glewInit() != GLEW_OK) {
         fprintf(stderr, "Erreur lors de l'initialisation de GLEW\n");
         return -1;
     }
     
     printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
     
     // Initialisation des ressources
     init();
     
     // Boucle principale de rendu
     renderLoop();
     
     // Nettoyage
     cleanup();
     
     return 0;
 }
 
 // Initialisation des ressources OpenGL
 void init() {
     // Compilation des shaders
     shaderProgram = compileShaders();
     
     // Création du maillage
     createMesh();
     
     // Création de la texture de bruit de Perlin
     createPerlinTexture();
     
     // Configuration du rendu OpenGL
     glEnable(GL_DEPTH_TEST);
     glEnable(GL_BLEND);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
     
     // Initialisation de la graine aléatoire
     srand((unsigned int)time(NULL));
 }
 
 // Fonction pour générer le bruit de Perlin
 float fade(float t) {
     return t * t * t * (t * (t * 6 - 15) + 10);
 }
 
 float lerp(float a, float b, float t) {
     return a + t * (b - a);
 }
 
 float grad(int hash, float x, float y, float z) {
     int h = hash & 15;
     float u = h < 8 ? x : y;
     float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
     return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
 }
 
 float perlinNoise(float x, float y, float z, int permutation[512]) {
     int X = (int)floor(x) & 255;
     int Y = (int)floor(y) & 255;
     int Z = (int)floor(z) & 255;
     
     x -= floor(x);
     y -= floor(y);
     z -= floor(z);
     
     float u = fade(x);
     float v = fade(y);
     float w = fade(z);
     
     int A = permutation[X] + Y;
     int AA = permutation[A] + Z;
     int AB = permutation[A + 1] + Z;
     int B = permutation[X + 1] + Y;
     int BA = permutation[B] + Z;
     int BB = permutation[B + 1] + Z;
     
     return lerp(lerp(lerp(grad(permutation[AA], x, y, z),
                            grad(permutation[BA], x - 1, y, z), u),
                       lerp(grad(permutation[AB], x, y - 1, z),
                            grad(permutation[BB], x - 1, y - 1, z), u), v),
                 lerp(lerp(grad(permutation[AA + 1], x, y, z - 1),
                            grad(permutation[BA + 1], x - 1, y, z - 1), u),
                       lerp(grad(permutation[AB + 1], x, y - 1, z - 1),
                            grad(permutation[BB + 1], x - 1, y - 1, z - 1), u), v), w);
 }
 
 // Création de la texture de bruit de Perlin
 void createPerlinTexture() {
     const int texSize = 256;
     unsigned char* data = (unsigned char*)malloc(texSize * texSize * 4);
     
     // Table de permutation pour le bruit de Perlin
     int permutation[256];
     for (int i = 0; i < 256; i++) {
         permutation[i] = i;
     }
     
     // Mélange de la table de permutation
     for (int i = 255; i > 0; i--) {
         int j = rand() % (i + 1);
         int temp = permutation[i];
         permutation[i] = permutation[j];
         permutation[j] = temp;
     }
     
     // Duplication de la table pour éviter l'overflow
     int p[512];
     for (int i = 0; i < 256; i++) {
         p[i] = p[i + 256] = permutation[i];
     }
     
     // Génération du bruit
     float frequency = 0.01f;
     float amplitude = 1.0f;
     int octaves = 6;
     
     for (int y = 0; y < texSize; y++) {
         for (int x = 0; x < texSize; x++) {
             float total = 0.0f;
             float freq = frequency;
             float amp = amplitude;
             
             // Cumul de plusieurs octaves pour un bruit plus naturel
             for (int o = 0; o < octaves; o++) {
                 float noiseVal = perlinNoise(x * freq, y * freq, 0.5f, p);
                 total += noiseVal * amp;
                 
                 freq *= 2.0f;
                 amp *= 0.5f;
             }
             
             // Normalisation des valeurs entre 0 et 1
             total = (total + 1.0f) / 2.0f;
             
             // Conversion en valeur RGB
             unsigned char value = (unsigned char)(total * 255.0f);
             
             int index = (y * texSize + x) * 4;
             data[index] = value;     // R
             data[index + 1] = value; // G
             data[index + 2] = value; // B
             data[index + 3] = 255;   // A
         }
     }
     
     // Création de la texture OpenGL
     glGenTextures(1, &perlinTexture);
     glBindTexture(GL_TEXTURE_2D, perlinTexture);
     
     // Configuration des paramètres de la texture
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     
     // Chargement des données dans la texture
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
     glGenerateMipmap(GL_TEXTURE_2D);
     
     free(data);
 }
 
 // Création du maillage plat qui représentera l'océan
 void createMesh() {
     float* vertices = (float*)malloc(GRID_SIZE * GRID_SIZE * 8 * sizeof(float));
     unsigned int* indices = (unsigned int*)malloc(6 * (GRID_SIZE - 1) * (GRID_SIZE - 1) * sizeof(unsigned int));
     
     // Génération des vertices
     for (int z = 0; z < GRID_SIZE; z++) {
         for (int x = 0; x < GRID_SIZE; x++) {
             int index = (z * GRID_SIZE + x) * 8;
             
             // Position (x, y, z)
             vertices[index] = ((float)x / (GRID_SIZE - 1) - 0.5f) * GRID_LENGTH;
             vertices[index + 1] = 0.0f; // y sera modifié par le shader
             vertices[index + 2] = ((float)z / (GRID_SIZE - 1) - 0.5f) * GRID_LENGTH;
             
             // Texture coordinates (u, v)
             vertices[index + 3] = (float)x / (GRID_SIZE - 1);
             vertices[index + 4] = (float)z / (GRID_SIZE - 1);
             
             // Normal (initially pointing up)
             vertices[index + 5] = 0.0f;
             vertices[index + 6] = 1.0f;
             vertices[index + 7] = 0.0f;
         }
     }
     
     // Génération des indices pour les triangles
     int indexCount = 0;
     for (int z = 0; z < GRID_SIZE - 1; z++) {
         for (int x = 0; x < GRID_SIZE - 1; x++) {
             unsigned int topLeft = z * GRID_SIZE + x;
             unsigned int topRight = topLeft + 1;
             unsigned int bottomLeft = (z + 1) * GRID_SIZE + x;
             unsigned int bottomRight = bottomLeft + 1;
             
             // Premier triangle
             indices[indexCount++] = topLeft;
             indices[indexCount++] = bottomLeft;
             indices[indexCount++] = topRight;
             
             // Second triangle
             indices[indexCount++] = topRight;
             indices[indexCount++] = bottomLeft;
             indices[indexCount++] = bottomRight;
         }
     }
     
     numIndices = indexCount;
     
     // Création des buffers OpenGL
     glGenVertexArrays(1, &vao);
     glGenBuffers(1, &vbo);
     glGenBuffers(1, &ebo);
     
     glBindVertexArray(vao);
     
     // Liaison et configuration du VBO
     glBindBuffer(GL_ARRAY_BUFFER, vbo);
     glBufferData(GL_ARRAY_BUFFER, GRID_SIZE * GRID_SIZE * 8 * sizeof(float), vertices, GL_STATIC_DRAW);
     
     // Liaison et configuration du EBO
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
     
     // Configuration des attributs de vertex
     // Position
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
     glEnableVertexAttribArray(0);
     
     // Texture coordinates
     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
     glEnableVertexAttribArray(1);
     
     // Normal
     glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
     glEnableVertexAttribArray(2);
     
     free(vertices);
     free(indices);
 }
 
 // Compilation et liaison des shaders
 GLuint compileShaders() {
     // Vertex shader
     GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
     glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
     glCompileShader(vertexShader);
     
     // Vérification de la compilation du fragment shader
     glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
     if (!success) {
         glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
         fprintf(stderr, "Erreur de compilation du fragment shader: %s\n", infoLog);
     }
     
     // Programme shader
     GLuint program = glCreateProgram();
     glAttachShader(program, vertexShader);
     glAttachShader(program, fragmentShader);
     glLinkProgram(program);
     
     // Vérification de la liaison du programme
     glGetProgramiv(program, GL_LINK_STATUS, &success);
     if (!success) {
         glGetProgramInfoLog(program, 512, NULL, infoLog);
         fprintf(stderr, "Erreur de liaison du programme shader: %s\n", infoLog);
     }
     
     // Nettoyage des shaders (ils sont déjà liés au programme)
     glDeleteShader(vertexShader);
     glDeleteShader(fragmentShader);
     
     return program;
 }
 
 // Boucle principale de rendu
 void renderLoop() {
     // Matrices de transformation
     float modelMatrix[16] = {
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f
     };
     
     float viewMatrix[16] = {
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.7f, -0.7f, 0.0f,
         0.0f, 0.7f, 0.7f, 0.0f,
         0.0f, -5.0f, -10.0f, 1.0f
     };
     
     // Matrice de projection perspective
     float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
     float fov = 45.0f * 3.14159f / 180.0f;
     float near = 0.1f;
     float far = 100.0f;
     float f = 1.0f / tanf(fov / 2.0f);
     
     float projectionMatrix[16] = {
         f / aspectRatio, 0.0f, 0.0f, 0.0f,
         0.0f, f, 0.0f, 0.0f,
         0.0f, 0.0f, (far + near) / (near - far), -1.0f,
         0.0f, 0.0f, (2.0f * far * near) / (near - far), 0.0f
     };
     
     // Paramètres pour l'eau
     float lightPos[3] = {5.0f, 10.0f, 5.0f};
     float viewPos[3] = {0.0f, 5.0f, 10.0f};
     float waterDeepColor[3] = {0.0f, 0.1f, 0.3f};
     float waterShallowColor[3] = {0.0f, 0.5f, 0.8f};
     float specularColor[3] = {1.0f, 1.0f, 1.0f};
     
     // Définition de la couleur d'arrière-plan
     glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
     
     // Sélection du programme shader
     glUseProgram(shaderProgram);
     
     // Définition des uniformes qui ne changent pas
     glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, modelMatrix);
     glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix);
     glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projectionMatrix);
     
     glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, lightPos);
     glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, viewPos);
     glUniform3fv(glGetUniformLocation(shaderProgram, "waterDeepColor"), 1, waterDeepColor);
     glUniform3fv(glGetUniformLocation(shaderProgram, "waterShallowColor"), 1, waterShallowColor);
     glUniform3fv(glGetUniformLocation(shaderProgram, "specularColor"), 1, specularColor);
     
     // Liaison de la texture de bruit de Perlin
     glActiveTexture(GL_TEXTURE0);
     glBindTexture(GL_TEXTURE_2D, perlinTexture);
     glUniform1i(glGetUniformLocation(shaderProgram, "perlinTex"), 0);
     
     // Boucle principale
     while (!glfwWindowShouldClose(window)) {
         // Mise à jour du temps pour l'animation
         time_value = (float)glfwGetTime();
         
         // Effacement des buffers
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         
         // Mise à jour du temps dans le shader
         glUniform1f(glGetUniformLocation(shaderProgram, "time"), time_value);
         
         // Dessin du maillage
         glBindVertexArray(vao);
         glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
         
         // Échange des buffers et traitement des événements
         glfwSwapBuffers(window);
         glfwPollEvents();
     }
 }
 
 // Nettoyage des ressources
 void cleanup() {
     glDeleteVertexArrays(1, &vao);
     glDeleteBuffers(1, &vbo);
     glDeleteBuffers(1, &ebo);
     glDeleteTextures(1, &perlinTexture);
     glDeleteProgram(shaderProgram);
     
     glfwTerminate();
 }ification de la compilation du vertex shader
     int success;
     char infoLog[512];
     glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
     if (!success) {
         glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
         fprintf(stderr, "Erreur de compilation du vertex shader: %s\n", infoLog);
     }
     
     // Fragment shader
     GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
     glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
     glCompileShader(fragmentShader);
     
     // Vér