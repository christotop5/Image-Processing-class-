#include "morphology.h"
#include "segmentation.h"
#include <stdlib.h>
#include <string.h>

/* Assure une image binaire (objet=255, fond=0). Si l'image n'est pas deja
 * binaire, on applique le seuil de Otsu. */
static Image *ensure_binary(const Image *im) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    size_t n = (size_t)g->width * g->height;
    int only_bin = 1;
    for (size_t i = 0; i < n; i++)
        if (g->data[i] != 0 && g->data[i] != 255) { only_bin = 0; break; }
    if (only_bin) return g;
    img_free(g);
    return seg_threshold_otsu(im);
}

Image *morph_binarize(const Image *im, int seuil) {
    return seg_threshold(im, seuil);
}

/* erode=1 pour l'erosion, erode=0 pour la dilatation */
static Image *morph_op(const Image *bin, int conn, int erode) {
    int W = bin->width, H = bin->height;
    Image *out = img_create(W, H, 1, 255);
    int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy8[8] = {-1,-1,-1,  0, 0,  1, 1, 1};
    int dx4[4] = {0, 0, -1, 1};
    int dy4[4] = {-1, 1, 0, 0};
    int *dx = (conn == 4) ? dx4 : dx8;
    int *dy = (conn == 4) ? dy4 : dy8;
    int nd = (conn == 4) ? 4 : 8;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int center = img_get(bin, x, y, 0);
            int val = center;
            if (erode) {
                /* si un voisin (ou hors image) est fond -> pixel devient fond */
                if (center > 0) {
                    for (int d = 0; d < nd; d++) {
                        int nx = x + dx[d], ny = y + dy[d];
                        if (nx < 0 || ny < 0 || nx >= W || ny >= H ||
                            img_get(bin, nx, ny, 0) == 0) { val = 0; break; }
                    }
                }
            } else {
                /* si un voisin est objet -> pixel devient objet */
                if (center == 0) {
                    for (int d = 0; d < nd; d++) {
                        int nx = x + dx[d], ny = y + dy[d];
                        if (nx >= 0 && ny >= 0 && nx < W && ny < H &&
                            img_get(bin, nx, ny, 0) > 0) { val = 255; break; }
                    }
                }
            }
            img_set(out, x, y, 0, (unsigned char)val);
        }
    }
    return out;
}

Image *morph_erode(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *out = morph_op(b, conn, 1);
    img_free(b);
    return out;
}

Image *morph_dilate(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *out = morph_op(b, conn, 0);
    img_free(b);
    return out;
}

Image *morph_open(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *e = morph_op(b, conn, 1);
    Image *out = morph_op(e, conn, 0);
    img_free(b); img_free(e);
    return out;
}

Image *morph_close(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *d = morph_op(b, conn, 0);
    Image *out = morph_op(d, conn, 1);
    img_free(b); img_free(d);
    return out;
}

Image *morph_gradient(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *e = morph_op(b, conn, 1);
    Image *d = morph_op(b, conn, 0);
    Image *out = img_create(b->width, b->height, 1, 255);
    size_t n = (size_t)b->width * b->height;
    for (size_t i = 0; i < n; i++)
        out->data[i] = (unsigned char)(d->data[i] - e->data[i]);
    img_free(b); img_free(e); img_free(d);
    return out;
}

Image *morph_gradient_internal(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *e = morph_op(b, conn, 1);
    Image *out = img_create(b->width, b->height, 1, 255);
    size_t n = (size_t)b->width * b->height;
    for (size_t i = 0; i < n; i++)
        out->data[i] = (unsigned char)(b->data[i] - e->data[i]);
    img_free(b); img_free(e);
    return out;
}

Image *morph_gradient_external(const Image *im, int conn) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    Image *d = morph_op(b, conn, 0);
    Image *out = img_create(b->width, b->height, 1, 255);
    size_t n = (size_t)b->width * b->height;
    for (size_t i = 0; i < n; i++)
        out->data[i] = (unsigned char)(d->data[i] - b->data[i]);
    img_free(b); img_free(d);
    return out;
}

