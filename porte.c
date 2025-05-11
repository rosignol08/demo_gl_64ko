/*!\file porte.c
 * \brief GL4Dummies, simple shader demo
 * \author Modified from existing code
 * \date April 20, 2025
 */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>

static void init(void);
static void draw(void);

/* Variables pour le rendu OpenGL */
static GLuint _pid = 0;
static GLuint _quad = 0;

void shader_test(int state)
{
    switch(state)
    {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        if (_quad) {
            _quad = 0;
        }
        if (_pid) {
            glDeleteProgram(_pid);
            _pid = 0;
        }
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        return;
    default: /* GL4DH_DRAW */
        draw();
        return;
    }
}

static void init(void)
{
    _pid = gl4duCreateProgram("<vs>shaders/test.vs", "<fs>shaders/test.fs", NULL);
    _quad = gl4dgGenQuadf();
}

static void draw(void)
{
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0;
    double dt = t - t0;
    t0 = t;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(_pid);
    glUniform1f(glGetUniformLocation(_pid, "time"), (float)t);
    glUniform2f(glGetUniformLocation(_pid, "resolution"), (float)1280, (float)720);

    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    gl4duSendMatrices();

    gl4dgDraw(_quad);
    glUseProgram(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}
/*
triangle de https://github.com/marklundin/glsl-sdf-primitives/blob/master/udTriangle.glsl*/