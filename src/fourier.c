#include "fourier.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Plus petite puissance de 2 >= n */
static int next_pow2(int n) {
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

/* FFT 1D iterative (Cooley-Tukey) sur re/im de taille n (puissance de 2).
 * sign = -1 pour la transformee directe, +1 pour l'inverse. */
static void fft1d(double *re, double *im, int n, int sign) {
    /* Permutation par inversion de bits */
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            double tr = re[i]; re[i] = re[j]; re[j] = tr;
            double ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = sign * 2.0 * M_PI / len;
        double wr = cos(ang), wi = sin(ang);
        for (int i = 0; i < n; i += len) {
            double cr = 1.0, ci = 0.0;
            for (int k = 0; k < len / 2; k++) {
                int a = i + k, b = i + k + len / 2;
                double tr = cr * re[b] - ci * im[b];
                double ti = cr * im[b] + ci * re[b];
                re[b] = re[a] - tr; im[b] = im[a] - ti;
                re[a] += tr;        im[a] += ti;
                double ncr = cr * wr - ci * wi;
                ci = cr * wi + ci * wr;
                cr = ncr;
            }
        }
    }
    if (sign == 1) {
        for (int i = 0; i < n; i++) { re[i] /= n; im[i] /= n; }
    }
}

/* FFT 2D en place sur des matrices W x H (puissances de 2) */
static void fft2d(double *re, double *im, int W, int H, int sign) {
    double *rr = (double *)malloc(sizeof(double) * (W > H ? W : H));
    double *ii = (double *)malloc(sizeof(double) * (W > H ? W : H));
    /* lignes */
    for (int y = 0; y < H; y++)
        fft1d(re + y * W, im + y * W, W, sign);
    /* colonnes */
    for (int x = 0; x < W; x++) {
        for (int y = 0; y < H; y++) { rr[y] = re[y * W + x]; ii[y] = im[y * W + x]; }
        fft1d(rr, ii, H, sign);
        for (int y = 0; y < H; y++) { re[y * W + x] = rr[y]; im[y * W + x] = ii[y]; }
    }
    free(rr); free(ii);
}

/* Prepare les buffers complexes a partir d'une image (zero-padding) */
static int prepare(const Image *im, double **re, double **im_buf,
                   int *W, int *H, Image **gray) {
    Image *g = img_to_gray(im);
    if (!g) return 0;
    int w = next_pow2(g->width);
    int h = next_pow2(g->height);
    double *R = (double *)calloc((size_t)w * h, sizeof(double));
    double *I = (double *)calloc((size_t)w * h, sizeof(double));
    if (!R || !I) { free(R); free(I); img_free(g); return 0; }
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            R[y * w + x] = img_get(g, x, y, 0);
    *re = R; *im_buf = I; *W = w; *H = h; *gray = g;
    return 1;
}

Image *fourier_spectrum(const Image *im) {
    double *re, *ii; int W, H; Image *g;
    if (!prepare(im, &re, &ii, &W, &H, &g)) return NULL;
    fft2d(re, ii, W, H, -1);

    /* magnitude log, recentree (fftshift) */
    double *mag = (double *)malloc(sizeof(double) * W * H);
    double mx = 0.0;
    for (int i = 0; i < W * H; i++) {
        mag[i] = log(1.0 + sqrt(re[i] * re[i] + ii[i] * ii[i]));
        if (mag[i] > mx) mx = mag[i];
    }
    Image *out = img_create(W, H, 1, 255);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int sx = (x + W / 2) % W;
            int sy = (y + H / 2) % H;
            double v = (mx > 0.0) ? mag[sy * W + sx] / mx * 255.0 : 0.0;
            img_set(out, x, y, 0, clamp255(v));
        }
    }
    free(mag); free(re); free(ii); img_free(g);
    return out;
}

/* Filtrage frequentiel ideal. highpass=0 -> passe-bas, 1 -> passe-haut */
static Image *fourier_filter(const Image *im, double radius, int highpass) {
    double *re, *ii; int W, H; Image *g;
    if (!prepare(im, &re, &ii, &W, &H, &g)) return NULL;
    fft2d(re, ii, W, H, -1);

    int cx = W / 2, cy = H / 2;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            /* distance a la composante continue (0,0), en tenant compte du
             * repliement periodique du spectre */
            int fx = (x <= cx) ? x : x - W;
            int fy = (y <= cy) ? y : y - H;
            double d = sqrt((double)fx * fx + (double)fy * fy);
            int keep = highpass ? (d >= radius) : (d <= radius);
            if (!keep) { re[y * W + x] = 0.0; ii[y * W + x] = 0.0; }
        }
    }
    fft2d(re, ii, W, H, +1);

    Image *out = img_create(g->width, g->height, 1, 255);
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            img_set(out, x, y, 0, clamp255(re[y * W + x]));
    free(re); free(ii); img_free(g);
    return out;
}

Image *fourier_lowpass(const Image *im, double radius) {
    if (radius <= 0) radius = 30;
    return fourier_filter(im, radius, 0);
}

Image *fourier_highpass(const Image *im, double radius) {
    if (radius <= 0) radius = 20;
    return fourier_filter(im, radius, 1);
}
