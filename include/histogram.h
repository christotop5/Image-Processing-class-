#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "image.h"

/*
 * Support 02 - Traitement de base : histogramme et statistiques.
 * Les operations travaillent sur des images en niveaux de gris.
 */

/* Calcule l'histogramme (256 classes) d'une image gris */
void histogram_gray(const Image *im, int hist[256]);

/* Luminance / brillance = moyenne des pixels */
double img_mean(const Image *im);

/* Contraste = ecart-type des niveaux de gris */
double img_stddev(const Image *im);

/* Dynamique = niveaux min et max presents dans l'image */
void img_minmax(const Image *im, int *vmin, int *vmax);

/* Egalisation d'histogramme (globale) pour ameliorer le contraste */
Image *histogram_equalize(const Image *im);

/* Egalisation locale sur une fenetre win x win autour de chaque pixel */
Image *histogram_equalize_local(const Image *im, int win);

/* Etirement de la dynamique (histogram stretch) sur [0,255] */
Image *histogram_stretch(const Image *im);

/* Genere une image (256 x hauteur) representant l'histogramme (barres) */
Image *histogram_to_image(const Image *im, int height);

#endif /* HISTOGRAM_H */
