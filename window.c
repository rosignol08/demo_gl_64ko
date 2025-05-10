/*!\file window.c
 *
 * \brief Utilisation de GL4Dummies pour réaliser une démo.
 *
 * Ici est géré l'ouverture de la fenêtre ainsi que l'ordonnancement
 * des animations. Apriori les seuls éléments à modifier ici lors de
 * votre intégration sont le tableau static \ref _animations et le nom
 * du fichier audio à lire.
 *
 * \author Farès BELHADJ, amsi@up8.edu
 * \date April 12, 2023
 */
#include <stdlib.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dh.h>
#include <GL4D/gl4duw_SDL2.h>
#include "animations.h"
#include "audioHelper.h"

/* Prototypes des fonctions statiques contenues dans ce fichier C. */
static void init(void);
static void quit(void);
static void resize(int w, int h);
static void keydown(int keycode);

/*!\brief tableau contenant les animations sous la forme de timeline,
 * ce tableau se termine toujours par l'élémént {0, NULL, NULL,
 * NULL} */
static GL4DHanime _animations[] = {
  { 13000, credits, NULL, NULL},//intro texte
  { 4000, intro_arabesque, NULL, NULL },//dessin fin : 17s
  { 2000, intro_arabesque, eau_scene, fondu },
  { 9000, eau_scene, NULL, NULL },//28 sec
  { 9000, credits, NULL, NULL},//38 sec jours et nuit texte
  { 2000, credits, montagne, fondui},
  { 9000, montagne, NULL, NULL },//48 sec
  { 15000, credits, NULL, NULL },//1:03 sec esprit de l'eau
  { 8000, balle_song, NULL, NULL },
  { 2000, balle_song, arbre, pixels },
  { 10000, arbre, NULL, NULL },//1:23 arbre fleuri
  { 19000, eau_scene, NULL, NULL },//eau reussi 1:43
  { 1000, eau_scene, blanc, NULL },//transition flash jsp
  { 10000, vert, NULL, NULL },//1:53 TODO: faire la scene porte_sacree
  { 10000, credits, NULL, NULL}, //txt4
  { 10000, credits, NULL, NULL}, //credits de fin
  { 0, NULL, NULL, NULL } /* Toujours laisser à la fin */
};

/*!\brief dimensions initiales de la fenêtre */
static GLfloat _dim[] = {1920, 1080};

/*!\brief fonction principale : initialise la fenêtre, OpenGL, audio
 * et lance la boucle principale (infinie).
 */
int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "Ateliers API8 - démo", 
			 GL4DW_POS_UNDEFINED, GL4DW_POS_UNDEFINED, 
			 _dim[0], _dim[1],
			 /* GL4DW_RESIZABLE |  */GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwKeyDownFunc(keydown);
  gl4duwDisplayFunc(gl4dhDraw);

  //ahInitAudio("snow-waltz-winter-piano-280274.mid");
  ahInitAudio("flute.mid");
  gl4duwMainLoop();
  return 0;
}

/*!\brief Cette fonction initialise les paramètres et éléments OpenGL
 * ainsi que les divers données et fonctionnalités liées à la gestion
 * des animations.
 */
static void init(void) {
  SDL_GL_SetSwapInterval(1);//la sycronisation verticale
  glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
  gl4dhInit(_animations, _dim[0], _dim[1], animationsInit);
  resize(_dim[0], _dim[1]);
}

/*!\brief paramétre la vue (viewPort) OpenGL en fonction des
 * dimensions de la fenêtre.
 * \param w largeur de la fenêtre.
 * \param w hauteur de la fenêtre.
 */
static void resize(int w, int h) {
  _dim[0] = w; _dim[1] = h;
  glViewport(0, 0, _dim[0], _dim[1]);
}

/*!\brief permet de gérer les évènements clavier-down.
 * \param keycode code de la touche pressée.
 */
static void keydown(int keycode) {
  switch(keycode) {
  case SDLK_ESCAPE:
  case 'q':
    exit(0);
  default: break;
  }
}

/*!\brief appelée à la sortie du programme (atexit).
 */
static void quit(void) {
  ahClean();
  gl4duClean(GL4DU_ALL);
}
