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
// #include <time.h>
// #define USE_MINIFIED_SHADER //faut utiliser https://ctrl-alt-test.fr/minifier/?main pour réduire la taille des shaders
/* ---- Début du code de noise.c ---- */

static GLuint permTexId = 0, gradTexId = 0;
static GLuint permTexId2 = 0, gradTexId2 = 0;
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
static void initNoiseTextures(int taille, GLuint *permTexId_ptr, GLuint *gradTexId_ptr) {
    initRandomPermAndGrad();
    int i, j, k, i8;
    GLubyte *buffer, v;

    // Vérifiez que les ID ne sont pas déjà initialisés
    if (*permTexId_ptr || *gradTexId_ptr)
        return;

    buffer = malloc((1 << 18) /* 4 * 256 * 256 */ * sizeof *buffer);
    assert(buffer);

    for (i = 0; i < taille; i++) {
        i8 = i << 7;
        for (j = 0; j < taille; j++) {
            k = (i8 + j) << 2;
            v = perm[(j + perm[i]) & 0xFF];
            buffer[k + 0] = (grad4[v & 0x1F][0] << 6) + 64;
            buffer[k + 1] = (grad4[v & 0x1F][1] << 6) + 64;
            buffer[k + 2] = (grad4[v & 0x1F][2] << 6) + 64;
            buffer[k + 3] = (grad4[v & 0x1F][3] << 6) + 64;
        }
    }
    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, permTexId_ptr);  // Utilisez le pointeur
    glBindTexture(GL_TEXTURE_2D, *permTexId_ptr);  // Déréférencez
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, taille, taille, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    for (i = 0; i < taille*2; i++)
    {
        i8 = i << 8;
        for (j = 0; j < taille*2; j++)
        {
            k = (i8 + j) << 2;
            buffer[k + 3] = (v = perm[(j + perm[i]) & 0xFF]);
            buffer[k + 0] = (grad3[v & 0x0F][0] << 6) + 64;
            buffer[k + 1] = (grad3[v & 0x0F][1] << 6) + 64;
            buffer[k + 2] = (grad3[v & 0x0F][2] << 6) + 64;
        }
    }
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, gradTexId_ptr);  // Utilisez le pointeur
    glBindTexture(GL_TEXTURE_2D, *gradTexId_ptr);  // Déréférencez
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, taille*2, taille*2, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glActiveTexture(GL_TEXTURE0);

    free(buffer);
}

