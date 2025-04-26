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
static GLuint _quad2Id = 0;
static GLuint _pId = 0;

// post traitement
static GLuint _fboId = 0;
static GLuint _texId = 0;
static GLuint _depthTexId = 0;
static GLuint _postProcessProgramId = 0;
static GLuint _screenQuadId = 0;

// bloom
static GLuint _bloomFboId[2] = {0, 0}; // Two framebuffers for ping-pong
static GLuint _bloomTexId[2] = {0, 0}; // Two textures for ping-pong
static GLuint _bloomDepthTexId = 0;
static GLuint _bloomScreenQuadId = 0;

// bright pass
static GLuint _brightPassProgramId = 0;
static GLuint _blurProgramId = 0;

// combine
static GLuint _combineProgramId = 0;

int effets = 0;
float teste = 0;
GLfloat screenX, screenY;

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
        if (_bloomFboId[0] || _bloomFboId[1])
        {
            glDeleteFramebuffers(2, _bloomFboId);
            _bloomFboId[0] = _bloomFboId[1] = 0;
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _wW, _wH, 0, GL_RGB, GL_FLOAT, NULL);
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

    /* Vérifier que le FBO est complet */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Framebuffer incomplet!\n");
    }

    /* Configuration des FBOs et des textures pour le Bloom (ping-pong technique) */
    glGenFramebuffers(2, _bloomFboId);
    glGenTextures(2, _bloomTexId);
    
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, _bloomFboId[i]);
        
        /* Créer la texture de couleur pour le bloom */
        glBindTexture(GL_TEXTURE_2D, _bloomTexId[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _wW, _wH, 0, GL_RGB, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _bloomTexId[i], 0);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFboId[0]);

    /* Créer la texture de profondeur pour le bloom */
    glGenTextures(1, &_bloomDepthTexId);
    glBindTexture(GL_TEXTURE_2D, _bloomDepthTexId);
    /* Vérifier que les FBOs pour bloom sont complets */
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, _bloomFboId[i]);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "Bloom Framebuffer %d incomplet!\n", i);
        }
    }

    /* Vérifier que le FBO pour bloom est complet */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Bloom Framebuffer incomplet!\n");
    }

    /* Créer un quad plein écran pour le bloom */
    _bloomScreenQuadId = gl4dgGenQuadf();

    /* Créer le shader pour le bright pass */
    _brightPassProgramId = gl4duCreateProgram("<vs>shaders/bloom.vs", "<fs>shaders/bright_pass.fs", NULL);
    
    /* Créer le shader pour le flou */
    _blurProgramId = gl4duCreateProgram("<vs>shaders/bloom.vs", "<fs>shaders/blur.fs", NULL);

    /* Créer le shader pour la combinaison */
    _combineProgramId = gl4duCreateProgram("<vs>shaders/bloom.vs", "<fs>shaders/combine.fs", NULL);

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _wW, _wH, 0, GL_RGB, GL_FLOAT, NULL);
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

// Fonction améliorée de projection 3D vers 2D en utilisant une approche similaire à OpenCV
void projeterPoint3D(GLfloat *point3D, GLfloat *screenPos) {
    // Obtenir les matrices actuelles
    gl4duBindMatrix("model");
    GLfloat *modelMatrix = gl4duGetMatrixData();
    gl4duBindMatrix("view");
    GLfloat *viewMatrix = gl4duGetMatrixData();
    gl4duBindMatrix("projection");
    GLfloat *projMatrix = gl4duGetMatrixData();

    // Position 3D en coordonnées homogènes
    GLfloat pos[4] = {point3D[0], point3D[1], point3D[2], 1.0f};

    // 1. Appliquer la transformation modèle (objet -> monde)
    GLfloat modelPos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            modelPos[i] += modelMatrix[i*4+j] * pos[j];

    // 2. Appliquer la transformation vue (monde -> caméra)
    GLfloat viewPos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            viewPos[i] += viewMatrix[i*4+j] * modelPos[j];

    // 3. Appliquer la transformation projection (caméra -> NDC)
    GLfloat clipPos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            clipPos[i] += projMatrix[i*4+j] * viewPos[j];

    // 4. Division perspective (NDC)
    if(clipPos[3] != 0.0f) {
        GLfloat invW = 1.0f / clipPos[3];
        screenPos[0] = clipPos[0] * invW;
        screenPos[1] = clipPos[1] * invW;
        screenPos[2] = clipPos[2] * invW;
    } else {
        screenPos[0] = screenPos[1] = screenPos[2] = 0.0f;
    }

    // 5. Conversion NDC [-1,1] → écran [0,1]
    screenPos[0] = (screenPos[0] + 1.0f) * 0.5f;
    screenPos[1] = (screenPos[1] + 1.0f) * 0.5f;
  
    // Ajuster la position verticale (déplacer vers le bas)
    //screenPos[1] = screenPos[1] + 0.2f; //pour ajuster le décalage
    //screenPos[0] = screenPos[0] + 0.2f; //pour ajuster le décalage
    
    // 5. Vérification pour savoir si le point est devant la caméra
    // Si z dans l'espace NDC est entre -1 et 1, le point est visible
    bool isVisible = (screenPos[2] >= -1.0f && screenPos[2] <= 1.0f &&
                     screenPos[0] >= 0.0f && screenPos[0] <= 1.0f &&
                     screenPos[1] >= 0.0f && screenPos[1] <= 1.0f);
    
    // Si le point n'est pas visible, marquer avec des coordonnées hors écran
    if (!isVisible) {
        screenPos[0] = -1.0f;
        screenPos[1] = -1.0f;
    }
}

