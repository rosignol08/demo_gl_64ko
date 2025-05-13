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
#include "audioHelper.h"

#define ECHANTILLONS 1024

/* Function prototypes */
static void init(void);
static void resize(int width, int height);
static void draw(void);

// audio

/* Global variables */
static GLuint _wW = 1920, _wH = 1080;

static GLuint _sphereId = 0;
static GLuint _sphereId2 = 0;
static GLuint _sphereId3 = 0;
static GLuint _sphereId4 = 0;
static GLuint _quadId = 0;
static GLuint _pId = 0;

// post traitement
static GLuint _fboId = 0;
static GLuint _texId = 0;
static GLuint _depthTexId = 0;
static GLuint _postProcessProgramId = 0;
static GLuint _screenQuadId = 0;
static GLuint _bloomtexture = 0;

// pour le framebuffer actuel
GLint originalFBO = 0;

// pour faire grossir la sphere
static float _scale_boule = 1.5f;
float teste = 0;

// pour savoir le temps depuis le debut de la scene
static double temps_début = 0.0;

void balle_song(int state)
{
    switch (state)
    {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        // liberation de la memoire des static

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
        if (_bloomtexture)
        {
            glDeleteTextures(1, &_bloomtexture);
            _bloomtexture = 0;
        }
        if (_postProcessProgramId)
        {
            glDeleteProgram(_postProcessProgramId);
            _postProcessProgramId = 0;
        }
        if (_screenQuadId)
        {
            // gl4dgDelete(_screenQuadId);
            _screenQuadId = 0;
        }
        if (_sphereId)
        {
            // gl4dgDelete(_sphereId);
            _sphereId = 0;
        }
        if (_sphereId2)
        {
            // gl4dgDelete(_sphereId2);
            _sphereId2 = 0;
        }
        if (_sphereId3)
        {
            // gl4dgDelete(_sphereId3);
            _sphereId3 = 0;
        }
        if (_sphereId4)
        {
            // gl4dgDelete(_sphereId4);
            _sphereId4 = 0;
        }
        if (_quadId)
        {
            // gl4dgDelete(_quadId);
            _quadId = 0;
        }
        if (_pId)
        {
            glDeleteProgram(_pId);
            _pId = 0;
        }
        _wW = 0;
        _wH = 0;

        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        /* METTRE A JOUR VOTRE ANIMATION EN FONCTION DU SON */
        {
            /* METTRE A JOUR VOTRE ANIMATION EN FONCTION DU SON */
            int i, len = ahGetAudioStreamLength();
            Sint16 *s = (Sint16 *)ahGetAudioStream();
            float volume = 0.0f;
            if (len >= 2 * ECHANTILLONS)
            {
                for (i = 0; i < ECHANTILLONS; i++)
                {
                    volume += fabsf(s[i] / 32768.0f); // méthode simple
                }
                volume /= ECHANTILLONS; // moyenne
            }
            teste = volume; // <= on stocke le volume dans la variable globale "teste"
            return;
        }
    default: /* GL4DH_DRAW */

        draw();
        return;
    }
}

/* Initialize the scene */
void init(void)
{
    // je recupere le framebuffer d'origine
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFBO);
    // printf("originalFBO = %d\n", originalFBO);

    /* Create shader program */
    //_pId = gl4duCreateProgram("<vs>shaders/ball.vs", "<fs>shaders/ball.fs", NULL);

    /* Créer le shader pour le post-processing */
    //_postProcessProgramId = gl4duCreateProgram("<vs>shaders/post.vs", "<fs>shaders/post.fs", NULL);

    /* Create shader program */
    //_pId = gl4duCreateProgram("<vs>shaders/ball.vs", "<fs>shaders/ball.fs", NULL);
    // Nouvelle manière include direct avec imfs (merci julien)

    const char *imfs = "<imfs>ball.fs</imfs>\n"
#ifdef __GLES4D__
                       "#version 300 es\n"
                       "precision mediump float;\n"
#else
                       "#version 330 core\n"
