/*!\file montagne.c
 * \brief GL4Dummies, scene de montagne
 * \author Romaric chhaffray
 * \date 28 04 2025
 */

#include <GL4D/gl4du.h>
#include <GL4D/gl4df.h>
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <string.h>
#include <stdlib.h>
//#include <time.h>
//#define USE_MINIFIED_SHADER //faut utiliser https://ctrl-alt-test.fr/minifier/?main pour réduire la taille des shaders
/* ---- Début du code de noise.c ---- */

static GLuint permTexId = 0, gradTexId = 0;
static int perm[256];
static int grad3[16][3];
static int grad4[32][4];

/* Initialize permutation and gradients with random values */
static void initRandomPermAndGrad(void)
{
    int i, j;

    /* Seed the random number generator */
    srand(42);

    /* Generate random permutation array */
    for (i = 0; i < 256; i++)
    {
        perm[i] = rand() % 256;
    }

    /* Generate random 3D gradient vectors */
    for (i = 0; i < 16; i++)
    {
        for (j = 0; j < 3; j++)
        {
            /* Generate values between -1 and 1 */
            grad3[i][j] = (rand() % 3) - 1;
        }
        /* Ensure we don't have a zero vector */
        if (grad3[i][0] == 0 && grad3[i][1] == 0 && grad3[i][2] == 0)
        {
            grad3[i][rand() % 3] = (rand() % 2) * 2 - 1; /* -1 or 1 */
        }
    }

    /* Generate random 4D gradient vectors */
    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < 4; j++)
        {
            /* Generate values between -1 and 1 */
            grad4[i][j] = (rand() % 3) - 1;
        }
        /* Ensure we don't have a zero vector */
        if (grad4[i][0] == 0 && grad4[i][1] == 0 && grad4[i][2] == 0 && grad4[i][3] == 0)
        {
            grad4[i][rand() % 4] = (rand() % 2) * 2 - 1; /* -1 or 1 */
        }
    }
}

/* Fonctions de gestion du bruit de Perlin */
static void initNoiseTextures(void)
{
    initRandomPermAndGrad();
    int i, j, k, i8;
    GLubyte *buffer, v;

    if (permTexId || gradTexId)
        return;

    buffer = malloc((1 << 18) /* 4 * 256 * 256 */ * sizeof *buffer);
    assert(buffer);

    for (i = 0; i < 128; i++)
    {
        i8 = i << 7;
        for (j = 0; j < 128; j++)
        {
            k = (i8 + j) << 2;
            v = perm[(j + perm[i]) & 0xFF];
            buffer[k + 0] = (grad4[v & 0x1F][0] << 6) + 64;
            buffer[k + 1] = (grad4[v & 0x1F][1] << 6) + 64;
            buffer[k + 2] = (grad4[v & 0x1F][2] << 6) + 64;
            buffer[k + 3] = (grad4[v & 0x1F][3] << 6) + 64;
        }
    }
    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &gradTexId);
    glBindTexture(GL_TEXTURE_2D, gradTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    for (i = 0; i < 256; i++)
    {
        i8 = i << 8;
        for (j = 0; j < 256; j++)
        {
            k = (i8 + j) << 2;
            buffer[k + 3] = (v = perm[(j + perm[i]) & 0xFF]);
            buffer[k + 0] = (grad3[v & 0x0F][0] << 6) + 64;
            buffer[k + 1] = (grad3[v & 0x0F][1] << 6) + 64;
            buffer[k + 2] = (grad3[v & 0x0F][2] << 6) + 64;
        }
    }
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &permTexId);
    glBindTexture(GL_TEXTURE_2D, permTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glActiveTexture(GL_TEXTURE0);

    free(buffer);
}

static void useNoiseTextures(GLuint pid, int shift)
{
    glActiveTexture(GL_TEXTURE1 + shift);
    glBindTexture(GL_TEXTURE_2D, gradTexId);
    glActiveTexture(GL_TEXTURE0 + shift);
    glBindTexture(GL_TEXTURE_2D, permTexId);
    glUniform1i(glGetUniformLocation(pid, "permTexture"), shift);
    glUniform1i(glGetUniformLocation(pid, "gradTexture"), shift + 1);
    glActiveTexture(GL_TEXTURE0);
}

static void unuseNoiseTextures(int shift)
{
    glActiveTexture(GL_TEXTURE1 + shift);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + shift);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

