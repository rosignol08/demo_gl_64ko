/*!\file credits.c
 * \brief Animation de crédits pour la démo
 * \author Romaric Chaffray
 * \date mai 2025
 */
#include <stdio.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dp.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL2/SDL_ttf.h>
#include <GL4D/gl4dh.h>

static GLuint _pId_text2 = 0;
static GLuint _quad_text2 = 0;
static GLuint _textTexId1 = 0;

static void initText(GLuint * ptId, const char * text);
static void init(void);
static void draw(void);
/* Animation de crédits */
void texte2(int state) {
  
  switch(state) {
  case GL4DH_INIT:
    init();
    return;
  
  case GL4DH_FREE:
    if(_textTexId1) {
      glDeleteTextures(1, &_textTexId1);
      _textTexId1 = 0;
    }
    if(_quad_text2) {
      glDeleteVertexArrays(1, &_quad_text2);
      _quad_text2 = 0;
    }
    if(_pId_text2) {
      glDeleteProgram(_pId_text2);
      _pId_text2 = 0;
    }
    //glBindTexture(GL_TEXTURE_2D, 0);
    return;
  
  case GL4DH_UPDATE_WITH_AUDIO:
    /* Rien à faire ici pour le moment */
    return;
  
  default: /* GL4DH_DRAW */
    draw();
    return;
  }
}

static void init(void){
  glEnable(GL_DEPTH_TEST);
  _pId_text2 = gl4duCreateProgram("<vs>shaders/credits.vs", "<fs>shaders/credits.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  _quad_text2 = gl4dgGenQuadf();
  
    //les texte
    initText(&_textTexId1,
      "Une carpe s’élança.\n"
      "Échouant nuit et jour,\n"
      "sans abandonner.");
  /*
    initText(&_textTexId2,
      "Une carpe, se lança dans l’ascension du fleuve.\n"
      "Nuit et jour, elle lutta contre les remous et le courant,\n"
      "échouant toujours à franchir la cascade sacrée.\n"
      "Les jours passaient, mais elle refusait d’abandonner.");
  initText(&_textTexId3,
    "À bout de forces, la carpe murmura une prière à l’esprit du fleuve.\n"
    "Le courant ralentit, et l’eau prit une teinte dorée.\n"
    "Guidée par l’esprit, la carpe s’élança une dernière fois..."
  );
    initText(&_textTexId4,
      " Elle franchit la porte, à la floraison des cerisiers, et se transforma en dragon.\n"
      "L’esprit du fleuve lui offrit un souffle de vie éternelle.\n"
      "La carpe, devenue dragon, s’envola dans le ciel, illuminant le monde de sa lumière dorée.");
  
  
  initText(&_textTexId5,
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
         */
}

static void draw(void) {
  static double t0 = 0.0;
  double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
  t0 = t;
    
    glClearColor(0.0, 0.0, 0.0, 1); //fond noir
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(_pId_text2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textTexId1);  
    
    glUniform1i(glGetUniformLocation(_pId_text2, "inv"), 1);
    glUniform1i(glGetUniformLocation(_pId_text2, "tex"), 0);
    glUniform1f(glGetUniformLocation(_pId_text2, "time"), t);
    
    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    
    // Position les crédits au centre avec une légère animation 
    gl4duScalef(1.0, 0.50, 1.0);
    gl4duTranslatef(0.0, 0.10, -2.30);
    
    gl4duSendMatrices();
    gl4dgDraw(_quad_text2);
    //printf("t = %f\n", t);
    glUseProgram(0);
}

//texture avec du texte
static void initText(GLuint * ptId, const char * text) {
  static int firstTime = 1;
  SDL_Color c = {229, 229, 229, 255}; // Couleur blanc gris
  SDL_Surface * d, * s;
  TTF_Font * font = NULL;
  if(firstTime) {
    //init de la bibliothèque SDL2 ttf
    if(TTF_Init() == -1) {
      fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
      exit(2);
    }
    firstTime = 0;
  }
  if(*ptId == 0) {
    //initialisation de la texture côté OpenGL
    glGenTextures(1, ptId);
    glBindTexture(GL_TEXTURE_2D, *ptId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  
  //test des polices dispo
  const char* fontPaths[] = {
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/ubuntu/Ubuntu-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
  };
  
  //Essayer chaque police jusqu'à en trouver une qui fonctionne
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
  
  //création d'une surface SDL avec le texte
  d = TTF_RenderUTF8_Blended_Wrapped(font, text, c, 2048);
  if(d == NULL) {
    TTF_CloseFont(font);
    fprintf(stderr, "Erreur lors du TTF_RenderText\n");
    return;
  }
  //copie de la surface SDL vers une seconde aux spécifications qui correspondent au format OpenGL
  s = SDL_CreateRGBSurface(0, d->w, d->h, 32, R_MASK, G_MASK, B_MASK, A_MASK);
  assert(s);
  SDL_BlitSurface(d, NULL, s, NULL);
  SDL_FreeSurface(d);
  //transfert vers la texture OpenGL
  glBindTexture(GL_TEXTURE_2D, *ptId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
  fprintf(stderr, "Dimensions de la texture : %d %d\n", s->w, s->h);
  SDL_FreeSurface(s);
  TTF_CloseFont(font);
  glBindTexture(GL_TEXTURE_2D, 0);
}