#endif
                       "in vec3 normal,fragPos;\n"
                       "in vec2 texCoord;\n"
                       "layout(location=0)out vec4 FragColor;\n"
                       "layout(location=1)out vec4 BrightColor;\n"
                       "uniform vec4 ballColor,lightColor,lightPosition,ambientColor;\n"
                       "uniform float shininess;\n"
                       "uniform int isEmissive,lightType;\n"
                       "uniform vec4 secondLightColor,secondLightPosition;\n"
                       "uniform int useSecondLight,secondLightType;\n"
                       "uniform vec4 thirdLightColor,thirdLightPosition;\n"
                       "uniform int useThirdLight,thirdLightType,numLights,utiliseWater;\n"
                       "uniform float time,waveStrength,waveSpeed;\n"
                       "uniform int isWater;\n"
                       "vec3 applyWaves(vec3 position,vec3 normal)\n"
                       "{\n"
                       "  if(isWater!=1)\n"
                       "    return position;\n"
                       "  float amplitude=waveStrength*.05;\n"
                       "  normal=position;\n"
                       "  normal.y+=sin(5.*position.x+time*waveSpeed)*cos(5.*position.z+time*waveSpeed*.7)*amplitude+sin(6.5*position.z+time*waveSpeed*.8)*cos(8.5*position.x+time*waveSpeed*1.1)*amplitude*.8;\n"
                       "  return normal;\n"
                       "}\n"
                       "void main()\n"
                       "{\n"
                       "  vec3 norm=normalize(normal),lightDir;\n"
                       "  float attenuation=1.;\n"
                       "  if(lightType==1)\n"
                       "    {\n"
                       "      lightDir=normalize(vec3(lightPosition)-fragPos);\n"
                       "      float distance=length(vec3(lightPosition)-fragPos);\n"
                       "      attenuation=1./(1.+.09*distance+.032*distance*distance);\n"
                       "    }\n"
                       "  else\n"
                       "     lightDir=normalize(-vec3(lightPosition));\n"
                       "  vec3 ambient=vec3(ambientColor);\n"
                       "  float diff=max(dot(norm,lightDir),0.);\n"
                       "  vec3 diffuse=diff*vec3(lightColor),viewDir=normalize(vec3(0,0,5)-fragPos);\n"
                       "  lightDir=normalize(lightDir+viewDir);\n"
                       "  diff=pow(max(dot(norm,lightDir),0.),shininess*2.);\n"
                       "  vec3 specular=diff*vec3(lightColor);\n"
                       "  diffuse*=attenuation;\n"
                       "  specular*=attenuation;\n"
                       "  diff=pow(1.-max(dot(viewDir,norm),0.),3.);\n"
                       "  vec3 rimLight=diff*vec3(lightColor)*.3*attenuation,secondaryLightContribution=vec3(0);\n"
                       "  if(useSecondLight==1)\n"
                       "    {\n"
                       "      vec3 secondLightDir;\n"
                       "      float secondAttenuation=1.;\n"
                       "      if(secondLightType==1)\n"
                       "        {\n"
                       "          secondLightDir=normalize(vec3(secondLightPosition)-fragPos);\n"
                       "          float secondDistance=length(vec3(secondLightPosition)-fragPos);\n"
                       "          secondAttenuation=1./(1.+.09*secondDistance+.032*secondDistance*secondDistance);\n"
                       "        }\n"
                       "      else\n"
                       "         secondLightDir=normalize(-vec3(secondLightPosition));\n"
                       "      float secondDiff=max(dot(norm,secondLightDir),0.);\n"
                       "      vec3 secondDiffuse=secondDiff*vec3(secondLightColor);\n"
                       "      secondLightDir=normalize(secondLightDir+viewDir);\n"
                       "      secondDiff=pow(max(dot(norm,secondLightDir),0.),shininess*2.);\n"
                       "      secondLightDir=secondDiff*vec3(secondLightColor);\n"
                       "      secondDiffuse*=secondAttenuation;\n"
                       "      secondLightDir*=secondAttenuation;\n"
                       "      secondaryLightContribution=secondDiffuse+secondLightDir;\n"
                       "    }\n"
                       "  vec3 thirdLightContribution=vec3(0);\n"
                       "  if(useThirdLight==1)\n"
                       "    {\n"
                       "      vec3 thirdLightDir;\n"
                       "      float thirdAttenuation=1.;\n"
                       "      if(thirdLightType==1)\n"
                       "        {\n"
                       "          thirdLightDir=normalize(vec3(thirdLightPosition)-fragPos);\n"
                       "          float thirdDistance=length(vec3(thirdLightPosition)-fragPos);\n"
                       "          thirdAttenuation=1./(1.+.09*thirdDistance+.032*thirdDistance*thirdDistance);\n"
                       "        }\n"
                       "      else\n"
                       "         thirdLightDir=normalize(-vec3(thirdLightPosition));\n"
                       "      float thirdDiff=max(dot(norm,thirdLightDir),0.);\n"
                       "      vec3 thirdDiffuse=thirdDiff*vec3(thirdLightColor);\n"
                       "      thirdLightDir=normalize(thirdLightDir+viewDir);\n"
                       "      thirdDiff=pow(max(dot(norm,thirdLightDir),0.),shininess*2.);\n"
                       "      thirdLightDir=thirdDiff*vec3(thirdLightColor);\n"
                       "      thirdDiffuse*=thirdAttenuation;\n"
                       "      thirdLightDir*=thirdAttenuation;\n"
                       "      thirdLightContribution=thirdDiffuse+thirdLightDir;\n"
                       "    }\n"
                       "  if(isEmissive==1)\n"
                       "    FragColor=vec4(vec3(ballColor)*1.5,ballColor.w);\n"
                       "  else if(utiliseWater==1)\n"
                       "    {\n"
                       "      vec3 wavyPos=applyWaves(fragPos,norm);\n"
                       "      norm=normalize(normalize(cross(applyWaves(wavyPos+vec3(.01,0,0),vec3(0,1,0))-applyWaves(wavyPos-vec3(.01,0,0),vec3(0,1,0)),applyWaves(wavyPos+vec3(0,0,.01),vec3(0,1,0))-applyWaves(wavyPos-vec3(0,0,.01),vec3(0,1,0)))));\n"
                       "      float waterRipple=sin(length(texCoord*10.)-time*2.)*.05,wavyDiff=max(dot(norm,normalize(vec3(lightPosition)-wavyPos)),0.);\n"
                       "      diffuse=wavyDiff*vec3(lightColor);\n"
                       "      wavyDiff=pow(max(dot(norm,lightDir),0.),shininess*(2.+sin(time)));\n"
                       "      specular=(1.+waterRipple*2.)*wavyDiff*vec3(lightColor);\n"
                       "      wavyPos=mix(vec3(0,.827,.98),vec3(1),pow(1.-max(dot(viewDir,norm),0.),3.));\n"
                       "      diffuse*=attenuation;\n"
                       "      specular*=attenuation;\n"
                       "      rimLight=diff*vec3(lightColor)*(.3+sin(time*1.3)*.1)*attenuation;\n"
                       "      wavyPos=mix((ambient+diffuse*(sin(time*.5)*.1+.9)+specular+rimLight)*mix(vec3(0,.5,1),vec3(0,.7,.8),sin(time*.3)*.5+.5),wavyPos,(.5+waterRipple*4.+sin(time*.8)*.1)*diff);\n"
                       "      FragColor=vec4(wavyPos,ballColor.w*(.4+waterRipple+diff*.2));\n"
                       "    }\n"
                       "  else\n"
                       "    {\n"
                       "      vec3 result=(ambient+diffuse+specular+rimLight+secondaryLightContribution+thirdLightContribution)*vec3(ballColor);\n"
                       "      FragColor=vec4(result,ballColor.w);\n"
                       "    }\n"
                       "  thirdLightContribution=FragColor.xyz;\n"
                       "  BrightColor=isEmissive==1?\n"
                       "    vec4(thirdLightContribution*3.,1):\n"
                       "    vec4(0,0,0,1);\n"
                       "}\n";

    const char *imvs = "<imvs>ball.vs</imvs>\n"
