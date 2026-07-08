#include "histogram.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

void histogram_gray(const Image *im, int hist[256]) {
    memset(hist, 0, 256 * sizeof(int));
    size_t n = (size_t)im->width * im->height;
    for (size_t i = 0; i < n; i++)
        hist[im->data[i]]++;
}

double img_mean(const Image *im) {
    size_t n = (size_t)im->width * im->height * im->channels;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += im->data[i];
    return sum / (double)n;
}

double img_stddev(const Image *im) {
    size_t n = (size_t)im->width * im->height * im->channels;
    double m = img_mean(im), s = 0.0;
    for (size_t i = 0; i < n; i++) {
        double d = im->data[i] - m;
        s += d * d;
    }
    return sqrt(s / (double)n);
}

void img_minmax(const Image *im, int *vmin, int *vmax) {
    size_t n = (size_t)im->width * im->height * im->channels;
    int mn = 255, mx = 0;
    for (size_t i = 0; i < n; i++) {
        if (im->data[i] < mn) mn = im->data[i];
        if (im->data[i] > mx) mx = im->data[i];
    }
    *vmin = mn;
    *vmax = mx;
}

Image *histogram_equalize(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    int hist[256];
    histogram_gray(g, hist);

    size_t nbp = (size_t)g->width * g->height;
    /* Fonction de repartition cumulee normalisee -> LUT */
    double cdf = 0.0;
    unsigned char lut[256];
    for (int i = 0; i < 256; i++) {
        cdf += (double)hist[i] / (double)nbp;
        lut[i] = clamp255(cdf * 255.0);
    }
    for (size_t i = 0; i < nbp; i++)
        g->data[i] = lut[g->data[i]];
    return g;
}

Image *histogram_stretch(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    int mn, mx;
    img_minmax(g, &mn, &mx);
    if (mx == mn) return g; /* image constante */
    unsigned char lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = clamp255(255.0 * (i - mn) / (double)(mx - mn));
    size_t n = (size_t)g->width * g->height;
    for (size_t i = 0; i < n; i++)
        g->data[i] = lut[g->data[i]];
    return g;
}

Image *histogram_equalize_local(const Image *im, int win) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    if (win < 3) win = 7;
    int r = win / 2;
    Image *out = img_create(g->width, g->height, 1, 255);
    if (!out) { img_free(g); return NULL; }

    for (int y = 0; y < g->height; y++) {
        for (int x = 0; x < g->width; x++) {
            int hist[256];
            memset(hist, 0, sizeof(hist));
            int count = 0;
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= g->width || ny >= g->height)
                        continue;
                    hist[img_get(g, nx, ny, 0)]++;
                    count++;
                }
            }
            int v = img_get(g, x, y, 0);
            int cum = 0;
            for (int i = 0; i <= v; i++) cum += hist[i];
            img_set(out, x, y, 0, clamp255(255.0 * cum / (double)count));
        }
    }
    img_free(g);
    return out;
}

Image *histogram_to_image(const Image *im, int height) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    if (height < 32) height = 200;
    int hist[256];
    histogram_gray(g, hist);
    img_free(g);

    int hmax = 1;
    for (int i = 0; i < 256; i++)
        if (hist[i] > hmax) hmax = hist[i];

    Image *out = img_create(256, height, 1, 255);
    if (!out) return NULL;
    memset(out->data, 255, (size_t)256 * height); /* fond blanc */

    for (int x = 0; x < 256; x++) {
        int barh = (int)((double)hist[x] / hmax * (height - 1));
        for (int y = 0; y < barh; y++)
            img_set(out, x, height - 1 - y, 0, 0); /* barre noire */
    }
    return out;
}