//static void freeNoiseTextures(void)
//{
//    glDeleteTextures(1, &gradTexId);
//    glDeleteTextures(1, &permTexId);
//    permTexId = 0;
//    gradTexId = 0;
//}

/* ---- Fin du code provenant de noise.c ---- */

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void draw(void);
/*!\brief largeur et hauteur de la fenêtre */
//static int _ww = 1280, _wh = 960;
/*!\brief identifiant du (futur) GLSL program */
static GLuint _pId = 0;
/*!\brief identifiant pour une géométrie GL4D */
static GLuint _quadId = 0;
static GLuint _gridId = 0;
static GLuint _sphereId = 0;

/*!\brief identifiant de framebuffer object */
static GLuint _fboId = 0;
static GLuint _texId = 0;


/* Variables pour la position de la caméra, le point regardé et le vecteur up */
static GLfloat eyeX = 0.0f, eyeY = 0.0f, eyeZ = 0.0f;
float test = 2.0f;
/* booléen pour bruit ou pas de bruit */
//static int _noise = 0;

void montagne(int state) {
        
    switch(state) {
    case GL4DH_INIT:
      /* INITIALISEZ VOTRE ANIMATION (SES VARIABLES <STATIC>s) */
      init();
      return;
    case GL4DH_FREE:
      /* LIBERER LA MEMOIRE UTILISEE PAR LES <STATIC>s */
      return;
    case GL4DH_UPDATE_WITH_AUDIO:
      /* METTRE A JOUR VOTRE ANIMATION EN FONCTION DU SON */
      return;
    default: /* GL4DH_DRAW */
      /* JOUER L'ANIMATION */
      draw();
      return;
    }
}

