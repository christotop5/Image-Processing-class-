#include "geometry.h"
#include <stdlib.h>
#include <math.h>

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

Image *resize_nearest(const Image *im, int new_w, int new_h) {
    if (new_w <= 0 || new_h <= 0) return NULL;
    Image *out = img_create(new_w, new_h, im->channels, im->max_value);
    if (!out) return NULL;
    double sx = (double)im->width / new_w;
    double sy = (double)im->height / new_h;
    for (int y = 0; y < new_h; y++) {
        int oy = clampi((int)(y * sy), 0, im->height - 1);
        for (int x = 0; x < new_w; x++) {
            int ox = clampi((int)(x * sx), 0, im->width - 1);
            for (int c = 0; c < im->channels; c++)
                img_set(out, x, y, c, img_get(im, ox, oy, c));
        }
    }
    return out;
}

Image *resize_bilinear(const Image *im, int new_w, int new_h) {
    if (new_w <= 0 || new_h <= 0) return NULL;
    Image *out = img_create(new_w, new_h, im->channels, im->max_value);
    if (!out) return NULL;
    double sx = (double)im->width / new_w;
    double sy = (double)im->height / new_h;
    for (int y = 0; y < new_h; y++) {
        double fy = (y + 0.5) * sy - 0.5;
        int y0 = (int)floor(fy);
        double dy = fy - y0;
        for (int x = 0; x < new_w; x++) {
            double fx = (x + 0.5) * sx - 0.5;
            int x0 = (int)floor(fx);
            double dx = fx - x0;
            int x1 = clampi(x0 + 1, 0, im->width - 1);
            int y1 = clampi(y0 + 1, 0, im->height - 1);
            int xx0 = clampi(x0, 0, im->width - 1);
            int yy0 = clampi(y0, 0, im->height - 1);
            for (int c = 0; c < im->channels; c++) {
                double v00 = img_get(im, xx0, yy0, c);
                double v10 = img_get(im, x1, yy0, c);
                double v01 = img_get(im, xx0, y1, c);
                double v11 = img_get(im, x1, y1, c);
                double top = v00 + dx * (v10 - v00);
                double bot = v01 + dx * (v11 - v01);
                img_set(out, x, y, c, clamp255(top + dy * (bot - top)));
            }
        }
    }
    return out;
}

/* Noyau cubique de Catmull-Rom */
static double cubic(double t) {
    double a = -0.5;
    t = fabs(t);
    if (t <= 1.0) return (a + 2.0) * t * t * t - (a + 3.0) * t * t + 1.0;
    if (t < 2.0) return a * t * t * t - 5.0 * a * t * t + 8.0 * a * t - 4.0 * a;
    return 0.0;
}

Image *resize_bicubic(const Image *im, int new_w, int new_h) {
    if (new_w <= 0 || new_h <= 0) return NULL;
    Image *out = img_create(new_w, new_h, im->channels, im->max_value);
    if (!out) return NULL;
    double sx = (double)im->width / new_w;
    double sy = (double)im->height / new_h;
    for (int y = 0; y < new_h; y++) {
        double fy = (y + 0.5) * sy - 0.5;
        int yi = (int)floor(fy);
        double dy = fy - yi;
        for (int x = 0; x < new_w; x++) {
            double fx = (x + 0.5) * sx - 0.5;
            int xi = (int)floor(fx);
            double dx = fx - xi;
            for (int c = 0; c < im->channels; c++) {
                double acc = 0.0;
                for (int m = -1; m <= 2; m++) {
                    for (int n = -1; n <= 2; n++) {
                        int px = clampi(xi + n, 0, im->width - 1);
                        int py = clampi(yi + m, 0, im->height - 1);
                        double w = cubic(n - dx) * cubic(m - dy);
                        acc += w * img_get(im, px, py, c);
                    }
                }
                img_set(out, x, y, c, clamp255(acc));
            }
        }
    }
    return out;
}
