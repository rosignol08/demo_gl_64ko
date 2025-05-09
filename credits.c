/*!\file credits.c
 * \brief Animation de crédits pour la démo
 * \author Farès BELHADJ
 * \author Dounia HULLOT
 * \date Avril 2025
 */
#include <stdio.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dp.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_ttf.h>
#include <GL4D/gl4dh.h>

static GLuint _pId = 0;
static GLuint _quad = 0;
static GLuint _textTexId1 = 0;
static GLuint _textTexId2 = 0;
static GLuint _textTexId3 = 0;

static GLfloat t0 = -1;
GLfloat t;
static void initText(GLuint * ptId, const char * text);
static void init(void);
static void draw(void);
/* Animation de crédits */
void credits(int state) {
  
  switch(state) {
  case GL4DH_INIT:
    init();
    return;
  
  case GL4DH_FREE:
    if(_textTexId1) {
      glDeleteTextures(1, &_textTexId1);
      _textTexId1 = 0;
    }
    return;
  
  case GL4DH_UPDATE_WITH_AUDIO:
    /* Rien à faire ici pour le moment */
    return;
  
  default: /* GL4DH_DRAW */
    draw();
    return;
  }
}

/*!\brief création d'une texture avec du texte. */
static void initText(GLuint * ptId, const char * text) {
  static int firstTime = 1;
  SDL_Color c = {255, 165, 0, 255}; // Couleur orange feu
  SDL_Surface * d, * s;
  TTF_Font * font = NULL;
  if(firstTime) {
    /* initialisation de la bibliothèque SDL2 ttf */
    if(TTF_Init() == -1) {
      fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
      exit(2);
    }
    firstTime = 0;
  }
  if(*ptId == 0) {
    /* initialisation de la texture côté OpenGL */
    glGenTextures(1, ptId);
    glBindTexture(GL_TEXTURE_2D, *ptId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  
  /* Essayer les polices dans l'ordre de préférence */
  const char* fontPaths[] = {
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/ubuntu/Ubuntu-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
  };
  
  /* Essayer chaque police jusqu'à en trouver une qui fonctionne */
  for (int i = 0; i < sizeof(fontPaths) / sizeof(fontPaths[0]); i++) {
    font = TTF_OpenFont(fontPaths[i], 128);
    if (font) {
      fprintf(stderr, "Police trouvée: %s\n", fontPaths[i]);
      break;
    }
  }
  
  if (!font) {
    fprintf(stderr, "Impossible de trouver une police TTF utilisable.\n");
    return;
  }
  
  /* création d'une surface SDL avec le texte */
  d = TTF_RenderUTF8_Blended_Wrapped(font, text, c, 2048);
  if(d == NULL) {
    TTF_CloseFont(font);
    fprintf(stderr, "Erreur lors du TTF_RenderText\n");
    return;
  }
  /* copie de la surface SDL vers une seconde aux spécifications qui correspondent au format OpenGL */
  s = SDL_CreateRGBSurface(0, d->w, d->h, 32, R_MASK, G_MASK, B_MASK, A_MASK);
  assert(s);
  SDL_BlitSurface(d, NULL, s, NULL);
  SDL_FreeSurface(d);
  /* transfert vers la texture OpenGL */
  glBindTexture(GL_TEXTURE_2D, *ptId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
  fprintf(stderr, "Dimensions de la texture : %d %d\n", s->w, s->h);
  SDL_FreeSurface(s);
  TTF_CloseFont(font);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void init(void){
  _pId = gl4duCreateProgram("<vs>shaders/credits.vs", "<fs>shaders/credits.fs", NULL);
  _quad = gl4dgGenQuadf();
  
  /* Initialisation du texte de crédits */
  initText(&_textTexId1, 
         "EMBER\n\n\n"
         "Une démo 64ko qui explore le thème du feu à travers diverses animations\n"
         "et effets visuels.\n\n\n"
         "MUSIQUE:\n"
         "\"Embers\"\n"
         "\"Musique originale créée par Dounia HULLOT avec GarageBand.\"\n"
         "\"Conversion/édition audio réalisée avec Audacity.\"\n"
         "Licence : Musique originale - libre de droits.\n\n\n"
         "DÉVELOPPÉ PAR:\n"
         "Dounia HULLOT\n\n\n"
         "REMERCIEMENTS:\n"
         "API8 - Université Paris 8\n"
         "Farès BELHADJ\n\n\n"
         "Avril 2025");
  initText(&_textTexId2,
         "EMBER\n\n\n"
         "Une démo 64ko qui explore le thème du feu à travers diverses animations\n"
         "et effets visuels.\n\n\n"
         "MUSIQUE:\n"
         "\"Embers\"\n"
         "\"Musique originale créée par Dounia HULLOT avec GarageBand.\"\n"
  );
  initText(&_textTexId3,
         "EMBER\n\n\n"
         "Une démo 64ko qui explore le thème du feu à travers diverses animations\n"
         "et effets visuels.\n\n\n"
         "MUSIQUE:\n"
         "\"Embers\"\n"
         "\"Musique originale créée par Dounia HULLOT avec GarageBand.\"\n"
  );
  t0 = -1;

}

void draw(void) {
  static double t0 = 0.0;
  double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
  t0 = t;
    
    glClearColor(0.05, 0.0, 0.0, 1); // Fond noir avec légère teinte rouge
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(_pId);
    if (t0 < 10.0f){
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _textTexId1);  
    }
    else if (t0 < 30.0f){
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _textTexId2);  
    }
    else if (t0 < 40.0f){
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _textTexId3);  
    }
    glUniform1i(glGetUniformLocation(_pId, "inv"), 1);
    glUniform1i(glGetUniformLocation(_pId, "tex"), 0);
    glUniform1f(glGetUniformLocation(_pId, "time"), t);
    
    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    
    // Position les crédits au centre avec une légère animation 
    gl4duScalef(0.8, 0.8, 1.0);
    gl4duTranslatef(0.0, 0.0, -2.0);
    
    gl4duSendMatrices();
    gl4dgDraw(_quad);
    printf("t = %f\n", t);
    glUseProgram(0);
}