#ifdef __GLES4D__
                       "#version 300 es\n"
#else
                       "#version 330 core\n"
#endif
                       "layout(location=0)in vec3 vPosition;\n"
                       "layout(location=1)in vec3 vNormal;\n"
                       "layout(location=2)in vec2 vTexCoord;\n"
                       "uniform mat4 projection,model,view;\n"
                       "uniform int isWater;\n"
                       "uniform float waveStrength,waveSpeed,time,movementFactor,amplFactor;\n"
                       "out vec3 normal,fragPos;\n"
                       "out vec2 texCoord;\n"
                       "void main()\n"
                       "{\n"
                       "  vec3 pos=vPosition;\n"
                       "  if(isWater==1)\n"
                       "    {\n"
                       "      float dist=length(pos.xz);\n"
                       "      dist=(sin(dist*5.-time*waveSpeed*movementFactor)*exp(-dist*1.5)+sin(dist*5.-time*waveSpeed*movementFactor*1.5)*exp(-dist*2.)*.3)*waveStrength*amplFactor;\n"
                       "      pos.y+=dist;\n"
                       "      vec2 normalizedDir=normalize(pos.xz+vec2(.01));\n"
                       "      pos.xz+=normalizedDir*dist*.2;\n"
                       "    }\n"
                       "  gl_Position=projection*(view*model)*vec4(pos,1);\n"
                       "  normal=mat3(transpose(inverse(model)))*vNormal;\n"
                       "  fragPos=vec3(model*vec4(vPosition,1));\n"
                       "  texCoord=vTexCoord;\n"
                       "}\n";
    _pId = gl4duCreateProgram(imvs, imfs, NULL);
    imfs = "<imfs>post.fs</imfs>\n"
