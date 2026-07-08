/*
 * Generateur d'images de test (PGM/PPM) pour le TP de traitement d'images.
 * Cree plusieurs images synthetiques dans le dossier images/ afin de pouvoir
 * tester tous les traitements sans dependre d'images externes.
 *
 * Compilation via le Makefile: `make samples`
 */
#include "image.h"
#include "io_pnm.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Petit generateur pseudo-aleatoire deterministe */
static unsigned int rng_state = 12345u;
static int rnd(int mod) {
    rng_state = rng_state * 1103515245u + 12345u;
    return (int)((rng_state >> 16) % (unsigned)mod);
}

/* Degrade horizontal (utile pour: etirement, egalisation, seuillage) */
static Image *make_gradient(int W, int H) {
    Image *im = img_create(W, H, 1, 255);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            img_set(im, x, y, 0, (unsigned char)(x * 255 / (W - 1)));
    return im;
}

/* Image a faible contraste (pour tester l'amelioration de contraste) */
static Image *make_lowcontrast(int W, int H) {
    Image *im = img_create(W, H, 1, 255);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            double d = sin(x * 0.05) * cos(y * 0.05);
            img_set(im, x, y, 0, (unsigned char)(110 + 30 * d));
        }
    return im;
}

/* Formes geometriques: disque + rectangle (contours, morphologie, segmentation) */
static Image *make_shapes(int W, int H) {
    Image *im = img_create(W, H, 1, 255);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            img_set(im, x, y, 0, 30); /* fond gris fonce */
    /* disque clair */
    int cx = W / 3, cy = H / 2, r = H / 4;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r * r) img_set(im, x, y, 0, 220);
        }
    /* rectangle clair (bien separe du disque pour l'etiquetage) */
    for (int y = H / 4; y < 3 * H / 4; y++)
        for (int x = 2 * W / 3 + 5; x < 2 * W / 3 + 55; x++)
            if (x >= 0 && x < W) img_set(im, x, y, 0, 200);
    return im;
}

/* Damier (contours nets, Fourier, convolution) */
static Image *make_checker(int W, int H, int square) {
    Image *im = img_create(W, H, 1, 255);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int c = ((x / square) + (y / square)) % 2;
            img_set(im, x, y, 0, c ? 230 : 25);
        }
    return im;
}

/* Image bruitee "poivre et sel" (test filtre median, morphologie) */
static Image *make_saltpepper(int W, int H) {
    Image *im = make_shapes(W, H);
    int n = W * H / 12;
    for (int i = 0; i < n; i++) {
        int x = rnd(W), y = rnd(H);
        img_set(im, x, y, 0, (rnd(2) ? 255 : 0));
    }
    return im;
}

/* Image couleur RVB (test conversion en gris, operations couleur) */
static Image *make_color(int W, int H) {
    Image *im = img_create(W, H, 3, 255);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            img_set(im, x, y, 0, (unsigned char)(x * 255 / (W - 1)));       /* R */
            img_set(im, x, y, 1, (unsigned char)(y * 255 / (H - 1)));       /* G */
            img_set(im, x, y, 2, (unsigned char)(255 - x * 255 / (W - 1))); /* B */
        }
    return im;
}

int main(void) {
    int W = 256, H = 256;
    struct { const char *name; Image *(*fn)(void); } dummy; (void)dummy;

    Image *im;

    im = make_gradient(W, H);      pnm_write("images/gradient.pgm", im);    img_free(im);
    im = make_lowcontrast(W, H);   pnm_write("images/lowcontrast.pgm", im); img_free(im);
    im = make_shapes(W, H);        pnm_write("images/shapes.pgm", im);      img_free(im);
    im = make_checker(W, H, 32);   pnm_write("images/checker.pgm", im);     img_free(im);
    im = make_saltpepper(W, H);    pnm_write("images/noise.pgm", im);       img_free(im);
    im = make_color(W, H);         pnm_write("images/color.ppm", im);       img_free(im);

    printf("Images de test generees dans le dossier images/:\n");
    printf("  gradient.pgm, lowcontrast.pgm, shapes.pgm,\n");
    printf("  checker.pgm, noise.pgm, color.ppm\n");
    return 0;
}
