#include "edges.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/*
 * Applique une paire de masques (gx, gy) de taille ksize et renvoie une
 * carte de norme du gradient (double) de la meme taille que l'image gris.
 */
static double *gradient_map(const Image *g, const double *gx, const double *gy,
                            int ksize) {
    int r = ksize / 2;
    int W = g->width, H = g->height;
    double *norm = (double *)malloc(sizeof(double) * W * H);
    if (!norm) return NULL;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            double sx = 0.0, sy = 0.0;
            for (int ky = -r; ky <= r; ky++) {
                int yy = clampi(y + ky, 0, H - 1);
                for (int kx = -r; kx <= r; kx++) {
                    int xx = clampi(x + kx, 0, W - 1);
                    double v = img_get(g, xx, yy, 0);
                    int idx = (ky + r) * ksize + (kx + r);
                    sx += v * gx[idx];
                    sy += v * gy[idx];
                }
            }
            norm[y * W + x] = sqrt(sx * sx + sy * sy);
        }
    }
    return norm;
}

/* Convertit une carte double en image gris normalisee sur [0,255] */
static Image *map_to_image(const double *map, int W, int H) {
    double mx = 0.0;
    for (int i = 0; i < W * H; i++) if (map[i] > mx) mx = map[i];
    Image *out = img_create(W, H, 1, 255);
    if (!out) return NULL;
    for (int i = 0; i < W * H; i++)
        out->data[i] = clamp255(mx > 0 ? map[i] / mx * 255.0 : 0.0);
    return out;
}

Image *edge_roberts(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    /* Masques 2x2 places dans un cadre 3x3 (coin superieur gauche) */
    double gx[9] = { -1, 0, 0,  0, 1, 0,  0, 0, 0 };
    double gy[9] = {  0,-1, 0,  1, 0, 0,  0, 0, 0 };
    double *m = gradient_map(g, gx, gy, 3);
    Image *out = map_to_image(m, g->width, g->height);
    free(m); img_free(g);
    return out;
}

Image *edge_prewitt(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    double gx[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
    double gy[9] = { -1,-1,-1,  0, 0, 0,  1, 1, 1 };
    double *m = gradient_map(g, gx, gy, 3);
    Image *out = map_to_image(m, g->width, g->height);
    free(m); img_free(g);
    return out;
}

Image *edge_sobel(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    double gx[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    double gy[9] = { -1,-2,-1,  0, 0, 0,  1, 2, 1 };
    double *m = gradient_map(g, gx, gy, 3);
    Image *out = map_to_image(m, g->width, g->height);
    free(m); img_free(g);
    return out;
}

Image *edge_laplacian(const Image *im, int variant8) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    double k4[9] = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
    double k8[9] = { 1, 1, 1, 1, -8, 1, 1, 1, 1 };
    double *k = variant8 ? k8 : k4;
    int W = g->width, H = g->height;
    double *m = (double *)malloc(sizeof(double) * W * H);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            double s = 0.0;
            for (int ky = -1; ky <= 1; ky++) {
                int yy = clampi(y + ky, 0, H - 1);
                for (int kx = -1; kx <= 1; kx++) {
                    int xx = clampi(x + kx, 0, W - 1);
                    s += img_get(g, xx, yy, 0) * k[(ky + 1) * 3 + (kx + 1)];
                }
            }
            m[y * W + x] = fabs(s); /* valeur absolue pour la visualisation */
        }
    }
    Image *out = map_to_image(m, W, H);
    free(m); img_free(g);
    return out;
}

Image *edge_gradient_threshold(const Image *im, int seuil) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    double gx[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    double gy[9] = { -1,-2,-1,  0, 0, 0,  1, 2, 1 };
    double *m = gradient_map(g, gx, gy, 3);
    int W = g->width, H = g->height;
    Image *out = img_create(W, H, 1, 255);
    for (int i = 0; i < W * H; i++)
        out->data[i] = (m[i] >= seuil) ? 255 : 0;
    free(m); img_free(g);
    return out;
}

Image *edge_hough_lines(const Image *im, int seuil, int votes_min, int accumulator) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    int W = g->width, H = g->height;

    /* 1) carte de contours par Sobel + seuillage */
    double gx[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    double gy[9] = { -1,-2,-1,  0, 0, 0,  1, 2, 1 };
    double *m = gradient_map(g, gx, gy, 3);

    /* 2) espace de Hough (rho, theta) en coordonnees polaires */
    int ntheta = 180;
    double diag = sqrt((double)W * W + (double)H * H);
    int nrho = (int)(2 * diag) + 1;
    int *acc = (int *)calloc((size_t)ntheta * nrho, sizeof(int));
    double *cost = (double *)malloc(sizeof(double) * ntheta);
    double *sint = (double *)malloc(sizeof(double) * ntheta);
    for (int t = 0; t < ntheta; t++) {
        double a = t * M_PI / ntheta;
        cost[t] = cos(a); sint[t] = sin(a);
    }
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (m[y * W + x] < seuil) continue;
            for (int t = 0; t < ntheta; t++) {
                double rho = x * cost[t] + y * sint[t];
                int ri = (int)(rho + diag);
                if (ri >= 0 && ri < nrho) acc[t * nrho + ri]++;
            }
        }
    }

    if (accumulator) {
        Image *out = img_create(ntheta, nrho, 1, 255);
        int mx = 1;
        for (int i = 0; i < ntheta * nrho; i++) if (acc[i] > mx) mx = acc[i];
        for (int i = 0; i < ntheta * nrho; i++)
            out->data[i] = clamp255((double)acc[i] / mx * 255.0);
        free(acc); free(cost); free(sint); free(m); img_free(g);
        return out;
    }

    /* 3) trace les droites qui depassent le seuil de votes sur l'image gris */
    if (votes_min <= 0) votes_min = (int)(diag * 0.4);
    Image *out = img_copy(g);
    for (int t = 0; t < ntheta; t++) {
        for (int ri = 0; ri < nrho; ri++) {
            if (acc[t * nrho + ri] < votes_min) continue;
            double rho = ri - diag;
            double c = cost[t], s = sint[t];
            /* parcourt l'image et marque les pixels proches de la droite */
            if (fabs(s) > fabs(c)) {
                for (int x = 0; x < W; x++) {
                    int y = (int)((rho - x * c) / s + 0.5);
                    if (y >= 0 && y < H) img_set(out, x, y, 0, 255);
                }
            } else {
                for (int y = 0; y < H; y++) {
                    int x = (int)((rho - y * s) / c + 0.5);
                    if (x >= 0 && x < W) img_set(out, x, y, 0, 255);
                }
            }
        }
    }
    free(acc); free(cost); free(sint); free(m); img_free(g);
    return out;
}