static void useNoiseTextures(GLuint pid, int shift, GLuint permTexId, GLuint gradTexId){
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

/* ---- Fin du code provenant de noise.c ---- */

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void draw(void);
/*!\brief largeur et hauteur de la fenêtre */
// static int _ww = 1280, _wh = 960;
/*!\brief identifiant du (futur) GLSL program */
static GLuint _pId = 0;
static GLuint _riviere_pId = 0;
/*!\brief identifiant pour une géométrie GL4D */
static GLuint _quadId = 0;
static GLuint _gridId = 0;
static GLuint _sphereId = 0;

/*!\brief identifiant de framebuffer object */
static GLuint _fboId = 0;
static GLuint _texId = 0;

float test = 0.0f;
/* booléen pour bruit ou pas de bruit */
// static int _noise = 0;

void montagne(int state)
{

    switch (state)
    {
    case GL4DH_INIT:
        /* INITIALISEZ VOTRE ANIMATION (SES VARIABLES <STATIC>s) */
        init();
        return;
    case GL4DH_FREE:
        /* LIBERER LA MEMOIRE UTILISEE PAR LES <STATIC>s */
        if (_pId)
        {
            glDeleteProgram(_pId);
            _pId = 0;
        }
        if (_riviere_pId)
        {
            glDeleteProgram(_riviere_pId);
            _riviere_pId = 0;
        }
        if (_gridId)
        {
            //gl4dgDelete(_gridId);
            _gridId = 0;
        }
        if (_sphereId)
        {
            //gl4dgDelete(_sphereId);
            _sphereId = 0;
        }
        if (_quadId)
        {
            //gl4dgDelete(_quadId);
            _quadId = 0;
        }
        if (permTexId)
        {
            glDeleteTextures(1, &permTexId);
            permTexId = 0;
        }
        if (gradTexId)
        {
            glDeleteTextures(1, &gradTexId);
            gradTexId = 0;
        }
        if (permTexId2)
        {
            glDeleteTextures(1, &permTexId2);
            permTexId2 = 0;
        }
        if (gradTexId2)
        {
            glDeleteTextures(1, &gradTexId2);
            gradTexId2 = 0;
        }
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
void init(void){
    //Nouvelle manière include direct avec imfs (merci julien)
    const char * imfs = "<imfs>riviere.fs</imfs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
        "precision mediump float;\n"
    #else
        "#version 330 core\n"
    #endif
"uniform float time,waveStrength,waveSpeed;\n"
"uniform int isWater;\n"
"uniform vec3 lightPosition,lightColor;\n"
"uniform float shininess,noiseScale;\n"
"uniform sampler2D permTexture,gradTexture;\n"
"in vec3 normal,fragPos;\n"
"in vec2 texCoord;\n"
"out vec4 FragColor;\n"
"vec3 applyWaves(vec3 position,vec3 normal)\n"
"{\n"
"  float amplitude=waveStrength*.05,wave=sin(position.z*10.-time*waveSpeed),turbulence=sin(position.x*5.+time*waveSpeed);\n"
"  position.y+=(wave*amplitude*1.5+turbulence*amplitude*.3)*(1.+(texture(permTexture,texCoord*noiseScale+vec2(time*.05,-time*.03)).x*2.-1.)*.3);\n"
"  return position;\n"
"}\n"
"void main()\n"
"{\n"
"  vec3 norm=normalize(normal),wavyPos=applyWaves(fragPos,norm),lightDir=normalize(lightPosition-wavyPos);\n"
"  vec2 noiseUV=texCoord*noiseScale+vec2(cos(time*.2)*.005,-time*.005);\n"
"  vec4 noise=texture(permTexture,noiseUV);\n"
"  norm=normalize(mix(mix(normal,normalize(cross(applyWaves(fragPos+vec3(.005,0,0),vec3(0,1,0))-applyWaves(fragPos-vec3(.005,0,0),vec3(0,1,0)),applyWaves(fragPos+vec3(0,0,.005),vec3(0,1,0))-applyWaves(fragPos-vec3(0,0,.005),vec3(0,1,0)))),.5),normalize(vec3((-texture(permTexture,noiseUV+vec2(.01,0)).x+noise.x)*waveStrength*10.,1,(-texture(permTexture,noiseUV+vec2(0,.01)).x+noise.x)*waveStrength*10.)),.3));\n"
"  float waterRipple=noise.x*.2,backLight=max(0.,dot(-normalize(-wavyPos),norm))*.5,diff=max(dot(norm,lightDir),0.);\n"
"  wavyPos=.5*diff*lightColor;\n"
"  diff=pow(max(dot(norm,lightDir),0.),shininess*(2.+sin(time)));\n"
"  FragColor=vec4(mix(vec3(0,.4,.8),vec3(0,.6,1),waterRipple)*(.2*lightColor+backLight*vec3(.2,.3,.5)+(vec3(.05,.05,.1)*sin(time*.5)*.5+.5)+wavyPos)+.3*(1.+waterRipple*2.)*diff*lightColor+wavyPos*(.3+sin(time*1.3)*.1),1);\n"
"}";

    const char * imvs = "<imvs>riviere.vs</imvs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
    #else
        "#version 330 core\n"
    #endif
    "layout(location=0)in vec3 vPosition;\n"
    "layout(location=1)in vec3 vNormal;\n"
    "layout(location=2)in vec2 vTexCoord;\n"
    "uniform mat4 projectionMatrix,modelMatrix,viewMatrix;\n"
    "uniform float waveStrength,waveSpeed,time,movementFactor,amplFactor;\n"
    "out vec3 normal,fragPos;\n"
    "out vec2 texCoord;\n"
    "void main()\n"
    "{\n"
    "  vec3 pos=vPosition;\n"
    "  gl_Position=projectionMatrix*viewMatrix*modelMatrix*vec4(pos,1);\n"
    "  normal=vNormal;\n"
    "  fragPos=vec3(modelMatrix*vec4(pos,1));\n"
    "  texCoord=vTexCoord;\n"
    "}\n";
    _riviere_pId = gl4duCreateProgram(imvs, imfs, NULL);
    imfs = "<imfs>lum_montagne.fs</imfs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
        "precision mediump float;\n"
    #else
        "#version 330 core\n"
    #endif
    "uniform vec4 sdiffus,sambient,sspeculaire,l0diffus,l0speculaire,l1diffus,l1speculaire,lambient;\n"
    "uniform mat4 projectionMatrix,viewMatrix,modelMatrix;\n"
    "uniform vec4 Lp0,Lp1;\n"
    "uniform float temps;\n"
    "uniform int has_noise,sky;\n"
    "in vec4 vmPos;\n"
    "in vec3 vmNormal;\n"
    "in vec2 tc;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D permTexture,gradTexture;\n"
    "#define ONE 0.00390625\n"
    "#define ONEHALF 0.001953125\n"
    "float fade(float t)\n"
    "{\n"
    "  return t*t*t*(t*(t*6.-15.)+10.);\n"
    "}\n"
    "float noise(vec2 P)\n"
    "{\n"
    "  vec2 Pi=ONE*floor(P)+ONEHALF;\n"
    "  P=fract(P);\n"
    "  vec2 grad10=texture(permTexture,Pi+vec2(ONE,0)).xy*4.-1.,grad01=texture(permTexture,Pi+vec2(0,ONE)).xy*4.-1.,grad11=texture(permTexture,Pi+vec2(ONE)).xy*4.-1.;\n"
    "  Pi=mix(vec2(dot(texture(permTexture,Pi).xy*4.-1.,P),dot(grad01,P-vec2(0,1))),vec2(dot(grad10,P-vec2(1,0)),dot(grad11,P-vec2(1))),fade(P.x));\n"
    "  return mix(Pi.x,Pi.y,fade(P.y));\n"
    "}\n"
    "float noise(vec3 P)\n"
    "{\n"
    "  vec3 Pi=ONE*floor(P)+ONEHALF;\n"
    "  P=fract(P);\n"
    "  float perm00=texture(permTexture,Pi.xy).w;\n"
    "  vec3 grad001=texture(permTexture,vec2(perm00,Pi.z+ONE)).xyz*4.-1.;\n"
    "  float perm01=texture(permTexture,Pi.xy+vec2(0,ONE)).w;\n"
    "  vec3 grad011=texture(permTexture,vec2(perm01,Pi.z+ONE)).xyz*4.-1.;\n"
    "  float perm10=texture(permTexture,Pi.xy+vec2(ONE,0)).w;\n"
    "  vec3 grad101=texture(permTexture,vec2(perm10,Pi.z+ONE)).xyz*4.-1.;\n"
    "  float perm11=texture(permTexture,Pi.xy+vec2(ONE)).w;\n"
    "  vec3 grad111=texture(permTexture,vec2(perm11,Pi.z+ONE)).xyz*4.-1.;\n"
    "  vec4 n_x=mix(vec4(dot(texture(permTexture,vec2(perm00,Pi.z)).xyz*4.-1.,P),dot(grad001,P-vec3(0,0,1)),dot(texture(permTexture,vec2(perm01,Pi.z)).xyz*4.-1.,P-vec3(0,1,0)),dot(grad011,P-vec3(0,1,1))),vec4(dot(texture(permTexture,vec2(perm10,Pi.z)).xyz*4.-1.,P-vec3(1,0,0)),dot(grad101,P-vec3(1,0,1)),dot(texture(permTexture,vec2(perm11,Pi.z)).xyz*4.-1.,P-vec3(1,1,0)),dot(grad111,P-vec3(1))),fade(P.x));\n"
    "  vec2 n_xy=mix(n_x.xy,n_x.zw,fade(P.y));\n"
    "  return mix(n_xy.x,n_xy.y,fade(P.z));\n"
    "}\n"
    "vec2 rug()\n"
    "{\n"
    "  vec2 no=vec2(0);\n"
    "  for(float freq=1.,amp=1.;freq<33.;freq*=2.,amp/=2.)\n"
    "    no+=vec2(amp*noise(freq*30.*tc.xy),amp*noise(freq*30.*tc.yx));\n"
    "  return no;\n"
    "}\n"
    "float rug2()\n"
    "{\n"
    "  float no=0.;\n"
    "  for(float freq=1.,amp=1.;freq<8.;freq*=2.,amp/=2.)\n"
    "    no+=amp*noise(freq*.3*vmPos.xyz+vec3(0,0,temps));\n"
    "  float cloudCoverage=clamp(temps*.05,.1,.9);\n"
    "  return smoothstep(cloudCoverage,cloudCoverage+.2,no);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "  if(sky!=0)\n"
    "    {\n"
    "      float nu=rug2();\n"
    "      fragColor=mix(vec4(.5,.7,1,1),vec4(1),smoothstep(0.,1.,nu));\n"
    "      return;\n"
    "    }\n"
    "  const vec3 vue=vec3(0,0,-1);\n"
    "  vec3 Ld0=normalize((vmPos-viewMatrix*Lp0).xyz),Ld1=normalize((vmPos-viewMatrix*Lp1).xyz),n=normalize(vmNormal),T=vec3(1,0,0),B=cross(T,n);\n"
    "  vec2 no=rug();\n"
    "  n=normalize(4.*n+no.x*T+no.y*B);\n"
    "  float intensite_diffus0=clamp(dot(n,-Ld0),0.,1.),intensite_diffus1=clamp(dot(n,-Ld1),0.,1.);\n"
    "  B=reflect(Ld0,n);\n"
    "  Ld0=reflect(Ld1,n);\n"
    "  fragColor=mix(sambient*lambient,intensite_diffus0*sdiffus*l0diffus+intensite_diffus1*sdiffus*l1diffus,.75)+pow(clamp(dot(B,-vue),0.,1.),4e2)*l0speculaire*sspeculaire+pow(clamp(dot(Ld0,-vue),0.,1.),4e2)*l1speculaire*sspeculaire;\n"
    "}\n";

    imvs = "<imvs>lum_montagne.vs</imvs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
    #else
        "#version 330 core\n"
    #endif
    "layout (location = 0) in vec3 vsiPosition;\n"
    "layout (location = 1) in vec3 vsiNormal;\n"
    "layout (location = 2) in vec2 vsiTexCoord;\n"
    "uniform mat4 projectionMatrix, viewMatrix, modelMatrix;\n"
    "out vec4 vmPos;\n"
    "out vec3 vmNormal;\n"
    "out vec2 tc;\n"
    "void main(void) {\n"
    "  vmPos = viewMatrix * modelMatrix * vec4(vsiPosition, 1.0);\n"
    "  vmNormal = (transpose(inverse(viewMatrix * modelMatrix)) * vec4(vsiNormal, 0.0)).xyz;\n"
    "  gl_Position = projectionMatrix * vmPos;\n"
    "  tc = vsiTexCoord;\n"
    "}\n";
    _pId = gl4duCreateProgram(imvs, imfs, NULL);
    //_pId = gl4duCreateProgram("<vs>shaders/lum_montagne.vs", "<fs>shaders/lum_montagne.fs", NULL);
    //_riviere_pId = gl4duCreateProgram("<vs>shaders/riviere.vs", "<fs>shaders/riviere.fs", NULL);
    /* générer le terrain */
    GLfloat *heightmap = gl4dmTriangleEdge(33, 33, 0.7f);
    /* Créer une grid */
    _gridId = gl4dgGenGrid2dFromHeightMapf(33, 33, heightmap);
    free(heightmap);
    _sphereId = gl4dgGenSpheref(33, 33);
    _quadId = gl4dgGenCubef();
    /* Set de la couleur (RGBA) d'effacement OpenGL */
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    /* activation du test de profondeur afin de prendre en compte la
     * notion de devant-derrière. */
    glEnable(GL_DEPTH_TEST);
    /* Création des matrices GL4Dummies, une pour la projection, une
     * pour la modélisation et une pour la vue */
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    gl4duGenMatrix(GL_FLOAT, "modelMatrix");
    gl4duGenMatrix(GL_FLOAT, "viewMatrix");
    
    gl4duBindMatrix("projectionMatrix");
    gl4duLoadIdentityf();
    gl4duPerspectivef(60.0f, 1.0f, 0.1f, 1000.0f);

    initNoiseTextures(128, &permTexId, &gradTexId);
    initNoiseTextures(128, &permTexId2, &gradTexId2);


    glGenFramebuffers(1, &_fboId);
    glGenTextures(1, &_texId);
    glBindTexture(GL_TEXTURE_2D, _texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 960, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/*!\brief Cette fonction dessine dans le contexte OpenGL actif. */
void draw(void)
{
    static GLfloat angle = 0.0f;
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
    t0 = t;
    /* effacement du buffer de couleur et de profondeur */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* activation du programme _pId */
    glUseProgram(_pId);
    /* lier la matrice vue */
    gl4duBindMatrix("viewMatrix");
    /* Charger la matrice identité */
    gl4duLoadIdentityf();
    //gl4duLookAtf(0.0f, 5.0f, 5.0f, 0.0f, 0.0f, -1.3f, 0.0f, 1.0f, 0.0f);
    //gl4duLookAtf(0.0f, 2.0f, 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    // eyeY += 0.0005f;
    test -= 0.1f*dt;
    /* lier la matrice modèle */
    gl4duBindMatrix("modelMatrix");
    /* Charger la matrice identité */
    gl4duLoadIdentityf();
    /* Mise à l'échelle du terrain */
    gl4duScalef(2.50f, 1.0f, 2.50f);
    gl4duRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    gl4duTranslatef(0.0f, -0.50f, -0.50f);
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
    glUniform4f(glGetUniformLocation(_pId, "Lp1"), 0.0f, sin(-1 + t * 2.3f) * 8.0f, -3.0f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "l1diffus"), 0.90f, 0.90f, 0.90f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "l1speculaire"), 0.6f, 0.6f, 0.60f, 1.0f); // couleur du reflet

    /* lumière ambiante */
    glUniform4f(glGetUniformLocation(_pId, "lambient"), 1.0f, 1.0f, 1.0f, 1.0f);

    /* Matériaux */
    glUniform4f(glGetUniformLocation(_pId, "sambient"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform4f(glGetUniformLocation(_pId, "sdiffus"), 0.0f, 0.341f, 0.122f, 1.0f); // couleur du terrain
    glUniform4f(glGetUniformLocation(_pId, "sspeculaire"), 0.1f, 0.1f, 0.1f, 0.20f);

    /* Activer les textures de bruit pour le terrain et le ciel */
    useNoiseTextures(_pId, 0, permTexId, gradTexId);

    glEnable(GL_DEPTH_TEST); // faut activer ça
    glEnable(GL_CULL_FACE);
    gl4dgDraw(_gridId);

    /* Configurer la sphère pour le ciel */
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duScalef(25.0f, 25.0f, 25.0f);
    
    gl4duSendMatrices();

    /* Désactiver le culling pour voir l'intérieur de la sphère */
    glDisable(GL_CULL_FACE);

    /* Dessiner le ciel avec les nuages */
    glUniform1i(glGetUniformLocation(_pId, "sky"), 1);
    gl4dgDraw(_sphereId);
    glUniform1i(glGetUniformLocation(_pId, "sky"), 0);

    /* Réactiver le culling */
    glEnable(GL_CULL_FACE);
    unuseNoiseTextures(0);

    glUseProgram(_riviere_pId);
    useNoiseTextures(_riviere_pId, 0, permTexId2, gradTexId2);

    //on va dessiner le quad comme une rivière inclinée sur le terrain
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duScalef(6.0f, 0.20f, 6.0f);
    gl4duTranslatef(0.0f, -1.80f, -1.30f);
    //gl4duRotatef(180.0f, 0.0f, 0.0f, 1.0f);
    gl4duRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    gl4duBindMatrix("viewMatrix");
    gl4duBindMatrix("projectionMatrix");
    gl4duSendMatrices();
    glUniform1f(glGetUniformLocation(_riviere_pId, "time"), t*10.0f);
    /* Configuration des lumières */
    glUniform1f(glGetUniformLocation(_riviere_pId, "noiseScale"), 0.8f);  // Ajoutez ce paramètre
    glUniform3f(glGetUniformLocation(_riviere_pId, "lightPosition"), 0.0f, sin(-1 + t * 2.3f) * 8.0f, -3.0f);
    glUniform3f(glGetUniformLocation(_riviere_pId, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(_riviere_pId, "shininess"), 16.0f);
    glUniform1f(glGetUniformLocation(_riviere_pId, "waveStrength"), 0.5f);
    glUniform1f(glGetUniformLocation(_riviere_pId, "waveSpeed"), 6.0f);
    glUniform1f(glGetUniformLocation(_riviere_pId, "movementFactor"), 1.50f);
    glUniform1f(glGetUniformLocation(_riviere_pId, "amplFactor"), 0.9f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl4dgDraw(_quadId);
    glDisable(GL_BLEND);

    /* Désactiver le programme shader */
    glUseProgram(0);
    /* Désactiver les textures de bruit */
    unuseNoiseTextures(0);

    /* Mise à jour de l'angle pour animation */
    angle += 18.0f * dt;
}