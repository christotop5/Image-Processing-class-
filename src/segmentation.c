#include "segmentation.h"
#include "histogram.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

Image *seg_threshold(const Image *im, int T) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    size_t n = (size_t)g->width * g->height;
    for (size_t i = 0; i < n; i++)
        g->data[i] = (g->data[i] >= T) ? 255 : 0;
    return g;
}

Image *seg_threshold_multi(const Image *im, const int *seuils, int n) {
    Image *g = img_to_gray(im);
    if (!g || n < 1) return g;
    size_t total = (size_t)g->width * g->height;
    for (size_t i = 0; i < total; i++) {
        int v = g->data[i];
        int cls = 0;
        while (cls < n && v >= seuils[cls]) cls++;
        g->data[i] = clamp255((double)cls * 255.0 / n);
    }
    return g;
}

int seg_otsu_value(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return 128;
    int hist[256];
    histogram_gray(g, hist);
    long total = (long)g->width * g->height;
    img_free(g);

    double sum = 0.0;
    for (int i = 0; i < 256; i++) sum += (double)i * hist[i];

    double sumB = 0.0;
    long wB = 0;
    double maxVar = -1.0;
    int threshold = 128;
    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;
        long wF = total - wB;
        if (wF == 0) break;
        sumB += (double)t * hist[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        /* variance inter-classes (maximiser <=> minimiser intra-classes) */
        double varBetween = (double)wB * wF * (mB - mF) * (mB - mF);
        if (varBetween > maxVar) {
            maxVar = varBetween;
            threshold = t;
        }
    }
    return threshold;
}

Image *seg_threshold_otsu(const Image *im) {
    /* Otsu renvoie la borne superieure de la classe "fond" : les pixels
     * strictement superieurs a ce seuil sont l'objet. */
    int t = seg_otsu_value(im);
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    size_t n = (size_t)g->width * g->height;
    for (size_t i = 0; i < n; i++)
        g->data[i] = (g->data[i] > t) ? 255 : 0;
    return g;
}

Image *seg_threshold_adaptive(const Image *im, int block_size) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    if (block_size < 8) block_size = 32;
    Image *out = img_create(g->width, g->height, 1, 255);

    for (int by = 0; by < g->height; by += block_size) {
        for (int bx = 0; bx < g->width; bx += block_size) {
            int ex = (bx + block_size < g->width) ? bx + block_size : g->width;
            int ey = (by + block_size < g->height) ? by + block_size : g->height;
            /* moyenne et variance locales */
            double sum = 0.0, sum2 = 0.0;
            int cnt = 0;
            for (int y = by; y < ey; y++)
                for (int x = bx; x < ex; x++) {
                    double v = img_get(g, x, y, 0);
                    sum += v; sum2 += v * v; cnt++;
                }
            double mean = sum / cnt;
            double var = sum2 / cnt - mean * mean;
            for (int y = by; y < ey; y++)
                for (int x = bx; x < ex; x++) {
                    int v = img_get(g, x, y, 0);
                    /* bloc homogene (variance faible): tout au fond */
                    int val = (var < 100.0) ? (mean >= 128 ? 255 : 0)
                                            : (v >= mean ? 255 : 0);
                    img_set(out, x, y, 0, (unsigned char)val);
                }
        }
    }
    img_free(g);
    return out;
}

Image *seg_kmeans(const Image *im, int k) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    if (k < 2) k = 2;
    if (k > 16) k = 16;
    size_t n = (size_t)g->width * g->height;

    double centers[16];
    for (int i = 0; i < k; i++) centers[i] = 255.0 * i / (k - 1);

    unsigned char *label = (unsigned char *)malloc(n);
    for (int iter = 0; iter < 20; iter++) {
        double sum[16] = {0};
        long cnt[16] = {0};
        for (size_t i = 0; i < n; i++) {
            double v = g->data[i];
            int best = 0;
            double bd = fabs(v - centers[0]);
            for (int c = 1; c < k; c++) {
                double d = fabs(v - centers[c]);
                if (d < bd) { bd = d; best = c; }
            }
            label[i] = (unsigned char)best;
            sum[best] += v; cnt[best]++;
        }
        double shift = 0.0;
        for (int c = 0; c < k; c++) {
            if (cnt[c] > 0) {
                double nc = sum[c] / cnt[c];
                shift += fabs(nc - centers[c]);
                centers[c] = nc;
            }
        }
        if (shift < 0.5) break;
    }
    for (size_t i = 0; i < n; i++)
        g->data[i] = clamp255(centers[label[i]]);
    free(label);
    return g;
}

Image *seg_region_growing(const Image *im, int seedx, int seedy, int tolerance) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    int W = g->width, H = g->height;
    if (seedx < 0 || seedx >= W || seedy < 0 || seedy >= H) {
        seedx = W / 2; seedy = H / 2;
    }
    if (tolerance <= 0) tolerance = 15;

    Image *out = img_create(W, H, 1, 255); /* fond noir */
    unsigned char *visited = (unsigned char *)calloc((size_t)W * H, 1);

    int *stack = (int *)malloc(sizeof(int) * W * H);
    int top = 0;
    double seedVal = img_get(g, seedx, seedy, 0);
    stack[top++] = seedy * W + seedx;
    visited[seedy * W + seedx] = 1;

    int dx4[4] = {1, -1, 0, 0};
    int dy4[4] = {0, 0, 1, -1};
    while (top > 0) {
        int p = stack[--top];
        int x = p % W, y = p / W;
        out->data[p] = 255;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx4[d], ny = y + dy4[d];
            if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
            int np = ny * W + nx;
            if (visited[np]) continue;
            if (fabs(img_get(g, nx, ny, 0) - seedVal) <= tolerance) {
                visited[np] = 1;
                stack[top++] = np;
            }
        }
    }
    free(stack); free(visited); img_free(g);
    return out;
}

/* --- Division-fusion (quadtree) --- */
static void split_region(const Image *g, Image *out, int x, int y, int w, int h,
                         double var_threshold) {
    double sum = 0.0, sum2 = 0.0;
    int cnt = 0;
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) {
            double v = img_get(g, i, j, 0);
            sum += v; sum2 += v * v; cnt++;
        }
    double mean = sum / cnt;
    double var = sum2 / cnt - mean * mean;

    /* homogene ou trop petit -> region uniforme = moyenne */
    if (var <= var_threshold || w <= 2 || h <= 2) {
        for (int j = y; j < y + h; j++)
            for (int i = x; i < x + w; i++)
                img_set(out, i, j, 0, clamp255(mean));
        return;
    }
    int w1 = w / 2, h1 = h / 2;
    int w2 = w - w1, h2 = h - h1;
    split_region(g, out, x,      y,      w1, h1, var_threshold);
    split_region(g, out, x + w1, y,      w2, h1, var_threshold);
    split_region(g, out, x,      y + h1, w1, h2, var_threshold);
    split_region(g, out, x + w1, y + h1, w2, h2, var_threshold);
}

Image *seg_split_merge(const Image *im, double var_threshold) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    if (var_threshold <= 0) var_threshold = 100.0;
    Image *out = img_create(g->width, g->height, 1, 255);
    split_region(g, out, 0, 0, g->width, g->height, var_threshold);
    img_free(g);
    return out;
}
