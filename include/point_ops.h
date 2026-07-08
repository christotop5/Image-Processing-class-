#ifndef POINT_OPS_H
#define POINT_OPS_H

#include "image.h"

/*
 * Support 02 - Transformations ponctuelles et operations entre images.
 * Chaque pixel de sortie ne depend que du/des pixel(s) correspondant(s).
 */

/* Transformation lineaire: etire [min,max] vers [0,255] (via LUT) */
Image *op_linear(const Image *im);

/* Transformation lineaire avec saturation entre les seuils smin et smax */
Image *op_linear_saturation(const Image *im, int smin, int smax);

/* Transformation lineaire par morceaux: definie par un point de controle
 * (in1,out1)-(in2,out2), lineaire par segments. */
Image *op_piecewise_linear(const Image *im, int in1, int out1,
                           int in2, int out2);

/* Transformation non-lineaire: correction gamma  s = 255*(r/255)^gamma */
Image *op_gamma(const Image *im, double gamma);

/* Negatif de l'image: s = max_value - r */
Image *op_negative(const Image *im);

/* --- Operations arithmetiques entre deux images (support 02) --- */
Image *op_add(const Image *a, const Image *b);        /* min(a+b,255) */
Image *op_sub(const Image *a, const Image *b);        /* max(a-b,0)   */
Image *op_mul_scalar(const Image *a, double ratio);   /* min(a*ratio,255) */
Image *op_blend(const Image *a, const Image *b, double alpha); /* a*alpha + b*(1-alpha) */

/* --- Operations logiques (support 02) --- */
Image *op_and(const Image *a, const Image *b);
Image *op_or(const Image *a, const Image *b);
Image *op_xor(const Image *a, const Image *b);
Image *op_not(const Image *a);

#endif /* POINT_OPS_H */
