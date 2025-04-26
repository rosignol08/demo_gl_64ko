/*!\file balle_song.c
 * \brief Simple 3D scene with a ball and light
 * \author OpenAI Assistant
 * \date April 23, 2025
 */

 #include <GL4D/gl4duw_SDL2.h>
 #include <GL4D/gl4dm.h>
 #include <GL4D/gl4dg.h>
 #include <GL4D/gl4dh.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <assert.h>
 #include <stdbool.h>
 
 /* Function prototypes */
 static void init(void);
 static void resize(int width, int height);
 static void draw(void);
 static void quit(void);
 
 /* Global variables */
 static GLuint _wW = 800, _wH = 600;
 static GLuint _sphereId = 0;
 static GLuint _sphereId2 = 0;
 static GLuint _quadId = 0;
 static GLuint _pId = 0;
 
 // post traitement
 static GLuint _fboId = 0;
 static GLuint _texId = 0;
 static GLuint _depthTexId = 0;
 static GLuint _postProcessProgramId = 0;
 static GLuint _screenQuadId = 0;
 static GLuint _bloomtexture = 0;
 int effets = 0;
 float teste = 0;
 
 void balle_song(int state)
 {
     switch (state)
     {
     case GL4DH_INIT:
         init();
         return;
     case GL4DH_FREE:
         if (_fboId)
         {
             glDeleteFramebuffers(1, &_fboId);
             _fboId = 0;
         }
 
         if (_texId)
         {
             glDeleteTextures(1, &_texId);
             _texId = 0;
         }
 
         if (_depthTexId)
         {
             glDeleteTextures(1, &_depthTexId);
             _depthTexId = 0;
         }
         return;
     case GL4DH_UPDATE_WITH_AUDIO:
         return;
     default: /* GL4DH_DRAW */
         draw();
         return;
     }
 }
 
 /* Initialize the scene */
 void init(void)
 {
     /* Enable depth testing for proper 3D rendering */
     glEnable(GL_DEPTH_TEST);
     /* Create shader program */
     _pId = gl4duCreateProgram("<vs>shaders/ball.vs", "<fs>shaders/ball.fs", NULL);
 
     /* Créer le shader pour le post-processing */
     _postProcessProgramId = gl4duCreateProgram("<vs>shaders/post.vs", "<fs>shaders/post.fs", NULL);

     /* Créer un quad plein écran pour le post-processing */
     _screenQuadId = gl4dgGenQuadf();
 
     /* Créer le FBO et les textures */
     glGenFramebuffers(1, &_fboId);
     glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
 
     /* Créer la texture de couleur */
     glGenTextures(1, &_texId);
     glBindTexture(GL_TEXTURE_2D, _texId);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texId, 0);
 
     /* Créer la texture de profondeur */
     glGenTextures(1, &_depthTexId);
     glBindTexture(GL_TEXTURE_2D, _depthTexId);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexId, 0);
    // Créer la texture de bloom
    glGenTextures(1, &_bloomtexture);
    glBindTexture(GL_TEXTURE_2D, _bloomtexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _wW, _wH, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Attacher au FBO comme COLOR_ATTACHMENT1
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _bloomtexture, 0);

    // Dire à OpenGL qu'on utilise deux sorties de couleur
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
     /* Vérifier que le FBO est complet */
     if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
     {
         fprintf(stderr, "Framebuffer incomplet!\n");
     }
     
     /* Revenir au framebuffer par défaut */
     glBindFramebuffer(GL_FRAMEBUFFER, 0);
 
     /* Create a sphere for the ball */
     _sphereId = gl4dgGenSpheref(30, 30);
     // pour le sol
     _quadId = gl4dgGenCubef(); // gl4dgGenQuadf();
     // orbes de lumiere
     _sphereId2 = gl4dgGenSpheref(20, 20);
 
     /* Enable depth testing for proper 3D rendering */
     glEnable(GL_DEPTH_TEST);
 
     /* Create matrices for projection, model and view transformations */
     gl4duGenMatrix(GL_FLOAT, "projection");
     gl4duGenMatrix(GL_FLOAT, "model");
     gl4duGenMatrix(GL_FLOAT, "view");
 
     /* Set up initial window size */
     resize(_wW, _wH);
 }
 
 /* Handle window resize */
 static void resize(int width, int height)
 {
     GLfloat ratio;
     _wW = width;
     _wH = height;
 
     /* Redimensionner les textures du FBO */
     if (_texId)
     {
         glBindTexture(GL_TEXTURE_2D, _texId);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
     }
 
     if (_depthTexId)
     {
         glBindTexture(GL_TEXTURE_2D, _depthTexId);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, _wW, _wH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
     }
 
     glViewport(0, 0, _wW, _wH);
     ratio = _wW / ((GLfloat)_wH);
 
     /* Set up perspective projection */
     gl4duBindMatrix("projection");
     gl4duLoadIdentityf();
     gl4duFrustumf(-ratio, ratio, -1.0f, 1.0f, 2.0f, 100.0f);
 }
 
 /* Render the scene with a ball and light */
 void draw(void)
 {
     static double t0 = 0.0;
     static float ballY = 0.0f;
     static const float gravity = 9.8f;
     static const float dampening = 0.8f;
     static const float floorY = -2.0f;
 
     double t = gl4dGetElapsedTime() / 1000.0;
     double dt = t - t0;
     t0 = t;
 
     float lightBallX = 3.0f * sinf(t * 0.5f);
     float lightBallY = 1.0f + 0.5f * sinf(t);
     float lightBallZ = 3.0f * cosf(t);
 
     /* Set up light position for moving light */
     GLfloat lightPos[4] = {3.0f * sinf(t * 0.5f), 1 + 0.50f * sinf(t), 3.0f * cosf(t), 1.0f};
     GLfloat lightColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};   /* Lumière plus intense (valeurs > 1 pour HDR) */
 
     GLfloat light2couleur_obj[4] = {1.0f, 0.1f, 0.8f, 1.0f}; // Blanc légèrement teinté de rouge
     GLfloat light2direction[4] = {1.0f, 0.0f, 0.0f, 0.0f};
     GLfloat light2couleur[4] = {1.0f, 0.8f, 0.8f, 1.0f}; // Blanc légèrement teinté de rouge
     
     GLfloat screenX, screenY;
 
     screenX = (lightPos[0] + 5.0f) / 10.0f;
     screenY = (lightPos[1] + 5.0f) / 10.0f;
 
     GLfloat light3couleur_obj[4] = {0.8f, 0.8f, 1.0f, 1.0f}; // Blanc légèrement teinté de bleu
     GLfloat light3direction[4] = {0.0f, -1.0f, 0.0f, 0.0f};
     GLfloat light3couleur[4] = {0.8f, 0.8f, 1.0f, 1.0f}; // Blanc légèrement teinté de bleu
     
     /* Clear the screen */
     glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Important : spécifier les deux buffers de sortie
unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
glDrawBuffers(2, attachments);
     glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
     /* Activate shader program */
     glUseProgram(_pId);
     /* Set up camera position */
     gl4duBindMatrix("view");
     gl4duLoadIdentityf();
     // gl4duLookAtf(0.0f, 1.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
     gl4duLookAtf(0.0f, 1.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
 
     /* Fixed position for ball - no physics */
     ballY = 0.0f;        /* Keep the ball in the middle */
 
     // Réinitialiser le paramètre "isEmissive" pour les autres objets
     glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
     glUniform4fv(glGetUniformLocation(_pId, "lightColor"), 1, lightColor);
     glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
 
     // Configurer la lumière directionnelle émise par un mur
     glUniform1i(glGetUniformLocation(_pId, "useSecondLight"), 1);
     
     // couleur de la lumière directionnelle
     glUniform4fv(glGetUniformLocation(_pId, "secondLightPosition"), 1, light2direction);
     glUniform4fv(glGetUniformLocation(_pId, "secondLightColor"), 1, light2couleur);
     // Ajouter ces lignes
     glUniform1i(glGetUniformLocation(_pId, "secondLightType"), 0); // 0 pour directionnelle
 
 
     //troisième lumière directionnelle
     glUniform1i(glGetUniformLocation(_pId, "useThirdLight"), 1);
     glUniform4fv(glGetUniformLocation(_pId, "thirdLightPosition"), 1, light3direction);
     glUniform4fv(glGetUniformLocation(_pId, "thirdLightColor"), 1, light3couleur);
     glUniform1i(glGetUniformLocation(_pId, "thirdLightType"), 0);  // 0 pour directionnelle
 
 
     // Draw a box with 6 quads (floor, ceiling, and 4 walls)
     gl4duBindMatrix("model");
 
     //truc pour les murs
     GLfloat boxColor[4] = {0.3f, 0.3f, 0.3f, 1.0f};
     glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, boxColor);
     glDisable(GL_CULL_FACE);
 
     // 1. Floor
     gl4duLoadIdentityf();
     gl4duTranslatef(0.0f, floorY - 0.5f, 0.0f);
     gl4duScalef(5.0f, 0.1f, 5.0f);
     gl4duSendMatrices();
     gl4dgDraw(_quadId);
 
     // 2. Ceiling
     gl4duLoadIdentityf();
     gl4duTranslatef(0.0f, floorY + 5.0f, 0.0f);
     gl4duScalef(5.0f, 0.1f, 5.0f);
     gl4duSendMatrices();
     gl4dgDraw(_quadId);
 
     // Rendre ce mur émissif
     glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1);
     // Couleur de la lumière émise par le mur
     glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, light2couleur_obj);
 
     // 3. Left wall
     gl4duLoadIdentityf();
     gl4duTranslatef(-5.0f, floorY + 2.5f, 0.0f);
     gl4duScalef(0.1f, 5.0f, 5.0f);
     gl4duSendMatrices();
     gl4dgDraw(_quadId);
     glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
     glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, boxColor);
 
     // 4. Right wall
     gl4duLoadIdentityf();
     gl4duTranslatef(5.0f, floorY + 2.5f, 0.0f);
     gl4duScalef(0.1f, 5.0f, 5.0f);
     gl4duSendMatrices();
     gl4dgDraw(_quadId);
 
     // 5. Front wall
     gl4duLoadIdentityf();
     gl4duTranslatef(0.0f, floorY + 2.5f, -5.0f);
     gl4duScalef(5.0f, 5.0f, 0.1f);
     gl4duSendMatrices();
     gl4dgDraw(_quadId);
     /* Draw the ball */
     gl4duBindMatrix("model");
     gl4duLoadIdentityf();
     gl4duTranslatef(0.0f, ballY, 0.0f);
     gl4duScalef(0.5f, 0.5f, 0.5f);
     glEnable(GL_CULL_FACE);
 
     /* Send matrices to the shader */
     gl4duSendMatrices();
 
     //la balle de base
     GLfloat ambientColor[4] = {0.1f, 0.1f, 0.1f, 1.0f}; /* Dark blue ambient */
     GLfloat ballColor[4] = {0.8f, 0.2f, 0.2f, 1.0f};    /* Red ball */
     GLfloat shininess = 64.0f;                          // Valeur de brillance (plus c'est élevé, plus le reflet est concentré)
 
     // envoie des truc au shader
     glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, ballColor);
     glUniform4fv(glGetUniformLocation(_pId, "lightColor"), 1, lightColor);
     glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
     glUniform4fv(glGetUniformLocation(_pId, "ambientColor"), 1, ambientColor);
     glUniform1f(glGetUniformLocation(_pId, "shininess"), shininess);
     glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
 
     // dessine la sphère
     gl4dgDraw(_sphereId);
 
     /* Draw the floor as a scaled quad */
     gl4duLoadIdentityf();
     // Faire tourner la balle lumineuse autour de la scène
     gl4duTranslatef(lightBallX, lightBallY, lightBallZ);
     gl4duScalef(0.2f, 0.2f, 0.2f); // Une balle plus petite
 
     // Mettre à jour la position de la lumière pour qu'elle suive la balle émissive
     lightPos[0] = lightBallX;
     lightPos[1] = lightBallY;
     lightPos[2] = lightBallZ;
     lightPos[3] = 1.0f;
 
     gl4duSendMatrices();
     // Couleur jaune-orangé pour la balle lumineuse
     GLfloat lightBallColor[4] = {1.0f, 0.9f, 0.6f, 4.0f};
     glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, lightBallColor);
     glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
     glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1); // Cette balle est émissive
     glUniform1i(glGetUniformLocation(_pId, "lightType"), 1);
 
     // Dessiner la balle lumineuse
     gl4dgDraw(_sphereId2);
 
     // 6. Back wall
     // gl4duLoadIdentityf();
     // gl4duTranslatef(0.0f, floorY + 2.5f, -5.0f);
     // gl4duScalef(5.0f, 5.0f, 0.1f);
     // gl4duSendMatrices();
     // gl4dgDraw(_quadId);
 
     teste += 0.05f;
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_postProcessProgramId);
    // Configurer le post-processeur pour combiner la scène originale et le bloom
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, _texId);  // Texture de la scène originale
glUniform1i(glGetUniformLocation(_postProcessProgramId, "screenTexture"), 0);
//pour le bloom
// Ajouter ces lignes:
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, _bloomtexture);
glUniform1i(glGetUniformLocation(_postProcessProgramId, "bloomTexture"), 1);

     int numLights = 1;
     glUniform1i(glGetUniformLocation(_postProcessProgramId, "numLights"), numLights);
     glUniform2f(glGetUniformLocation(_postProcessProgramId, "lightBallPosition"), screenX, screenY);
     glUniform1i(glGetUniformLocation(_postProcessProgramId, "screenTexture"), 0);
 
     /* Paramètres optionnels pour les effets */
     glUniform1i(glGetUniformLocation(_postProcessProgramId, "effect"), effets);
     glUniform1f(glGetUniformLocation(_postProcessProgramId, "time"), t);
     glUniform2f(glGetUniformLocation(_postProcessProgramId, "resolution"), _wW, _wH);
    
     
    /* Activer la texture générée */
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
     /* Dessiner un quad plein écran */
     gl4dgDraw(_screenQuadId);
     /* Update effect every second */
     static double lastEffectChange = 0.0;
     double currentTime = gl4dGetElapsedTime() / 1000.0;
 
     if (currentTime - lastEffectChange >= 1.0)
     { /* Check if 1 second has passed */
         // effets = (effets + 1) % 5;  /* Increment and cycle from 0 to 4 */
         lastEffectChange = currentTime;
     }
     effets = 8;
     /* Disable shader */
     glUseProgram(0);
 }
 
 /* Cleanup function */
 void quit(void)
 {
     gl4duClean(GL4DU_ALL);
 }


 
     /*
     //un tableau de positions si on veut plusieurs lumières
 GLfloat lightPosArray[6]; // 3 lumières x 2 coordonnées (x,y)
 
 // Première lumière (la boule mobile)
 lightPosArray[0] = (lightPos[0] + 5.0f) / 10.0f; // X
 lightPosArray[1] = (lightPos[1] + 5.0f) / 10.0f; // Y
 
 // Deuxième lumière (fixe)
 lightPosArray[2] = 0.1f; // X
 lightPosArray[3] = 0.7f; // Y
 
 // Troisième lumière (mobile)
 lightPosArray[4] = 0.5f + 0.3f * sinf(t); // X
 lightPosArray[5] = 0.2f; // Y
 
 */