#ifdef __GLES4D__
           "#version 300 es\n"
           "precision mediump float;\n"
#else
           "#version 330 core\n"
#endif
           "in vec2 texCoords;\n"
           "layout(location=0)out vec4 fragColor;\n"
           "layout(location=1)out vec4 BrightColor;\n"
           "uniform sampler2D screenTexture,bloomTexture;\n"
           "uniform float time;\n"
           "uniform vec2 resolution;\n"
           "uniform int numLights;\n"
           "void main()\n"
           "{\n"
           "  vec3 bloomTotal=vec3(0);\n"
           "  float totalWeight=0.;\n"
           "  for(int x=-7;x<=7;x+=1)\n"
           "    for(int y=-7;y<=7;y+=1)\n"
           "      {\n"
           "        float dist=length(vec2(x,y));\n"
           "        if(dist>7)\n"
           "          continue;\n"
           "        dist=1./(1.+dist*dist*.02);\n"
           "        vec2 offset=vec2(x,y)*2./resolution;\n"
           "        bloomTotal+=texture(bloomTexture,texCoords+offset).xyz*dist;\n"
           "        totalWeight+=dist;\n"
           "      }\n"
           "  bloomTotal=bloomTotal/totalWeight*2.5+texture(screenTexture,texCoords).xyz;\n"
           "  bloomTotal/=bloomTotal+vec3(1);\n"
           "  fragColor=vec4(bloomTotal,1);\n"
           "  BrightColor=vec4(0);\n"
           "}\n";

    imvs = "<imvs>post.vs</imvs>\n"
#ifdef __GLES4D__
           "#version 300 es\n"
#else
           "#version 330 core\n"
