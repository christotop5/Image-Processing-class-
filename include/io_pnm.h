#ifndef IO_PNM_H
#define IO_PNM_H

#include "image.h"

/*
 * Lecture/ecriture des formats PNM non compresses (support 01 du cours):
 *   - PGM : niveaux de gris 8 bits  (magic P2 ASCII / P5 binaire)
 *   - PPM : couleur RVB 24 bits     (magic P3 ASCII / P6 binaire)
 *
 * pnm_read  detecte automatiquement le format d'apres l'entete.
 * pnm_write ecrit en binaire (P5 pour le gris, P6 pour la couleur).
 * Retour: pointeur Image (a liberer) ou NULL / code d'erreur.
 */
Image *pnm_read(const char *path);
int    pnm_write(const char *path, const Image *img);

#endif /* IO_PNM_H */
