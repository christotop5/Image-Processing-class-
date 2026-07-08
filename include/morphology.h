#ifndef MORPHOLOGY_H
#define MORPHOLOGY_H

#include "image.h"

/*
 * Support 07 - Images binaires et morphologie mathematique.
 * Les operations attendent (ou binarisent) une image en niveaux de gris:
 * objet = 255, fond = 0. La connexite vaut 4 ou 8.
 */

/* Binarise une image gris (seuil) : objet=255, fond=0 */
Image *morph_binarize(const Image *im, int seuil);

Image *morph_erode(const Image *im, int conn);
Image *morph_dilate(const Image *im, int conn);
Image *morph_open(const Image *im, int conn);   /* erosion puis dilatation */
Image *morph_close(const Image *im, int conn);  /* dilatation puis erosion */

/* Gradients morphologiques */
Image *morph_gradient(const Image *im, int conn);          /* dilate - erode */
Image *morph_gradient_internal(const Image *im, int conn); /* orig - erode */
Image *morph_gradient_external(const Image *im, int conn); /* dilate - orig */

/* Etiquetage en composantes connexes: chaque region recoit une couleur (PPM) */
Image *morph_label(const Image *im, int conn, int *nb_components);

/*
 * Carte de distance a partir du fond.
 * metric = 4 (distance de Manhattan D4) ou 8 (distance de l'echiquier D8).
 * Le resultat est normalise sur [0,255] pour l'affichage.
 */
Image *morph_distance(const Image *im, int metric);

/* Seuillage par hysteresis (support 07): deux seuils bas/haut */
Image *morph_hysteresis(const Image *im, int low, int high);

#endif /* MORPHOLOGY_H */
