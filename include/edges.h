#ifndef EDGES_H
#define EDGES_H

#include "image.h"

/*
 * Support 05 - Detection de contours.
 * Operateurs de derivee premiere (Roberts, Prewitt, Sobel), derivee seconde
 * (Laplacien), norme du gradient avec seuillage, et transformee de Hough.
 */

/* Norme du gradient par les masques de Roberts */
Image *edge_roberts(const Image *im);

/* Norme du gradient par les masques de Prewitt */
Image *edge_prewitt(const Image *im);

/* Norme du gradient par les masques de Sobel */
Image *edge_sobel(const Image *im);

/* Laplacien (derivee seconde). variant8=1 utilise le masque 8-connexe */
Image *edge_laplacian(const Image *im, int variant8);

/* Gradient de Sobel puis seuillage: pixel contour (255) si |G| >= seuil */
Image *edge_gradient_threshold(const Image *im, int seuil);

/*
 * Transformee de Hough pour la detection de droites.
 * - seuil : seuil sur le gradient de Sobel pour selectionner les pixels contour
 * - votes_min : nombre de votes minimal pour valider une droite
 * Si accumulator != 0, renvoie l'image de l'accumulateur (rho x theta).
 * Sinon, renvoie l'image d'origine (gris) avec les droites tracees en blanc.
 */
Image *edge_hough_lines(const Image *im, int seuil, int votes_min, int accumulator);

#endif /* EDGES_H */
