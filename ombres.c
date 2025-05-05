#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <math.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define SHADOW_MAP_SIDE 4096

static void init(void);
static void resize(int width, int height);
static void draw(void);
static void sortie(void);
static void scene(GLboolean sm, GLfloat dt, double t);

static GLuint _quadId = 0;
static GLuint _cubeId = 0;
static GLuint _teapot = 0;
static GLuint _torusId = 0, _coneId = 0;
static GLuint _smTex = 0;
static GLuint _pId = 0, _smpId = 0, _fbo = 0;
static GLuint _wW = SCREEN_WIDTH, _wH = SCREEN_HEIGHT;

void ombre_scene(int state) {
    switch(state) {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        sortie();
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        return;
    default: /* GL4DH_DRAW */
        draw();
        return;
    }
}

void init(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    _quadId = gl4dgGenQuadf();
    _cubeId = gl4dgGenCubef();
    _teapot = gl4dgGenTeapotf(70);
    _torusId = gl4dgGenTorusf(20, 10, 0.3);
    _coneId = gl4dgGenConef(20, GL_TRUE);
    // Shader pour le rendu final
    const char * imfs = "<imfs>ombre.fs</imfs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
    #else
        "#version 330 core\n"
    #endif
        "in vec3 N;\n"
        "in vec4 mvpos;\n"
        "in vec4 vsoSMCoord;\n"
        "out vec4 fragColor;\n"
        "uniform vec4 scolor;\n"
        "uniform vec4 lcolor;\n"
        "uniform vec4 lumpos;\n"
        "uniform sampler2D smTex;\n"
        "void main() {\n"
        "  vec3 n = normalize(N);\n"
        "  vec4 ambient = 0.15f * lcolor * scolor;\n"
        "  vec3 Ld = normalize(mvpos.xyz - lumpos.xyz);\n"
        "  float ild = clamp(dot(n, -Ld), 0.0, 1.0);\n"
        "  vec4 diffus = (ild * lcolor) * scolor;\n"
        "  vec3 R = normalize(reflect(Ld, n));\n"
        "  vec3 Vue = -normalize(mvpos.xyz);\n"
        "  float ils = pow(clamp(dot(R, Vue), 0, 1), 100);\n"
        "  vec4 spec = ils * lcolor;\n"
        "  vec3 projCoords = vsoSMCoord.xyz / vsoSMCoord.w;\n"
        "  if(texture(smTex, projCoords.xy).r  <  projCoords.z) {\n"
        "    diffus = spec = vec4(0.0);\n"
        "  }\n"
        "  fragColor = ambient + diffus + spec;\n"
        "}\n";

    const char * imvs = "<imvs>ombre.vs</imvs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
        "in vec3 vertexPosition;\n"
    #else
        "#version 330 core\n"
        "layout(location = 0) in vec3 vertexPosition;\n"
    #endif
        "uniform mat4 mod;\n"
        "uniform mat4 lightView;\n"
        "uniform mat4 lightProj;\n"
        "uniform mat4 proj;\n"
        "uniform mat4 view;\n"
        "out vec4 mvpos;\n"
        "out vec3 N;\n"
        "out vec4 vsoSMCoord;\n"
        "void main() {\n"
        "  N = normalize((inverse(transpose(mod)) * vec4(vertexPosition, 0)).xyz);\n"
        "  mvpos = view * mod * vec4(vertexPosition, 1.0);\n"
        "  gl_Position = proj * mvpos;\n"
        "  const mat4 bias = mat4( 0.5, 0.0, 0.0, 0.0,\n"
        "                          0.0, 0.5, 0.0, 0.0,\n"
        "                          0.0, 0.0, 0.5, 0.0,\n"
        "                          0.5, 0.5, 0.5, 1.0 );\n"
        "  vsoSMCoord  = bias * lightProj * lightView * mod * vec4(vertexPosition, 1.0);\n"
        "}\n";

    _pId = gl4duCreateProgram(imvs, imfs, NULL);

    // Shader pour la shadow map
    imfs = "<imfs>shadowMap.fs</imfs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
    #else
        "#version 330 core\n"
    #endif
        "layout (location = 0) out float depth;\n"
        "void main(void) {\n"
        "}\n";

    imvs = "<imvs>shadowMap.vs</imvs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
        "in vec3 vertexPosition;\n"
    #else
        "#version 330 core\n"
        "layout(location = 0) in vec3 vertexPosition;\n"
    #endif
        "uniform mat4 mod;\n"
        "uniform mat4 lightView;\n"
        "uniform mat4 lightProj;\n"
        "void main(void) {\n"
        "  gl_Position = lightProj * lightView * mod * vec4(vertexPosition, 1.0);\n"
        "}\n";

    _smpId = gl4duCreateProgram(imvs, imfs, NULL);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    gl4duGenMatrix(GL_FLOAT, "proj");
    gl4duGenMatrix(GL_FLOAT, "mod");
    gl4duGenMatrix(GL_FLOAT, "view");
    gl4duGenMatrix(GL_FLOAT, "lightView");
    gl4duGenMatrix(GL_FLOAT, "lightProj");

    // Création et paramétrage de la Texture de shadow map
    glGenTextures(1, &_smTex);
    glBindTexture(GL_TEXTURE_2D, _smTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Création du Framebuffer Object
    glGenFramebuffers(1, &_fbo);
    resize(_wW, _wH);
}

void resize(int width, int height) {
    GLfloat ratio;
    _wW = width;
    _wH = height;
    glViewport(0, 0, _wW, _wH);
    ratio = _wW / (GLfloat)_wH;
    gl4duBindMatrix("proj");
    gl4duLoadIdentityf();
    gl4duFrustumf(-ratio, ratio, -1.0f, 1.0f, 2.0f, 100.0f);
    gl4duBindMatrix("lightProj");
    gl4duLoadIdentityf();
    gl4duFrustumf(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 50.0f);
    //gl4duFrustumf(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);
}
void scene(GLboolean sm, GLfloat dt, double t) {
    static GLfloat x = 0.0f;
    GLfloat blanc[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat jaune[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    // Position the light at the top oscillating left to right, pointing downward
    GLfloat lumpos0[] = { 5.0f * sinf(x), 3.0f, 0.0f, 1.0f }, lumpos[4], * mat;
    
    // Increment x for the oscillation
    x += dt * 2.0f;

    gl4duBindMatrix("view");
    mat = (GLfloat *)gl4duGetMatrixData();
    MMAT4XVEC4(lumpos, mat, lumpos0);

    if(sm) {
        glCullFace(GL_FRONT);
        glUseProgram(_smpId);
        gl4duBindMatrix("lightView");
        gl4duLoadIdentityf();
        gl4duLookAtf(lumpos0[0], lumpos0[1], lumpos0[2], 0, 0, -5, 0, 1, 0);
    } else {
        glCullFace(GL_BACK);
        glUseProgram(_pId);
        glUniform4fv(glGetUniformLocation(_pId, "lcolor"), 1, blanc);
        glUniform4fv(glGetUniformLocation(_pId, "lumpos"), 1, lumpos);
    }   
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _smTex);
        glUniform1i(glGetUniformLocation(_pId, "smTex"), 0);

    // Draw the background cube in both passes
    gl4duBindMatrix("mod");
    gl4duLoadIdentityf();
  gl4duTranslatef(0.0f, 0.0f, -4.0f);
  gl4duScalef(7.0f, 1.0f, 7.0f);
  //gl4duRotatef(90.0f, 1.0f, 0.0f, 0.0f);
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_pId, "scolor"), 1, blanc);
  glUniform4fv(glGetUniformLocation(_pId, "lcolor"), 1, blanc);
  glUniform4fv(glGetUniformLocation(_pId, "lumpos"), 1, lumpos);
  gl4dgDraw(_quadId);

    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, 0.0f, 2.0f);
    gl4duScalef(0.5f, 0.5f, 0.5f);
    //gl4duRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gl4duSendMatrices();    
    gl4dgDraw(_teapot);
    
    gl4duLoadIdentityf();
    gl4duTranslatef(-1.5f, 0.0f, -0.5f);
    gl4duScalef(0.25f, 1.0f, 0.25f);
    gl4duRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gl4duSendMatrices();
    
        glUniform4fv(glGetUniformLocation(_pId, "scolor"), 1, jaune);
    
    gl4dgDraw(_torusId);

    gl4duLoadIdentityf();
    gl4duTranslatef(3.0f, 0.5f, -4.5f);
    gl4duScalef(3.0f, 3.0f, 3.0f);
    gl4duRotatef(10.0f, 1.0f, 0.0f, 0.0f);
    gl4duRotatef(t * 60.0f, 0.0f, 1.0f, 0.0f);
    gl4duSendMatrices();
        glUniform4fv(glGetUniformLocation(_pId, "scolor"), 1, blanc);
    gl4dgDraw(_teapot);
    
    glUseProgram(0);
}

void draw(void) {
    GLenum rendering = GL_COLOR_ATTACHMENT0;
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0, dt = t - t0;
    t0 = t;

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glDrawBuffer(GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _smTex, 0);
    glViewport(0, 0, SHADOW_MAP_SIDE, SHADOW_MAP_SIDE);
    glClear(GL_DEPTH_BUFFER_BIT);
    scene(GL_TRUE, dt,t);

    glDrawBuffers(1, &rendering);
    glViewport(0, 0, _wW, _wH);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl4duBindMatrix("view");
    gl4duLoadIdentityf();
    gl4duLookAtf(0, 1.8f, 6.0f, 0, 0, 0, 0.0f, 1.0f, 0);
    //gl4duLookAtf(0.0f, 0.0f, 4.0f, 0, 0, 0, 0.0f, 1.0f, 0);
    scene(GL_FALSE, dt,t);
}

void sortie(void) {
    if(_fbo) {
        glDeleteTextures(1, &_smTex);
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    gl4duClean(GL4DU_ALL);
}
