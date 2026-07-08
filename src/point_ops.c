#include "point_ops.h"
#include "histogram.h"
#include <stdlib.h>
#include <math.h>

/* Applique une LUT a une image gris (renvoie une nouvelle image) */
static Image *apply_lut(const Image *im, const unsigned char lut[256]) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    size_t n = (size_t)g->width * g->height;
    for (size_t i = 0; i < n; i++)
        g->data[i] = lut[g->data[i]];
    return g;
}

Image *op_linear(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    int mn, mx;
    img_minmax(g, &mn, &mx);
    if (mx == mn) return g;
    unsigned char lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = clamp255(255.0 * (i - mn) / (double)(mx - mn));
    size_t n = (size_t)g->width * g->height;
    for (size_t i = 0; i < n; i++) g->data[i] = lut[g->data[i]];
    return g;
}

Image *op_linear_saturation(const Image *im, int smin, int smax) {
    if (smax <= smin) { smin = 0; smax = 255; }
    unsigned char lut[256];
    for (int i = 0; i < 256; i++) {
        if (i <= smin) lut[i] = 0;
        else if (i >= smax) lut[i] = 255;
        else lut[i] = clamp255(255.0 * (i - smin) / (double)(smax - smin));
    }
    return apply_lut(im, lut);
}

Image *op_piecewise_linear(const Image *im, int in1, int out1,
                           int in2, int out2) {
    if (in2 <= in1) { in1 = 85; in2 = 170; out1 = 30; out2 = 225; }
    unsigned char lut[256];
    for (int i = 0; i < 256; i++) {
        double v;
        if (i <= in1)
            v = (in1 == 0) ? out1 : (double)out1 * i / in1;
        else if (i <= in2)
            v = out1 + (double)(out2 - out1) * (i - in1) / (in2 - in1);
        else
            v = out2 + (double)(255 - out2) * (i - in2) / (255 - in2);
        lut[i] = clamp255(v);
    }
    return apply_lut(im, lut);
}

Image *op_gamma(const Image *im, double gamma) {
    if (gamma <= 0.0) gamma = 1.0;
    unsigned char lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = clamp255(255.0 * pow(i / 255.0, gamma));
    return apply_lut(im, lut);
}

Image *op_negative(const Image *im) {
    Image *out = img_copy(im);
    if (!out) return NULL;
    size_t n = (size_t)out->width * out->height * out->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = (unsigned char)(out->max_value - out->data[i]);
    return out;
}

/* Verifie que deux images ont les memes dimensions et canaux */
static int same_geometry(const Image *a, const Image *b) {
    return a && b && a->width == b->width && a->height == b->height &&
           a->channels == b->channels;
}

Image *op_add(const Image *a, const Image *b) {
    if (!same_geometry(a, b)) return NULL;
    Image *out = img_copy(a);
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = clamp255((double)a->data[i] + b->data[i]);
    return out;
}

Image *op_sub(const Image *a, const Image *b) {
    if (!same_geometry(a, b)) return NULL;
    Image *out = img_copy(a);
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = clamp255((double)a->data[i] - b->data[i]);
    return out;
}

Image *op_mul_scalar(const Image *a, double ratio) {
    Image *out = img_copy(a);
    if (!out) return NULL;
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = clamp255((double)a->data[i] * ratio);
    return out;
}

Image *op_blend(const Image *a, const Image *b, double alpha) {
    if (!same_geometry(a, b)) return NULL;
    if (alpha < 0) alpha = 0;
    if (alpha > 1) alpha = 1;
    Image *out = img_copy(a);
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = clamp255(alpha * a->data[i] + (1.0 - alpha) * b->data[i]);
    return out;
}

Image *op_and(const Image *a, const Image *b) {
    if (!same_geometry(a, b)) return NULL;
    Image *out = img_copy(a);
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = a->data[i] & b->data[i];
    return out;
}

Image *op_or(const Image *a, const Image *b) {
    if (!same_geometry(a, b)) return NULL;
    Image *out = img_copy(a);
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = a->data[i] | b->data[i];
    return out;
}

Image *op_xor(const Image *a, const Image *b) {
    if (!same_geometry(a, b)) return NULL;
    Image *out = img_copy(a);
    size_t n = (size_t)a->width * a->height * a->channels;
    for (size_t i = 0; i < n; i++)
        out->data[i] = a->data[i] ^ b->data[i];
    return out;
}

Image *op_not(const Image *a) {
    return op_negative(a);
}