Image *morph_label(const Image *im, int conn, int *nb_components) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    int W = b->width, H = b->height;
    int *labels = (int *)calloc((size_t)W * H, sizeof(int));
    int *stack = (int *)malloc(sizeof(int) * W * H);

    int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy8[8] = {-1,-1,-1,  0, 0,  1, 1, 1};
    int dx4[4] = {0, 0, -1, 1};
    int dy4[4] = {-1, 1, 0, 0};
    int *dx = (conn == 4) ? dx4 : dx8;
    int *dy = (conn == 4) ? dy4 : dy8;
    int nd = (conn == 4) ? 4 : 8;

    int current = 0;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int p = y * W + x;
            if (img_get(b, x, y, 0) == 0 || labels[p] != 0) continue;
            current++;
            int top = 0;
            stack[top++] = p;
            labels[p] = current;
            while (top > 0) {
                int q = stack[--top];
                int qx = q % W, qy = q / W;
                for (int d = 0; d < nd; d++) {
                    int nx = qx + dx[d], ny = qy + dy[d];
                    if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                    int np = ny * W + nx;
                    if (img_get(b, nx, ny, 0) > 0 && labels[np] == 0) {
                        labels[np] = current;
                        stack[top++] = np;
                    }
                }
            }
        }
    }
    if (nb_components) *nb_components = current;

    /* Colorisation des etiquettes en image couleur (PPM) */
    Image *out = img_create(W, H, 3, 255);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int L = labels[y * W + x];
            unsigned char r = 0, g = 0, bl = 0;
            if (L > 0) {
                /* couleurs pseudo-aleatoires stables par etiquette */
                r = (unsigned char)((L * 97 + 30) % 256);
                g = (unsigned char)((L * 57 + 80) % 256);
                bl = (unsigned char)((L * 131 + 120) % 256);
            }
            img_set(out, x, y, 0, r);
            img_set(out, x, y, 1, g);
            img_set(out, x, y, 2, bl);
        }
    }
    free(labels); free(stack); img_free(b);
    return out;
}

Image *morph_distance(const Image *im, int metric) {
    Image *b = ensure_binary(im);
    if (!b) return NULL;
    int W = b->width, H = b->height;
    const int INF = 1 << 20;
    int *dist = (int *)malloc(sizeof(int) * W * H);
    for (int i = 0; i < W * H; i++)
        dist[i] = (b->data[i] > 0) ? INF : 0;

    int diag = (metric == 8) ? 1 : 2; /* cout diagonale: D8=1, D4=2 (approx) */

    /* passe avant */
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int p = y * W + x;
            if (dist[p] == 0) continue;
            int best = dist[p];
            if (x > 0)               best = (dist[p - 1] + 1 < best) ? dist[p - 1] + 1 : best;
            if (y > 0)               best = (dist[p - W] + 1 < best) ? dist[p - W] + 1 : best;
            if (metric == 8) {
                if (x > 0 && y > 0)      best = (dist[p - W - 1] + diag < best) ? dist[p - W - 1] + diag : best;
                if (x < W - 1 && y > 0)  best = (dist[p - W + 1] + diag < best) ? dist[p - W + 1] + diag : best;
            }
            dist[p] = best;
        }
    /* passe arriere */
    for (int y = H - 1; y >= 0; y--)
        for (int x = W - 1; x >= 0; x--) {
            int p = y * W + x;
            if (dist[p] == 0) continue;
            int best = dist[p];
            if (x < W - 1)           best = (dist[p + 1] + 1 < best) ? dist[p + 1] + 1 : best;
            if (y < H - 1)           best = (dist[p + W] + 1 < best) ? dist[p + W] + 1 : best;
            if (metric == 8) {
                if (x < W - 1 && y < H - 1) best = (dist[p + W + 1] + diag < best) ? dist[p + W + 1] + diag : best;
                if (x > 0 && y < H - 1)     best = (dist[p + W - 1] + diag < best) ? dist[p + W - 1] + diag : best;
            }
            dist[p] = best;
        }

    int mx = 1;
    for (int i = 0; i < W * H; i++)
        if (dist[i] < INF && dist[i] > mx) mx = dist[i];
    Image *out = img_create(W, H, 1, 255);
    for (int i = 0; i < W * H; i++) {
        int d = (dist[i] >= INF) ? mx : dist[i];
        out->data[i] = clamp255((double)d / mx * 255.0);
    }
    free(dist); img_free(b);
    return out;
}

Image *morph_hysteresis(const Image *im, int low, int high) {
    Image *g = img_to_gray(im);
    if (!g) return NULL;
    if (low >= high) { low = 40; high = 90; }
    int W = g->width, H = g->height;
    Image *out = img_create(W, H, 1, 255);
    int *stack = (int *)malloc(sizeof(int) * W * H);
    int top = 0;

    /* pixels forts (>= high) */
    for (int i = 0; i < W * H; i++)
        if (g->data[i] >= high) { out->data[i] = 255; stack[top++] = i; }

    int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy8[8] = {-1,-1,-1,  0, 0,  1, 1, 1};
    /* propagation vers les pixels faibles connectes (>= low) */
    while (top > 0) {
        int p = stack[--top];
        int x = p % W, y = p / W;
        for (int d = 0; d < 8; d++) {
            int nx = x + dx8[d], ny = y + dy8[d];
            if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
            int np = ny * W + nx;
            if (out->data[np] == 0 && g->data[np] >= low) {
                out->data[np] = 255;
                stack[top++] = np;
            }
        }
    }
    free(stack); img_free(g);
    return out;
}
