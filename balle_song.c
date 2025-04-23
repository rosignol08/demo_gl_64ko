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

/* Function prototypes */
static void init(void);
static void resize(int width, int height);
static void draw(void);
static void quit(void);

/* Global variables */
static GLuint _wW = 800, _wH = 600;
static GLuint _sphereId = 0;
static GLuint _quadId = 0;
static GLuint _pId = 0;
float teste = 2.0f;

void balle_song(int state)
{
    switch (state)
    {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
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
    /* Set background color to dark blue */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Create a sphere for the ball */
    _sphereId = gl4dgGenSpheref(30, 30);
    // pour le sol
    _quadId = gl4dgGenQuadf();
    //orbes de 

    /* Create shader program */
    _pId = gl4duCreateProgram("<vs>shaders/ball.vs", "<fs>shaders/ball.fs", NULL);

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
    static float ballVelocity = 0.0f;
    static const float gravity = 9.8f;
    static const float dampening = 0.8f;
    static const float floorY = -2.0f;

    double t = gl4dGetElapsedTime() / 1000.0;
    double dt = t - t0;
    t0 = t;

    /* Clear the screen */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    /* Set up camera position */
    gl4duBindMatrix("view");
    gl4duLoadIdentityf();
    gl4duLookAtf(0.0f, 1.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    /* Fixed position for ball - no physics */
    ballY = 0.0f;        /* Keep the ball in the middle */
    ballVelocity = 0.0f; /* No velocity */

    /* Draw the ball */
    gl4duBindMatrix("model");
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, ballY, 0.0f);
    gl4duScalef(0.5f, 0.5f, 0.5f);

    /* Activate shader program */
    glUseProgram(_pId);

    /* Set up light position for moving light */
    GLfloat lightPos[4] = {3.0f * sinf(t), 2.0f, 3.0f * cosf(t), 1.0f};
    GLfloat ballColor[4] = {0.8f, 0.2f, 0.2f, 1.0f};    /* Red ball */
    //GLfloat lightColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};   /* Slightly warm white light */
    GLfloat ambientColor[4] = {0.1f, 0.1f, 0.1f, 1.0f}; /* Dark blue ambient */
    GLfloat lightColor[4] = {1.2f, 1.2f, 1.2f, 1.0f};   /* Lumière plus intense (valeurs > 1 pour HDR) */
    GLfloat shininess = 64.0f; // Valeur de brillance (plus c'est élevé, plus le reflet est concentré)
    
    /* Send matrices to the shader */
    gl4duSendMatrices();

    /* Send colors and light position to the shader */
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, ballColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightColor"), 1, lightColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
    glUniform4fv(glGetUniformLocation(_pId, "ambientColor"), 1, ambientColor);
    glUniform1f(glGetUniformLocation(_pId, "shininess"), shininess);
    

    /* Draw the sphere */
    gl4dgDraw(_sphereId);

    /* Draw the floor as a scaled quad */
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, floorY, 0.0f);
    gl4duScalef(5.0f, 0.1f, 5.0f);
    gl4duRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    /* Send updated matrix */
    gl4duSendMatrices();

    /* Update floor color - dark gray */
    GLfloat floorColor[4] = {0.3f, 0.3f, 0.3f, 1.0f};
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, floorColor);
    glDisable(GL_CULL_FACE);
    /* Draw floor */
    gl4dgDraw(_quadId);
    glEnable(GL_CULL_FACE);
    /* Disable shader */
    glUseProgram(0);
}

/* Cleanup function */
void quit(void)
{
    gl4duClean(GL4DU_ALL);
}