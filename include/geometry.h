#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "image.h"

/*
 * Support 02 - Changement d'echelle par interpolation.
 * Fonctionne pour les images gris et couleur.
 */

/* Interpolation par plus proche voisin */
Image *resize_nearest(const Image *im, int new_w, int new_h);

/* Interpolation bilineaire (4 voisins) */
Image *resize_bilinear(const Image *im, int new_w, int new_h);

/* Interpolation bicubique (16 voisins) */
Image *resize_bicubic(const Image *im, int new_w, int new_h);

#endif /* GEOMETRY_H */
