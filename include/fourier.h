#ifndef FOURIER_H
#define FOURIER_H

#include "image.h"

/*
 * Support 04 - Transformee de Fourier.
 * L'image est completee (zero-padding) a des dimensions puissances de 2 pour
 * appliquer une FFT 2D (Cooley-Tukey).
 */

/* Spectre de Fourier rehausse: log(1 + |F|), quadrants recentres, [0,255] */
Image *fourier_spectrum(const Image *im);

/* Filtrage passe-bas ideal dans le domaine frequentiel (rayon en pixels) */
Image *fourier_lowpass(const Image *im, double radius);

/* Filtrage passe-haut ideal dans le domaine frequentiel (rayon en pixels) */
Image *fourier_highpass(const Image *im, double radius);

#endif /* FOURIER_H */