/*!\brief initialise les paramètres OpenGL et les données. */
void init(void)
{
    //const char *noiseFragmentShader = NULL;
    //const char *noiseVertexShader = NULL;
    
    _pId = gl4duCreateProgram("<vs>shaders/lum_montagne.vs", "<fs>shaders/lum_montagne.fs", NULL);
    /* générer le terrain */
    GLfloat *heightmap = gl4dmTriangleEdge(33, 33, 0.6f);
    /* Créer une grid */
    _gridId = gl4dgGenGrid2dFromHeightMapf(33, 33, heightmap);
    free(heightmap);
    _sphereId = gl4dgGenSpheref(33, 33);
    _quadId = gl4dgGenQuadf();
    /* Set de la couleur (RGBA) d'effacement OpenGL */
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    /* activation du test de profondeur afin de prendre en compte la
     * notion de devant-derrière. */
    glEnable(GL_DEPTH_TEST);
    /* Création des matrices GL4Dummies, une pour la projection, une
     * pour la modélisation et une pour la vue */
    //gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    //gl4duBindMatrix("projectionMatrix");
    //gl4duLoadIdentityf();
    //gl4duFrustumf(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    //
    //gl4duGenMatrix(GL_FLOAT, "modelMatrix");
    //gl4duGenMatrix(GL_FLOAT, "viewMatrix");
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    gl4duGenMatrix(GL_FLOAT, "modelMatrix");
    gl4duGenMatrix(GL_FLOAT, "viewMatrix");
    initNoiseTextures();

    glGenFramebuffers(1, &_fboId);
    glGenTextures(1, &_texId);
    glBindTexture(GL_TEXTURE_2D, _texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 960, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
}
/* Fonction pour simplifier le contrôle de la caméra */
static void setupCamera(float posX, float posY, float posZ, float rotX, float rotY, float rotZ) {
    // Calcul du vecteur de direction à partir des angles de rotation
    float dirX = cosf(rotY) * cosf(rotX);
    float dirY = sinf(rotX);
    float dirZ = sinf(rotY) * cosf(rotX);
    
    // Point regardé = position + direction
    float targetX = posX + dirX;
    float targetY = posY + dirY;
    float targetZ = posZ + dirZ;
    
    // Vecteur up (dépend de l'angle Z pour permettre le "roll")
    float upX = sinf(rotZ) * sinf(rotY);
    float upY = cosf(rotZ);
    float upZ = -sinf(rotZ) * cosf(rotY);
    
    // Appel à gl4duLookAtf avec les paramètres calculés
    gl4duLookAtf(posX, posY, posZ, targetX, targetY, targetZ, upX, upY, upZ);
}
/*!\brief Cette fonction dessine dans le contexte OpenGL actif. */
void draw(void)
{
    static GLfloat angle = 0.0f;
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
    t0 = t;

    //glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _texId, 0);
    //gl4dgDraw(_quadId);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* effacement du buffer de couleur et de profondeur */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* activation du programme _pId */
    glUseProgram(_pId);
    /* lier la matrice de projection */
    //gl4duBindMatrix("projectionMatrix");
    /* lier la matrice vue */
    gl4duBindMatrix("viewMatrix");
    /* Charger la matrice identité */
    gl4duLoadIdentityf();
    /* Composer la matrice vue avec la caméra */
    //gl4duLookAtf(0.0f, 2.0f, 2.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    
    eyeX = 0.50f, eyeY = 0.50f, eyeZ = 0.0f;
     
    //setupCamera(eyeX, eyeY, eyeZ, 0.0f, 0.0f, test);
    gl4duLookAtf(0.0f, 3.5f, test, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    //eyeY += 0.0005f;
    test += 0.005f;
    /* lier la matrice modèle */
    gl4duBindMatrix("modelMatrix");
    /* Charger la matrice identité */
    gl4duLoadIdentityf();
    /* Mise à l'échelle du terrain */
    gl4duScalef(10.0f, 5.0f, 10.0f);
    /* Envoyer les matrices */
    gl4duSendMatrices();

    /* Passer le temps au shader pour l'animation */
    glUniform1f(glGetUniformLocation(_pId, "temps"), t * 0.1f);

    /* Configuration des lumières */
    /* lumière 0 */
    glUniform4f(glGetUniformLocation(_pId, "Lp0"), -0.3f, 6.5f, 0.5f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "l0diffus"), 0.5f, 0.5f, 1.0f, 1.0f);      // Bleu clair
    glUniform4f(glGetUniformLocation(_pId, "l0speculaire"), 0.8f, 0.8f, 0.80f, 1.0f); // Bleu clair brillant

    /* lumière 1 - rotation circulaire */
    glUniform4f(glGetUniformLocation(_pId, "Lp1"), 0.0f, sin(-1 + t * 0.3f) * 8.0f, -3.0f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "l1diffus"), 0.90f, 0.90f, 0.90f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "l1speculaire"), 0.6f, 0.6f, 0.60f, 1.0f); // couleur du reflet

    /* lumière ambiante */
    glUniform4f(glGetUniformLocation(_pId, "lambient"), 1.0f, 1.0f, 1.0f, 1.0f);

    /* Matériaux */
    glUniform4f(glGetUniformLocation(_pId, "sambient"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "sdiffus"), 0.50f, 0.50f, 0.5f, 1.0f); // couleur du terrain
    glUniform4f(glGetUniformLocation(_pId, "sspeculaire"), 0.1f, 0.1f, 0.1f, 0.20f);

    /* Activer les textures de bruit pour le terrain et le ciel */
    useNoiseTextures(_pId, 0);

    /* Dessiner le terrain */
    //gl4dgDraw(_gridId);
    
    /* Use proper culling for the terrain to improve performance */
    //glCullFace(GL_BACK);
    
    glEnable(GL_DEPTH_TEST);//faut activer ça
    glEnable(GL_CULL_FACE);
    gl4dgDraw(_gridId);
    //glEnable(GL_DEPTH_TEST);

    /* Configurer la sphère pour le ciel */
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duScalef(15.0f, 15.0f, 15.0f);
    gl4duSendMatrices();

    /* Désactiver le culling pour voir l'intérieur de la sphère */
    glDisable(GL_CULL_FACE);

    /* Dessiner le ciel avec les nuages */
    glUniform1i(glGetUniformLocation(_pId, "sky"), 1);
    gl4dgDraw(_sphereId);
    glUniform1i(glGetUniformLocation(_pId, "sky"), 0);

    /* Réactiver le culling */
    glEnable(GL_CULL_FACE);

    /* Désactiver les textures de bruit */
    unuseNoiseTextures(0);

    /* Désactiver le programme shader */
    glUseProgram(0);

    /* Mise à jour de l'angle pour animation */
    angle += 18.0f * dt;
}

/* Nettoyage à la sortie */
//void quit(void)
//{
//    if (_fboId)
//    {
//        glDeleteFramebuffers(1, &_fboId);
//        _fboId = 0;
//    }
//    if (_texId)
//    {
//        glDeleteTextures(1, &_texId);
//        _texId = 0;
//    }
//
//    /* Libérer les textures de bruit */
//    freeNoiseTextures();
//
//    /* Nettoyer GL4Dummies */
//    gl4duClean(GL4DU_ALL);
//}