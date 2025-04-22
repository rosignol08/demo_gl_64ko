/*!\file window.c
 * \brief GL4Dummies, exemple progressif d'éclairage avec bruit de Perlin intégré
 * \author Farès BELHADJ, amsi@up8.edu
 * \date February 02 2025
 */

 #include <GL4D/gl4du.h>
 #include <GL4D/gl4df.h>
 #include <GL4D/gl4duw_SDL2.h>
 #include <assert.h>
 #include <time.h>
 #define USE_MINIFIED_SHADER
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
     srand(time(NULL));
 
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
 
 static void freeNoiseTextures(void)
 {
     glDeleteTextures(1, &gradTexId);
     glDeleteTextures(1, &permTexId);
     permTexId = 0;
     gradTexId = 0;
 }
 
 /* ---- Fin du code provenant de noise.c ---- */
 
 /* Prototypes des fonctions statiques contenues dans ce fichier C */
 static void init(void);
 static void draw(void);
 // static void keyd(int keycode);
 static void resize(int w, int h);
 static void quit(void);
 
 /*!\brief largeur et hauteur de la fenêtre */
 static int _ww = 1280, _wh = 960;
 /*!\brief identifiant du (futur) GLSL program */
 static GLuint _pId = 0;
 /*!\brief identifiant pour une géométrie GL4D */
 static GLuint _quadId = 0;
 static GLuint _gridId = 0;
 static GLuint _sphereId = 0;
 
 /*!\brief identifiant de framebuffer object */
 static GLuint _fboId = 0;
 static GLuint _texId = 0;
 
 /* booléen pour bruit ou pas de bruit */
 static int _noise = 0;
 
 /*!\brief créé la fenêtre d'affichage, initialise GL et les données,
  * affecte les fonctions d'événements et lance la boucle principale
  * d'affichage.
  */
 int main(int argc, char **argv)
 {
     if (!gl4duwCreateWindow(argc, argv, "GL4Dummies", 20, 20, _ww, _wh, GL4DW_RESIZABLE | GL4DW_SHOWN))
         return 1;
     init();
     atexit(quit);
     gl4duwResizeFunc(resize);
     // gl4duwKeyDownFunc(keyd);
     gl4duwDisplayFunc(draw);
     gl4duwMainLoop();
     return 0;
 }
 
 /*!\brief initialise les paramètres OpenGL et les données. */
 void init(void)
 {
     const char *noiseFragmentShader = NULL;
     const char *noiseVertexShader = NULL;
     /*
     if (!_pId)
     {
         #ifdef USE_MINIFIED_SHADER
         //#ifndef SHADER_MINIFIER_IMPL
         //#ifndef SHADER_MINIFIER_HEADER
         # define SHADER_MINIFIER_HEADER
         # define VAR_Lp0 "f"
         # define VAR_Lp1 "n"
         # define VAR_fragColor "r"
         # define VAR_gradTexture "C"
         # define VAR_has_noise "i"
         # define VAR_l0diffus "z"
         # define VAR_l0speculaire "E"
         # define VAR_l1diffus "O"
         # define VAR_l1speculaire "x"
         # define VAR_lambient "w"
         # define VAR_modelMatrix "F"
         # define VAR_permTexture "G"
         # define VAR_projectionMatrix "m"
         # define VAR_sambient "d"
         # define VAR_sdiffus "y"
         # define VAR_sky "c"
         # define VAR_sspeculaire "v"
         # define VAR_tc "H"
         # define VAR_temps "t"
         # define VAR_viewMatrix "s"
         # define VAR_vmNormal "D"
         # define VAR_vmPos "A"
         //#else // if SHADER_MINIFIER_IMPL
         noiseFragmentShader =
         // input
         "#version 330\n"
         #endif
         
         
         
         #ifdef USE_MINIFIED_SHADER
         # define VAR_modelMatrix "zz"
         # define VAR_projectionMatrix "Pp"
         # define VAR_tc "gg"
         # define VAR_viewMatrix "ll"
         # define VAR_vmNormal "oo"
         # define VAR_vmPos "nn"
         # define VAR_vsiNormal "i"
         # define VAR_vsiPosition "v"
         # define VAR_vsiTexCoord "m"
         //#else // if SHADER_MINIFIER_IMPL
         // input
         noiseVertexShader =
          "#version 330\n"
          "layout(location=0) in vec3 v;"
          "layout(location=1) in vec3 i;"
          "layout(location=2) in vec2 m;"
          "uniform mat4 Pp,ll,zz;"
          "out vec4 nn;"
          "out vec3 oo;"
          "out vec2 gg;"
          "void main()"
          "{"
            "nn=ll*zz*vec4(v,1);"
            "oo=(transpose(inverse(ll*zz))*vec4(i,0)).xyz;"
            "gl_Position=Pp*nn;"
            "gg=m;"
          "}",
         #endif
 */
     _pId = gl4duCreateProgram("<vs>shaders/lum_montagne.vs", "<fs>shaders/lum_montagne.fs", NULL);
         //_pId = gl4duCreateProgram("<vs>shaders/lum_montagne.vs", noiseFragmentShader, NULL);
         gl4duAtExit(quit);
     
     //_pId = gl4duCreateProgram(gl4dfBasicVS, noiseFragmentShader, NULL);
     // gl4duAtExit(quit);
 
     /* Création du programme shader (voir le dossier shader) */
     //_pId = gl4duCreateProgram("<vs>shaders/lum_montagne.vs", "<fs>shaders/lum_montagne.fs", NULL);
     //_pId = gl4duCreateProgram(vertexShader, noiseVertexShader, noiseFragmentShader);
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
     gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
     gl4duGenMatrix(GL_FLOAT, "modelMatrix");
     gl4duGenMatrix(GL_FLOAT, "viewMatrix");
     resize(_ww, _wh);
     initNoiseTextures();
 
     glGenFramebuffers(1, &_fboId);
     glGenTextures(1, &_texId);
     glBindTexture(GL_TEXTURE_2D, _texId);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _ww, _wh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
     glBindTexture(GL_TEXTURE_2D, 0);
 }
 /* Variables pour la position de la caméra, le point regardé et le vecteur up */
 static GLfloat eyeX = 0.20f, eyeY = 0.20f, eyeZ = 0.20f;
 static GLfloat centerX = -20.10f, centerY = 4.40f, centerZ = -1.80f;
 static GLfloat upX = 0.0f, upY = 0.40f, upZ = 0.0f;
 // Eye: (0.20, 0.30, 0.20)
 // Center: (-20.10, 4.40, -1.80)
 // Up: (0.00, 0.40, 0.00)
 
 ///* Fonction pour gérer les mouvements de caméra */
 // static void keyd(int keycode)
 //{
 //     GLfloat step = 0.1f;
 //
 //     switch(keycode) {
 //         /* Position de la caméra (eye) */
 //         case GL4DK_q:
 //             eyeX -= step;
 //             break;
 //         case GL4DK_w:
 //             eyeX += step;
 //             break;
 //         case GL4DK_a:
 //             eyeY -= step;
 //             break;
 //         case GL4DK_s:
 //             eyeY += step;
 //             break;
 //         case GL4DK_z:
 //             eyeZ -= step;
 //             break;
 //         case GL4DK_x:
 //             eyeZ += step;
 //             break;
 //
 //         /* Point regardé (center) */
 //         case GL4DK_e:
 //             centerX -= step;
 //             break;
 //         case GL4DK_r:
 //             centerX += step;
 //             break;
 //         case GL4DK_d:
 //             centerY -= step;
 //             break;
 //         case GL4DK_f:
 //             centerY += step;
 //             break;
 //         case GL4DK_c:
 //             centerZ -= step;
 //             break;
 //         case GL4DK_v:
 //             centerZ += step;
 //             break;
 //
 //         /* Vecteur up */
 //         case GL4DK_t:
 //             upX -= step;
 //             break;
 //         case GL4DK_y:
 //             upX += step;
 //             break;
 //         case GL4DK_g:
 //             upY -= step;
 //             break;
 //         case GL4DK_h:
 //             upY += step;
 //             break;
 //         case GL4DK_b:
 //             upZ -= step;
 //             break;
 //         case GL4DK_n:
 //             upZ += step;
 //             break;
 //         default:
 //             break;
 //     }
 //
 //     printf("Camera settings:\n");
 //     printf("Eye: (%.2f, %.2f, %.2f)\n", eyeX, eyeY, eyeZ);
 //     printf("Center: (%.2f, %.2f, %.2f)\n", centerX, centerY, centerZ);
 //     printf("Up: (%.2f, %.2f, %.2f)\n\n", upX, upY, upZ);
 // }
 /*!\brief Cette fonction dessine dans le contexte OpenGL actif. */
 void draw(void)
 {
     static GLfloat angle = 0.0f;
     static double t0 = 0.0;
     double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
     t0 = t;
     glBindFramebuffer(GL_FRAMEBUFFER, _fboId);
     glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _texId, 0);
     gl4dgDraw(_quadId);
     glBindFramebuffer(GL_FRAMEBUFFER, 0);
     ////////////////////
 
     /* effacement du buffer de couleur et de profondeur */
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     /* activation du programme _pId */
     glUseProgram(_pId);
     /* lier la matrice vue */
     gl4duBindMatrix("viewMatrix");
     /* Charger la matrice identité */
     gl4duLoadIdentityf();
     /* Composer la matrice vue avec la caméra */
     // gl4duLookAtf(0.0f, 2.0f, 2.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
     gl4duLookAtf(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
     eyeY += 0.0005f;
     /* lier la matrice modèle */
     gl4duBindMatrix("modelMatrix");
     /* Charger la matrice identité */
     gl4duLoadIdentityf();
     /* Mise à l'échelle du terrain */
     gl4duScalef(1.0f, 0.5f, 1.0f);
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
     gl4dgDraw(_gridId);
 
     /* Configurer la sphère pour le ciel */
     gl4duBindMatrix("modelMatrix");
     gl4duLoadIdentityf();
     gl4duScalef(5.0f, 5.0f, 5.0f);
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
 void quit(void)
 {
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
 
     /* Libérer les textures de bruit */
     freeNoiseTextures();
 
     /* Nettoyer GL4Dummies */
     gl4duClean(GL4DU_ALL);
 }
 /*backup des shaders*/
 ///*!\file light.fs
 // * \brief fragment shader progressif sur l'éclairage. */
 //#version 330
 ///* caractéristiques diffus et ambient de la surface et des lumières */
 //uniform vec4 sdiffus, sambient, sspeculaire, l0diffus, l0speculaire, l1diffus, l1speculaire, lambient;
 //uniform mat4 projectionMatrix, viewMatrix, modelMatrix;
 //uniform vec4 Lp0, Lp1;
 //uniform float temps;
 //uniform int has_noise, sky;
 //in vec4 vmPos;
 //in vec3 vmNormal;
 //in vec2 tc;
 ///* sortie du frament shader : une couleur */
 //out vec4 fragColor;
 //
 ///*
 // * 2D, 3D and 4D Perlin noise (classic) in a GLSL fragment shader.
 // *
 // * Classic noise is implemented by the functions:
 // * float noise(vec2 P)
 // * float noise(vec3 P)
 // *
 // * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 // *
 // * You may use, modify and redistribute this code free of charge,
 // * provided that the author's names and this notice appear intact.
 // */
 //
 //uniform sampler2D permTexture;
 //uniform sampler2D gradTexture;
 //
 //#define ONE 0.00390625
 //#define ONEHALF 0.001953125
 //
 ///*
 // * The interpolation function. 
 // */
 //float fade(float t) {
 //  return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
 //}
 //
 ///*
 // * 2D classic Perlin noise. Fast, but less useful than 3D noise.
 // */
 //float noise(vec2 P)
 //{
 //  vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
 //  vec2 Pf = fract(P);             // Fractional part for interpolation
 //
 //  // Noise contribution from lower left corner
 //  vec2 grad00 = texture(permTexture, Pi).rg * 4.0 - 1.0;
 //  float n00 = dot(grad00, Pf);
 //
 //  // Noise contribution from lower right corner
 //  vec2 grad10 = texture(permTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
 //  float n10 = dot(grad10, Pf - vec2(1.0, 0.0));
 //
 //  // Noise contribution from upper left corner
 //  vec2 grad01 = texture(permTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
 //  float n01 = dot(grad01, Pf - vec2(0.0, 1.0));
 //
 //  // Noise contribution from upper right corner
 //  vec2 grad11 = texture(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
 //  float n11 = dot(grad11, Pf - vec2(1.0, 1.0));
 //
 //  // Blend contributions along x
 //  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade(Pf.x));
 //
 //  // Blend contributions along y
 //  float n_xy = mix(n_x.x, n_x.y, fade(Pf.y));
 //
 //  // We're done, return the final noise value.
 //  return n_xy;
 //}
 //
 ///*
 // * 3D classic noise. Slower, but a lot more useful than 2D noise.
 // */
 //float noise(vec3 P)
 //{
 //  vec3 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled so +1 moves one texel
 //                                  // and offset 1/2 texel to sample texel centers
 //  vec3 Pf = fract(P);     // Fractional part for interpolation
 //
 //  // Noise contributions from (x=0, y=0), z=0 and z=1
 //  float perm00 = texture(permTexture, Pi.xy).a ;
 //  vec3  grad000 = texture(permTexture, vec2(perm00, Pi.z)).rgb * 4.0 - 1.0;
 //  float n000 = dot(grad000, Pf);
 //  vec3  grad001 = texture(permTexture, vec2(perm00, Pi.z + ONE)).rgb * 4.0 - 1.0;
 //  float n001 = dot(grad001, Pf - vec3(0.0, 0.0, 1.0));
 //
 //  // Noise contributions from (x=0, y=1), z=0 and z=1
 //  float perm01 = texture(permTexture, Pi.xy + vec2(0.0, ONE)).a ;
 //  vec3  grad010 = texture(permTexture, vec2(perm01, Pi.z)).rgb * 4.0 - 1.0;
 //  float n010 = dot(grad010, Pf - vec3(0.0, 1.0, 0.0));
 //  vec3  grad011 = texture(permTexture, vec2(perm01, Pi.z + ONE)).rgb * 4.0 - 1.0;
 //  float n011 = dot(grad011, Pf - vec3(0.0, 1.0, 1.0));
 //
 //  // Noise contributions from (x=1, y=0), z=0 and z=1
 //  float perm10 = texture(permTexture, Pi.xy + vec2(ONE, 0.0)).a ;
 //  vec3  grad100 = texture(permTexture, vec2(perm10, Pi.z)).rgb * 4.0 - 1.0;
 //  float n100 = dot(grad100, Pf - vec3(1.0, 0.0, 0.0));
 //  vec3  grad101 = texture(permTexture, vec2(perm10, Pi.z + ONE)).rgb * 4.0 - 1.0;
 //  float n101 = dot(grad101, Pf - vec3(1.0, 0.0, 1.0));
 //
 //  // Noise contributions from (x=1, y=1), z=0 and z=1
 //  float perm11 = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a ;
 //  vec3  grad110 = texture(permTexture, vec2(perm11, Pi.z)).rgb * 4.0 - 1.0;
 //  float n110 = dot(grad110, Pf - vec3(1.0, 1.0, 0.0));
 //  vec3  grad111 = texture(permTexture, vec2(perm11, Pi.z + ONE)).rgb * 4.0 - 1.0;
 //  float n111 = dot(grad111, Pf - vec3(1.0, 1.0, 1.0));
 //
 //  // Blend contributions along x
 //  vec4 n_x = mix(vec4(n000, n001, n010, n011),
 //                 vec4(n100, n101, n110, n111), fade(Pf.x));
 //
 //  // Blend contributions along y
 //  vec2 n_xy = mix(n_x.xy, n_x.zw, fade(Pf.y));
 //
 //  // Blend contributions along z
 //  float n_xyz = mix(n_xy.x, n_xy.y, fade(Pf.z));
 //
 //  // We're done, return the final noise value.
 //  return n_xyz;
 //}
 //
 //vec2 rug(void) {
 //  const float zoom = 30.0;
 //  vec2 no = vec2(0.0);
 //  for(float freq = 1.0, amp = 1.0; freq < 33.0; freq *= 2.0, amp /= 2.0)
 //    no += vec2(amp * noise(freq * zoom * tc.xy), amp * noise(freq * zoom * tc.yx));
 //  return no;
 //}
 //
 //float rug22(void) {
 //  const float zoom = 0.30; // Nuages plus larges avec un zoom plus petit
 //  float no = 0.0;
 //  // Utilisez moins d'octaves pour un rendu plus rapide et fluide
 //  for(float freq = 1.0, amp = 1.0; freq < 8.0; freq *= 2.0, amp /= 2.0)
 //    no += amp * noise(freq * zoom * vmPos.xyz + vec3(0.0, 0.0, temps)); // Ajout du temps pour animation
 //  
 //  // Ajustement de la distribution pour avoir des zones vides et des zones denses
 //  no = smoothstep(0.1, 0.9, no);
 //  return no;
 //}
 //float rug2(void) {
 //  const float zoom = 0.30; // Nuages plus larges avec un zoom plus petit
 //  float no = 0.0;
 //  // Animation basée sur le temps
 //  for(float freq = 1.0, amp = 1.0; freq < 8.0; freq *= 2.0, amp /= 2.0)
 //    no += amp * noise(freq * zoom * vmPos.xyz + vec3(0.0, 0.0, temps));
 //  
 //  // Augmenter progressivement la densité des nuages avec le temps
 //  float cloudCoverage = clamp(temps * 0.05, 0.1, 0.9); // Contrôle la couverture du ciel
 //  
 //  // Ajustement de la distribution avec seuil variable
 //  no = smoothstep(cloudCoverage, cloudCoverage + 0.2, no);
 //  return no;
 //}
 //
 //void main(void) {
 //  if(sky != 0) {
 //    // Augmenter la valeur du seuil inférieur dans smoothstep pour avoir plus de nuages
 //    float nu = rug2();
 //    vec4 cloudColor = vec4(1.0, 1.0, 1.0, 1.0);
 //    // Bleu ciel pour le fond
 //    vec4 skyColor = vec4(0.5, 0.7, 1.0, 1.0);
 //    
 //    // Ajuster le seuil pour augmenter la densité des nuages (de 0.1 à 0.0)
 //    // Diminuer l'écart entre les valeurs pour des nuages plus grands
 //    float cloudIntensity = smoothstep(0.0, 1.0, nu);
 //    
 //    // Mélanger le ciel et les nuages
 //    fragColor = mix(skyColor, cloudColor, cloudIntensity);
 //    return;
 //  }
 //  const vec3 vue = vec3(0.0, 0.0, -1.0);
 //  vec4 vLp0 = viewMatrix * Lp0;
 //  vec3 Ld0  = normalize((vmPos - vLp0).xyz);
 //  vec4 vLp1 = viewMatrix * Lp1;
 //  vec3 Ld1  = normalize((vmPos - vLp1).xyz);
 //  vec3 n = normalize(vmNormal);
 //  /* simulation de normal map */
 //  //if(has_noise != 0) {
 //  vec3 T = vec3(1.0, 0.0, 0.0);
 //  vec3 B = cross(T, n);
 //  vec2 no = rug();
 //  n = normalize(4.0 * n + no.x * T + no.y * B);
 //  //}
 //  /* fin simulation */
 //
 //  float intensite_diffus0 = clamp(dot(n, -Ld0), 0.0, 1.0);
 //  float intensite_diffus1 = clamp(dot(n, -Ld1), 0.0, 1.0);
 //  vec3 R0 = reflect(Ld0, n);
 //  vec3 R1 = reflect(Ld1, n);
 //  float intensite_speculaire0 = pow(clamp(dot(R0, -vue), 0.0, 1.0), 400.0);
 //  float intensite_speculaire1 = pow(clamp(dot(R1, -vue), 0.0, 1.0), 400.0);
 //  vec4 diffus = intensite_diffus0 * sdiffus * l0diffus + intensite_diffus1 * sdiffus * l1diffus;
 //  /* diffus = vec4(ivec4(diffus * 4.0)) / 4.0; */ /* cell shading (toon shading) */
 //  vec4 ambient = sambient * lambient;
 //  fragColor = mix(ambient, diffus, 0.75) + intensite_speculaire0 * l0speculaire * sspeculaire + intensite_speculaire1 * l1speculaire * sspeculaire;
 //}