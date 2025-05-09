/*!\file animations.h
 *
 * \brief Votre espace de liberté : c'est ici que vous pouvez ajouter
 * vos fonctions de transition et d'animation avant de les faire
 * référencées dans le tableau _animations du fichier \ref window.c
 *
 * Des squelettes d'animations et de transitions sont fournis pour
 * comprendre le fonctionnement de la bibliothèque. En bonus des
 * exemples dont un fondu en GLSL.
 *
 * \author Farès BELHADJ, amsi@up8.edu
 * \date April 12, 2023
 */
#ifndef _ANIMATIONS_H

#define _ANIMATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

  extern void fondu(void (* a0)(int), void (* a1)(int), Uint32 t, Uint32 et, int state);
  extern void fondui(void (* a0)(int), void (* a1)(int), Uint32 t, Uint32 et, int state);
  extern void pixels(void (* a0)(int), void (* a1)(int), Uint32 t, Uint32 et, int state);

  extern void rouge(int state);
  extern void vert(int state);
  extern void bleu(int state);
  extern void noir(int state);
  extern void blanc(int state);
  extern void couleur_du_mur(int state);
  extern void animationsInit(void);

  /* wf_cube.c */
  extern void wf_cube(int state);
  /* basic_audio.v */
  extern void basic_audio(int state);
  /* vhs.c */
  //extern void vhs(int state);
  extern void arbre(int state);
  extern void montagne(int state);
  extern void balle_song(int state);
  extern void eau_scene(int state);
  extern void credits(int state);
  //extern void test_lumi(int state);
  extern void intro_arabesque(int state);
  extern void montagne_arbre(int state);

#ifdef __cplusplus
}
#endif

#endif
