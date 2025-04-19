/*!\file vhs.c
 * \brief GL4Dummies, exemple avec wireframe cube et effet VHS
 * \author Modifié par vous
 * \date avril 19, 2025
 */

 #include <GL4D/gl4duw_SDL2.h>
 #include <GL4D/gl4dm.h>
 #include <GL4D/gl4dg.h>
 #include <GL4D/gl4dh.h>
 
 static void init(void);
 static void draw(void);
 static void resize(int w, int h);
 
 /* variables globales pour la technique de post-processing */
 static GLuint _pId_normal = 0;   /* shader pour le rendu normal */
 static GLuint _pId_vhs = 0;      /* shader pour l'effet VHS */
 static GLuint _cube_vhs = 0;
 static GLuint _fbo = 0;
 static GLuint _texId[2] = {0, 0};
 static GLuint _quad = 0;
 static int _wW = 800, _wH = 600; /* taille par défaut, sera mise à jour */
 
 void vhs(int state) {
   switch(state) {
   case GL4DH_INIT:
     init();
     return;
   case GL4DH_FREE:
     /* Libération des ressources */
     if(_texId[0])
       glDeleteTextures(2, _texId);
     if(_fbo)
       glDeleteFramebuffers(1, &_fbo);
     return;
   case GL4DH_UPDATE_WITH_AUDIO:
     /* METTRE A JOUR VOTRE ANIMATION EN FONCTION DU SON */
     return;
   default: /* GL4DH_DRAW */
     draw();
     return;
   }
 }
 
 void init(void) {
   /* générer un cube en GL4D */
   _cube_vhs = gl4dgGenCubef();
   /* créer un quad pour le post-processing */
   _quad = gl4dgGenQuadf();
   
   /* créer les programmes GPU pour OpenGL (en GL4D) */
   /* programme pour la scène 3D - identique à wf_cube */
   _pId_normal = gl4duCreateProgram("<vs>shaders/wf_cube.vs", "<fs>shaders/wf_cube.fs", NULL);
   /* programme pour le filtre post-process - votre filtre VHS */
   _pId_vhs = gl4duCreateProgram("<vs>shaders/filter.vs", "<fs>shaders/filter.fs", NULL);
   
   /* création des textures pour le rendu hors écran */
   glGenTextures(2, _texId);
   for(int i = 0; i < 2; ++i) {
     glBindTexture(GL_TEXTURE_2D, _texId[i]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
   }
   glBindTexture(GL_TEXTURE_2D, 0);
   
   /* création du FBO (Frame Buffer Object) */
   glGenFramebuffers(1, &_fbo);
   
   /* créer dans GL4D les matrices nécessaires */
   gl4duGenMatrix(GL_FLOAT, "modview");
   gl4duGenMatrix(GL_FLOAT, "proj");
   
   /* configurer la matrice de projection */
   gl4duBindMatrix("proj");
   gl4duLoadIdentityf();
   gl4duFrustumf(-1, 1, -1, 1, 1, 1000);
   
   /* récupérer les dimensions actuelles de la fenêtre */
   int w, h;
   SDL_GetWindowSize(gl4duwGetSDL_Window(), &w, &h);
   resize(w, h);
 }
 
 void resize(int w, int h) {
   float ratio = h / (float)w;
   _wW = w;
   _wH = h;
   
   /* mise à jour des textures */
   for(int i = 0; i < 2; ++i) {
     glBindTexture(GL_TEXTURE_2D, _texId[i]);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
   }
   glBindTexture(GL_TEXTURE_2D, 0);
   
   /* mise à jour de la matrice de projection */
   gl4duBindMatrix("proj");
   gl4duLoadIdentityf();
   gl4duFrustumf(-1, 1, -1 * ratio, 1 * ratio, 1, 1000);
   
   /* mise à jour du viewport */
   glViewport(0, 0, _wW, _wH);
 }
 
 void draw(void) {
   static double t0 = 0;
   double t = gl4dhGetTicks() / 1000.0, dt = t - t0;
   t0 = t;
   static float a = 0;
   
   /* ÉTAPE 1: Rendu de la scène dans une texture */
   glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texId[0], 0);
   
   /* Activer le z-buffer pour le rendu 3D */
   glEnable(GL_DEPTH_TEST);
   
   /* on active le rendu en mode fil de fer */
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   /* set la couleur d'effacement OpenGL */
   glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
   /* effacer le buffer de couleur et le buffer de profondeur */
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   /* utiliser le programme GPU pour la scène */
   glUseProgram(_pId_normal);
   /* binder la matrice modview */
   gl4duBindMatrix("modview");
   /* mettre la matrice identité */
   gl4duLoadIdentityf();
   /* translation dans les z négatifs (-5) */ 
   gl4duTranslatef(0, 0, -5);
   /* rotation d'angle a et d'axe <0, 1, 0> */
   gl4duRotatef(a, 0, 1, 0);
   /* envoyer les matrices au programme GPU */
   gl4duSendMatrices();
   /* dessiner le cube */
   gl4dgDraw(_cube_vhs);
   
   /* ÉTAPE 2: Application du filtre post-process VHS */
   glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texId[1], 0); /* sortie */
   
   /* Désactiver le z-buffer pour le post-processing */
   glDisable(GL_DEPTH_TEST);
   glClear(GL_COLOR_BUFFER_BIT);
   
   /* Mode plein écran pour le quad */
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   
   /* utiliser le shader de post-processing VHS */
   glUseProgram(_pId_vhs);
   
   /* activer la texture contenant la scène rendue */
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texId[0]); /* entrée */
   glUniform1i(glGetUniformLocation(_pId_vhs, "tex"), 0);
   
   /* passer les paramètres du shader VHS */
   glUniform1f(glGetUniformLocation(_pId_vhs, "time"), t);
   glUniform3fv(glGetUniformLocation(_pId_vhs, "rgb_shift_horison"), 1, (GLfloat[]){0.0f, 0.0f, 0.01f});
  glUniform3fv(glGetUniformLocation(_pId_vhs, "rgb_shift_verti"), 1, (GLfloat[]){0.0f, 0.0f, 0.01f});
   /* dessiner un quad plein écran */
   gl4dgDraw(_quad);
   glBindTexture(GL_TEXTURE_2D, 0);
   
   /* ÉTAPE 3: Copier le résultat final à l'écran */
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
   glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
   
   /* augmenter l'angle a */
   a += 60.0 * dt;
 }