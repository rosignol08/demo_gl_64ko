/*!\file window.c
 * \brief affichage du spectre sonore
 * \author Farès BELHADJ, amsi@up8.edu
 * \date Janvier 20 2025 */
#include <stdio.h>
#include <GL4D/gl4du.h>
#include "tiny4D.h"
#include "audioHelper.h"
/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void draw(void);
/*!\brief nombre d'échantillons du signal sonore */
#define ECHANTILLONS 1024
/*!\brief amplitude des échantillons du signal sonore */
static Sint16 _hauteurs[ECHANTILLONS];
/*!\brief dimensions de la fenêtre */
static int _wW = ECHANTILLONS, _wH = 256;

/*!\brief id du screen à créer */
static GLuint _screen = 0;


void basic_audio(int state) {
  /* INITIALISEZ VOS VARIABLES */
  /* ... */
  switch(state) {
  case GL4DH_INIT:
    /* INITIALISEZ VOTRE ANIMATION (SES VARIABLES <STATIC>s) */
    _screen = gl4dpInitScreenWithDimensions(_wW, _wH);
    return;
  case GL4DH_FREE:
    /* LIBERER LA MEMOIRE UTILISEE PAR LES <STATIC>s */
    return;
  case GL4DH_UPDATE_WITH_AUDIO: {
    /* METTRE A JOUR VOTRE ANIMATION EN FONCTION DU SON */
    int i, len = ahGetAudioStreamLength();
    Sint16 *s = (Sint16 *)ahGetAudioStream();
    if(len >= 2 * ECHANTILLONS)
      for(i = 0; i < ECHANTILLONS; i++)
        _hauteurs[i] = _wH / 2 + (_wH / 2) * s[i] / ((1 << 15) - 1.0);
    return;
  }
  default: /* GL4DH_DRAW */
    /* JOUER L'ANIMATION */
    draw();
    return;
  }
}


/*!\brief dessine dans le contexte OpenGL actif. Vous pouvez
 * décommenter pour dessiner avec des lignes. */
static void draw(void) {
  int i, w = gl4dpGetWidth();
  GLuint * pixels = gl4dpGetPixels();

  gl4dpClearScreen();  
  
  for(i = 0; i < ECHANTILLONS; i+=2) {
    int x0, y0;
    x0 = (i * (_wW - 1)) / (ECHANTILLONS - 1);
    y0 = _hauteurs[i];
    pixels[y0 * w + x0] = _rgb(0, 255, 0);
  }
  for(i = 1; i < ECHANTILLONS; i+=2) {
    int x0, y0;
    x0 = (i * (_wW - 1)) / (ECHANTILLONS - 1);
    y0 = _hauteurs[i];
    pixels[y0 * w + x0] = _rgb(255, 0, 0);
  }
  gl4dpScreenHasChanged();
  gl4dpUpdateScreen(NULL);
}


