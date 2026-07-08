#include "convolution.h"
#include <stdlib.h>
#include <math.h>

/* Renvoie la coordonnee corrigee selon le mode de bord (ou -1 si zero) */
static int border_index(int p, int n, BorderMode mode) {
    if (p >= 0 && p < n) return p;
    switch (mode) {
        case BORDER_ZERO:
            return -1;
        case BORDER_REPLICATE:
            return (p < 0) ? 0 : n - 1;
        case BORDER_MIRROR:
        default:
            if (p < 0) p = -p - 1;
            if (p >= n) p = 2 * n - p - 1;
            if (p < 0) p = 0;
            if (p >= n) p = n - 1;
            return p;
    }
}

Image *convolve(const Image *im, const double *kernel, int ksize,
                double divisor, BorderMode border) {
    if (ksize < 1 || ksize % 2 == 0) return NULL;
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    Image *out = img_create(g->width, g->height, 1, 255);
    if (!out) { img_free(g); return NULL; }

    int r = ksize / 2;
    if (divisor == 0.0) {
        double s = 0.0;
        for (int i = 0; i < ksize * ksize; i++) s += kernel[i];
        divisor = (s == 0.0) ? 1.0 : s;
    }

    for (int y = 0; y < g->height; y++) {
        for (int x = 0; x < g->width; x++) {
            double acc = 0.0;
            for (int ky = -r; ky <= r; ky++) {
                int sy = border_index(y + ky, g->height, border);
                for (int kx = -r; kx <= r; kx++) {
                    int sx = border_index(x + kx, g->width, border);
                    double pv = 0.0;
                    if (sx >= 0 && sy >= 0) pv = img_get(g, sx, sy, 0);
                    acc += pv * kernel[(ky + r) * ksize + (kx + r)];
                }
            }
            img_set(out, x, y, 0, clamp255(acc / divisor));
        }
    }
    img_free(g);
    return out;
}

Image *filter_mean(const Image *im, int ksize) {
    if (ksize < 3) ksize = 3;
    if (ksize % 2 == 0) ksize++;
    int n = ksize * ksize;
    double *k = (double *)malloc(n * sizeof(double));
    if (!k) return NULL;
    for (int i = 0; i < n; i++) k[i] = 1.0;
    Image *out = convolve(im, k, ksize, (double)n, BORDER_MIRROR);
    free(k);
    return out;
}

Image *filter_gaussian(const Image *im) {
    /* Noyau gaussien 5x5 du support 03 (somme = 98) */
    double k[25] = {
        1, 2, 3, 2, 1,
        2, 6, 8, 6, 2,
        3, 8, 10, 8, 3,
        2, 6, 8, 6, 2,
        1, 2, 3, 2, 1
    };
    return convolve(im, k, 5, 98.0, BORDER_MIRROR);
}

static int cmp_uchar(const void *a, const void *b) {
    return (int)(*(const unsigned char *)a) - (int)(*(const unsigned char *)b);
}

/* Filtre d'ordre: 0=min, size-1=max, size/2=median */
static Image *rank_filter(const Image *im, int ksize, int which) {
    if (ksize < 3) ksize = 3;
    if (ksize % 2 == 0) ksize++;
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    Image *out = img_create(g->width, g->height, 1, 255);
    if (!out) { img_free(g); return NULL; }
    int r = ksize / 2;
    int wsize = ksize * ksize;
    unsigned char *win = (unsigned char *)malloc(wsize);
    if (!win) { img_free(g); img_free(out); return NULL; }

    for (int y = 0; y < g->height; y++) {
        for (int x = 0; x < g->width; x++) {
            int cnt = 0;
            for (int ky = -r; ky <= r; ky++) {
                int sy = border_index(y + ky, g->height, BORDER_MIRROR);
                for (int kx = -r; kx <= r; kx++) {
                    int sx = border_index(x + kx, g->width, BORDER_MIRROR);
                    win[cnt++] = img_get(g, sx, sy, 0);
                }
            }
            qsort(win, cnt, 1, cmp_uchar);
            int idx = (which < 0) ? cnt / 2
                     : (which == 0) ? 0
                     : cnt - 1;
            img_set(out, x, y, 0, win[idx]);
        }
    }
    free(win);
    img_free(g);
    return out;
}

Image *filter_median(const Image *im, int ksize) { return rank_filter(im, ksize, -1); }
Image *filter_min(const Image *im, int ksize)    { return rank_filter(im, ksize, 0); }
Image *filter_max(const Image *im, int ksize)    { return rank_filter(im, ksize, 1); }