#endif
           "layout(location=0)in vec3 aPos;\n"
           "layout(location=2)in vec2 aTexCoords;\n"
           "out vec2 texCoords;\n"
           "void main()\n"
           "{\n"
           "  texCoords=aTexCoords;\n"
           "  gl_Position=vec4(aPos,1);\n"
           "}\n";

    _postProcessProgramId = gl4duCreateProgram(imvs, imfs, NULL);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // pour pas que ca fasse des artefacts sur les bords

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
    // glBindFramebuffer(GL_FRAMEBUFFER, originalFBO);
    //  Vérifier le FBO
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Framebuffer incomplet! Error: %d (0x%x)\n", status, status);
        // Essayez avec une configuration plus simple si nécessaire
    }

    // Revenir au framebuffer par défaut
    glBindFramebuffer(GL_FRAMEBUFFER, originalFBO);

    /* Create a sphere for the ball */
    _sphereId = gl4dgGenSpheref(30, 30);
    // pour le sol
    _quadId = gl4dgGenCubef(); // gl4dgGenQuadf();
    // orbes de lumiere
    _sphereId2 = gl4dgGenSpheref(20, 20);
    _sphereId3 = gl4dgGenSpheref(20, 20);
    _sphereId4 = gl4dgGenSpheref(20, 20);

    /* Create matrices for projection, model and view transformations */
    gl4duGenMatrix(GL_FLOAT, "projection");
    gl4duGenMatrix(GL_FLOAT, "model");
    gl4duGenMatrix(GL_FLOAT, "view");

    /* Enable depth testing for proper 3D rendering */
    glEnable(GL_DEPTH_TEST);

    /* Set up initial window size */
    resize(_wW, _wH);
    // gl4duBindMatrix("projection");
    // gl4duLoadIdentityf();
    // gl4duFrustumf(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
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
    float ballY = 0.0f;
    // static const float gravity = 9.8f;
    // static const float dampening = 0.8f;
    const float floorY = -2.0f;

    double t = gl4dGetElapsedTime() / 1000.0;
    double dt = t - t0;
    t0 = t;

    float lightBallX = 3.0f * sinf(t * 0.5f);
    float lightBallY = 1.0f + 0.5f * sinf(t);
    float lightBallZ = 3.0f * cosf(t);

    /* Set up light position for moving light */
    GLfloat lightPos[4] = {3.0f * sinf(t * 0.5f), 1 + 0.50f * sinf(t), 3.0f * cosf(t), 1.0f};
    GLfloat lightColor[4] = {1.0f, 1.0f, 1.0f, 0.30f}; /* Lumière plus intense (valeurs > 1 pour HDR) */

    GLfloat light2Pos[4] = {3.0f * cosf(t * 0.7f), 1.5f + 0.3f * sinf(t * 1.2f), 3.0f * sinf(t * 0.7f), 1.0f};
    // GLfloat light2couleur_obj[4] = {0.635f, 1.0f, 0.929f, 1.0f}; // Blanc légèrement teinté de turquoise
    // GLfloat light2direction[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    GLfloat light2couleur[4] = {0.29f, 0.988f, 0.859f, 1.0f}; // Blanc légèrement teinté de turquoise

    GLfloat light3Pos[4] = {2.5f * sinf(t * 0.8f + 2.0f), 0.8f + 0.6f * cosf(t * 0.9f), 2.5f * cosf(t * 0.8f + 1.5f), 1.0f};
    // GLfloat light3direction[4] = {0.0f, -1.0f, 0.0f, 1.0f};
    GLfloat light3couleur[4] = {0.373f, 0.373f, 1.0f, 1.0f}; // Blanc légèrement teinté de bleu
    // pour le son
    // printf("Volume: %f\n", teste);
    //  Amplify the volume to get a wider range of effects
    float amplifiedVolume = teste * 60.0f;

    // Clamp the amplified volume to a reasonable range (0-8)
    if (amplifiedVolume < 0.0f)
    {
        amplifiedVolume = 0.0f;
    }
    else if (amplifiedVolume > 8.0f)
    {
        amplifiedVolume = 8.0f;
    }

    // Convert to integer effect value
    // effets = (int)amplifiedVolume;
    // printf("effet: %d\n", effets);
    // glUniform1f(glGetUniformLocation(_postProcessProgramId, "audioLevel"), audioLevel);
    /* Clear the screen */
    // glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //  Au début de draw()
    glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // printf("FBO = %d\n", _fboId);
    //  Important : spécifier les deux buffers de sortie
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    /* Activate shader program */
    glUseProgram(_pId);
    /* Set up camera position */
    gl4duBindMatrix("view");
    gl4duLoadIdentityf();
    // Animation pour zoomer sur la sphère
    float cameraZoom = 7.0f;            // Distance initiale de la caméra
    const float MIN_ZOOM = 2.7f;        // Distance minimale réduite à la sphère (était 2.5f)
    const float TIME_THRESHOLD = 20.0f; // Secondes avant que la caméra commence à se déplacer
    const float ZOOM_SPEED = 0.0f;      // Vitesse augmentée du mouvement de la caméra

    // Si temps_début n'est pas encore défini, l'initialiser
    if (temps_début == 0.0)
    {
        temps_début = t;
    }

    // Calculer le temps écoulé depuis le début de la scène
    double temps_ecoule = t - temps_début;
    // printf("temps écoulé = %f\n", temps_ecoule);

    // Vérifier si nous devons zoomer
    if (temps_ecoule > TIME_THRESHOLD)
    {
        // Calculer à quel point nous devrions être proche
        cameraZoom -= ZOOM_SPEED * dt;
        // Ne pas trop s'approcher
        if (cameraZoom < MIN_ZOOM)
        {
            cameraZoom = MIN_ZOOM;
        }

        // Mettre à jour la position de la caméra
        gl4duBindMatrix("view");
        gl4duLoadIdentityf();
        //         gl4duLookAtf(0.0f, 1.0f, cameraZoom, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    }
    // else {
    //   gl4duLookAtf(0.0f, 1.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    //}
    gl4duLookAtf(0.0f, 1.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // Réinitialiser le paramètre "isEmissive" pour les autres objets
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
    glUniform4fv(glGetUniformLocation(_pId, "lightColor"), 1, lightColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);

    // Configurer la lumière directionnelle émise par un mur
    glUniform1i(glGetUniformLocation(_pId, "useSecondLight"), 1);

    // couleur de la lumière directionnelle
    glUniform1i(glGetUniformLocation(_pId, "useSecondLight"), 1);
    glUniform4fv(glGetUniformLocation(_pId, "secondLightPosition"), 1, light2Pos);
    glUniform4fv(glGetUniformLocation(_pId, "secondLightColor"), 1, light2couleur);
    // Ajouter ces lignes
    glUniform1i(glGetUniformLocation(_pId, "secondLightType"), 1); // 0 pour directionnelle

    // troisième lumière directionnelle
    glUniform1i(glGetUniformLocation(_pId, "useThirdLight"), 1);
    glUniform4fv(glGetUniformLocation(_pId, "thirdLightPosition"), 1, light3Pos);
    glUniform4fv(glGetUniformLocation(_pId, "thirdLightColor"), 1, light3couleur);
    glUniform1i(glGetUniformLocation(_pId, "thirdLightType"), 1); // 0 pour directionnelle

    // Draw a box with 6 quads (floor, ceiling, and 4 walls)
    gl4duBindMatrix("model");
    // Dark base color for all walls
    GLfloat couleur_mur[4] = {0.612, 0.812, 0.729, 1.0f};
    GLfloat emissiveColor[4] = {0.424f, 0.8f, 1.0f, 1.0f}; // couleur emissive bleu

    // Default to non-emissive
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, couleur_mur);
    glDisable(GL_CULL_FACE);

    // Variables to track wall emissive states
    float lastTriggerTime = 0;
    int lastWall = 0;
    int activeWall = 0;
    const float emissiveDuration = 0.5f; // le temps d'emissivité

    // check si le temps d'emissivité est écoulé
    bool isAnyWallActive = (t - lastTriggerTime) < emissiveDuration && activeWall > 0;

    // ça detecte si le son est fort
    if (!isAnyWallActive)
    {
        // determine quel mur est actif en fonction du volume
        int currentWall = 0;
        if (amplifiedVolume >= 1.5f && amplifiedVolume < 2.0f)
        {
            currentWall = 1; // mur droite
        }
        else if (amplifiedVolume >= 2.0f && amplifiedVolume < 3.0f)
        {
            currentWall = 2; // sol
        }
        else if (amplifiedVolume >= 3.0f)
        {
            currentWall = 3; // mur gauche
        }

        // si le mur actif est différent du dernier mur, on le fait emissif
        if (currentWall != 0 && currentWall != lastWall)
        {
            activeWall = currentWall;
            lastWall = currentWall;
            lastTriggerTime = t;
        }
    }
    // verifier si le temps d'emissivité est écoulé
    else if ((t - lastTriggerTime) >= emissiveDuration)
    {
        activeWall = 0;
    }

    // 1. sol
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, floorY - 0.5f, 0.0f);
    gl4duScalef(5.0f, 0.1f, 5.0f);
    gl4duSendMatrices();

    // si il est actif, on le fait emissif
    if (activeWall == 2)
    {
        glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
        glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, emissiveColor);
    }
    else
    {
        glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
        glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, couleur_mur);
        glUniform1f(glGetUniformLocation(_pId, "shininess"), 64.0f);
    }
    gl4dgDraw(_quadId);

    // 2. toit
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, floorY + 5.0f, 0.0f);
    gl4duScalef(5.0f, 0.1f, 5.0f);
    gl4duSendMatrices();
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, couleur_mur);
    glUniform1f(glGetUniformLocation(_pId, "shininess"), 64.0f);
    gl4dgDraw(_quadId);

    // 3. mur de gauche
    gl4duLoadIdentityf();
    gl4duTranslatef(-5.0f, floorY + 2.5f, 0.0f);
    gl4duScalef(0.1f, 5.0f, 5.0f);
    gl4duSendMatrices();
    // on le fait emissif si c'est le mur actif
    if (activeWall == 3)
    {
        glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
        // glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, emissiveColor);
    }
    else
    {
        glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
        glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, couleur_mur);
        glUniform1f(glGetUniformLocation(_pId, "shininess"), 64.0f);
    }
    gl4dgDraw(_quadId);

    // 4. mur de droite
    gl4duLoadIdentityf();
    gl4duTranslatef(5.0f, floorY + 2.5f, 0.0f);
    gl4duScalef(0.1f, 5.0f, 5.0f);
    gl4duSendMatrices();

    // on le fait emissif si c'est le mur actif
    if (activeWall == 1)
    {
        // glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1);
        // glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, emissiveColor);
    }
    else
    {
        glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
        glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, couleur_mur);
        glUniform1f(glGetUniformLocation(_pId, "shininess"), 64.0f);
    }
    gl4dgDraw(_quadId);

    // 5. mur arière
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, floorY + 2.5f, -5.0f);
    gl4duScalef(5.0f, 5.0f, 0.1f);
    gl4duSendMatrices();
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, couleur_mur);
    glUniform1f(glGetUniformLocation(_pId, "shininess"), 64.0f);
    gl4dgDraw(_quadId);

    // la balle d'eau
    gl4duBindMatrix("model");
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, ballY, -1.0f);
    gl4duScalef(_scale_boule, _scale_boule, _scale_boule);
    glEnable(GL_CULL_FACE);

    /* Send matrices to the shader */
    gl4duSendMatrices();

    // la balle de base
    GLfloat ambientColor[4] = {0.612, 0.812, 0.729, 0.40f}; /* Dark blue ambient */
    GLfloat ballColor[4] = {0.8f, 0.2f, 0.2f, 1.0f};        /* Red ball */
    GLfloat shininess = 64.0f;                              // Valeur de brillance (plus c'est élevé, plus le reflet est concentré)

    // envoie des truc au shader
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, ballColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightColor"), 1, lightColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
    glUniform4fv(glGetUniformLocation(_pId, "ambientColor"), 1, ambientColor);
    glUniform1f(glGetUniformLocation(_pId, "shininess"), shininess);
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniform1f(glGetUniformLocation(_pId, "waveStrength"), 1.5f); // Adjust wave intensity
    glUniform1f(glGetUniformLocation(_pId, "waveSpeed"), 5.0f);    // Adjust wave speed

    glUniform1f(glGetUniformLocation(_pId, "movementFactor"), 1.0f); // pour l'eau qui bouge
    // Clamp amplifiedVolume entre 0 and 2
    float amplificateur_mouvement = amplifiedVolume > 2.0f ? 2.0f : (amplifiedVolume < 0.0f ? 0.0f : amplifiedVolume);

    amplificateur_mouvement *= 2.0f;
    amplificateur_mouvement = amplifiedVolume > 3.0f ? 3.0f : (amplifiedVolume < 0.0f ? 0.0f : amplifiedVolume);
    // printf("amplificateur_mouvement: %f\n", amplificateur_mouvement);
    if (amplificateur_mouvement > 2.0f)
    {
        glUniform1f(glGetUniformLocation(_pId, "amplFactor"), amplificateur_mouvement * 0.10f); // pour l'eau qui bouge
    }
    else
    {
        glUniform1f(glGetUniformLocation(_pId, "touche"), 0.10f); // pour l'eau qui bouge
    }
    // movementFactor; // Facteur de vitesse pour le mouvement
    glUniform1i(glGetUniformLocation(_pId, "isWater"), 1); // Enable water effect
    glUniform1f(glGetUniformLocation(_pId, "time"), t);
    glUniform1i(glGetUniformLocation(_pId, "utiliseWater"), 1);
    // dessine la sphère
    gl4dgDraw(_sphereId);
    glUniform1i(glGetUniformLocation(_pId, "utiliseWater"), 0);
    glUniform1i(glGetUniformLocation(_pId, "isWater"), 0); // Disable water effect
    // enlever glEnable(GL_BLEND); et glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glDisable(GL_BLEND);

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
    GLfloat lightBallColor[4] = {1.0f, 0.9f, 0.6f, 1.0f};
    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, lightBallColor);
    glUniform4fv(glGetUniformLocation(_pId, "lightPosition"), 1, lightPos);
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1); // Cette balle est émissive
    glUniform1i(glGetUniformLocation(_pId, "lightType"), 1);
    glUniform1f(glGetUniformLocation(_pId, "time"), t);
    gl4dgDraw(_sphereId2);

    // la deuxieme sphere de lumière
    gl4duLoadIdentityf();
    // la trajetoire de la deuxieme boule
    float lightBall2X = 3.0f * cosf(t * 0.7f);
    float lightBall2Y = 1.5f + 0.3f * sinf(t * 1.2f);
    float lightBall2Z = 3.0f * sinf(t * 0.7f);

    gl4duTranslatef(lightBall2X, lightBall2Y, lightBall2Z);
    gl4duScalef(0.2f, 0.2f, 0.2f);
    gl4duSendMatrices();

    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, light2couleur);
    // Update its position for lighting
    GLfloat lightPos2[4] = {lightBall2X, lightBall2Y, lightBall2Z, 1.0f};
    glUniform4fv(glGetUniformLocation(_pId, "secondLightPosition"), 1, lightPos2);
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1);
    gl4dgDraw(_sphereId3);

    /* Draw third orbiting light sphere */
    gl4duLoadIdentityf();
    // Another unique trajectory for the third light ball
    float lightBall3X = 2.5f * sinf(t * 0.8f + 2.0f);
    float lightBall3Y = 0.8f + 0.6f * cosf(t * 0.9f);
    float lightBall3Z = 2.5f * cosf(t * 0.8f + 1.5f);

    gl4duTranslatef(lightBall3X, lightBall3Y, lightBall3Z);
    gl4duScalef(0.2f, 0.2f, 0.2f);
    gl4duSendMatrices();

    glUniform4fv(glGetUniformLocation(_pId, "ballColor"), 1, light3couleur);
    GLfloat lightPos3[4] = {lightBall3X, lightBall3Y, lightBall3Z, 1.0f};
    glUniform4fv(glGetUniformLocation(_pId, "thirdLightPosition"), 1, lightPos3);
    glUniform1i(glGetUniformLocation(_pId, "isEmissive"), 1);
    gl4dgDraw(_sphereId4);

    // 6. Back wall
    // gl4duLoadIdentityf();
    // gl4duTranslatef(0.0f, floorY + 2.5f, -5.0f);
    // gl4duScalef(5.0f, 5.0f, 0.1f);
    // gl4duSendMatrices();
    // gl4dgDraw(_quadId);

    teste += 0.05f;

    /* S'assurer que le framebuffer par défaut est actif */
    glBindFramebuffer(GL_FRAMEBUFFER, originalFBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_postProcessProgramId);
    // Configurer le post-processeur pour combiner la scène originale et le bloom
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texId); // Texture de la scène originale
    glUniform1i(glGetUniformLocation(_postProcessProgramId, "screenTexture"), 0);
    // pour le bloom
    //  Ajouter ces lignes:
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _bloomtexture);
    glUniform1i(glGetUniformLocation(_postProcessProgramId, "bloomTexture"), 1);

    int numLights = 3;
    glUniform1i(glGetUniformLocation(_postProcessProgramId, "numLights"), numLights);
    // glUniform1i(glGetUniformLocation(_postProcessProgramId, "screenTexture"), 0);

    /* Paramètres optionnels pour les effets */
    glUniform1f(glGetUniformLocation(_postProcessProgramId, "time"), t);
    glUniform2f(glGetUniformLocation(_postProcessProgramId, "resolution"), _wW, _wH);
    /* Activer la texture générée */
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
    /* Dessiner un quad plein écran */
    gl4dgDraw(_screenQuadId);
    // desactivation du shader
    glUseProgram(0);
}