// Fonction pour multiplier deux matrices 4x4
void multiplyMatrices(const GLfloat *a, const GLfloat *b, GLfloat *result) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i*4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                result[i*4 + j] += a[i*4 + k] * b[k*4 + j];
            }
        }
    }
}

/* Render the scene with a ball and light */
void draw(void){
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

    //screenX = (lightPos[0] + 5.0f) / 10.0f;
    //screenY = (lightPos[1] + 5.0f) / 10.0f;
    
    GLfloat lightWorldPos[4] = {lightPos[0], lightPos[1], lightPos[2], lightPos[3]};
    GLfloat screenPos[4];
    
    GLfloat light3couleur_obj[4] = {0.8f, 0.8f, 1.0f, 1.0f}; // Blanc légèrement teinté de bleu
    GLfloat light3direction[4] = {0.0f, -1.0f, 0.0f, 0.0f};
    GLfloat light3couleur[4] = {0.8f, 0.8f, 1.0f, 1.0f}; // Blanc légèrement teinté de bleu
    
    /* Clear the screen */
    glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    GLfloat lightBallColor[4] = {30.0f, 28.0f, 18.0f, 1.0f}; // Encore plus lumineux
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, lightBallColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1); // Cette balle est émissive
    glUniform1i(glGetUniformLocation(_pId, "lightType"), 1);

    //la balle lumineuse
    gl4dgDraw(_sphereId2);

    teste += 0.05f;
    
    /* PHASE 2: Bright pass */
    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFboId[0]);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // IMPORTANT: définir la couleur d'effacement
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_brightPassProgramId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texId);
    glUniform1i(glGetUniformLocation(_brightPassProgramId, "sceneTexture"), 0);
    gl4dgDraw(_bloomScreenQuadId);

    /* PHASE 3: Blur ping-pong */
    bool horizontal = true;
    int blur_passes = 15; // 5-10 passes donnent de bons résultats

    glUseProgram(_blurProgramId);

    for(int i = 0; i < blur_passes; ++i) {
        // Toujours écrire dans le framebuffer opposé à celui qu'on lit
        glBindFramebuffer(GL_FRAMEBUFFER, _bloomFboId[horizontal ? 1 : 0]);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // <-- AVANT le clear
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUniform1i(glGetUniformLocation(_blurProgramId, "horizontal"), horizontal ? 1 : 0);
        
        // Lire depuis l'autre texture
        glActiveTexture(GL_TEXTURE0);
        if (i == 0) {
            // Première passe: prendre le résultat du bright pass
            glBindTexture(GL_TEXTURE_2D, _bloomTexId[0]);
        } else {
            // Passes suivantes: lire l'autre buffer
            glBindTexture(GL_TEXTURE_2D, _bloomTexId[horizontal ? 0 : 1]);
        }
        glUniform1i(glGetUniformLocation(_blurProgramId, "image"), 0);
        
        // Dessiner le quad
        gl4dgDraw(_bloomScreenQuadId);
        
        // Inverser l'orientation pour la passe suivante
        horizontal = !horizontal;
    }

    /* PHASE 4: Combine */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_combineProgramId);

    // Scène originale
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texId);
    glUniform1i(glGetUniformLocation(_combineProgramId, "sceneTexture"), 0);

    // Bloom flouté - utiliser le buffer qui contient le dernier résultat du blur
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _bloomTexId[horizontal ? 0 : 1]); // Le buffer opposé au dernier horizontal
    glUniform1i(glGetUniformLocation(_combineProgramId, "blurredTexture"), 1);

    // Paramètres additionnels
    glUniform1f(glGetUniformLocation(_combineProgramId, "bloomIntensity"), 15.5f); // Plus intense
    glUniform1f(glGetUniformLocation(_combineProgramId, "exposure"), 1.0f);

    // Dessiner le résultat final
    gl4dgDraw(_bloomScreenQuadId);

    /* Update effect every second */
    static double lastEffectChange = 0.0;
    double currentTime = gl4dGetElapsedTime() / 1000.0;

    if (currentTime - lastEffectChange >= 1.0)
    { /* Check if 1 second has passed */
        // effets = (effets + 1) % 5;  /* Increment and cycle from 0 to 4 */
        lastEffectChange = currentTime;
    }
    effets = 7;
    /* Disable shader */
    glUseProgram(0);
}

/* Cleanup function */
void quit(void)
{
    gl4duClean(GL4DU_ALL);
}