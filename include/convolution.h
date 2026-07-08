#ifndef CONVOLUTION_H
#define CONVOLUTION_H

#include "image.h"

/*
 * Support 03 - Convolution et filtrage spatial.
 * Le traitement se fait sur des images en niveaux de gris.
 */

/* Gestion des bords lors de la convolution */
typedef enum {
    BORDER_ZERO,   /* pixels hors image = 0 */
    BORDER_MIRROR, /* miroir de l'image */
    BORDER_REPLICATE /* replique le pixel de bord */
} BorderMode;

/*
 * Convolution generique par un noyau carre ksize x ksize.
 * Le resultat est divise par `divisor` (mettre 0 pour utiliser la somme des
 * coefficients, ou 1 si le noyau est deja normalise).
 */
Image *convolve(const Image *im, const double *kernel, int ksize,
                double divisor, BorderMode border);

/* Filtre moyenneur (passe-bas) NxN */
Image *filter_mean(const Image *im, int ksize);

/* Filtre gaussien 5x5 (noyau du cours) */
Image *filter_gaussian(const Image *im);

/* Filtre median NxN (non lineaire) */
Image *filter_median(const Image *im, int ksize);

/* Filtres d'ordre min / max sur un voisinage NxN */
Image *filter_min(const Image *im, int ksize);
Image *filter_max(const Image *im, int ksize);

#endif /* CONVOLUTION_H */
