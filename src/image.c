#include "image.h"
#include <stdlib.h>
#include <string.h>

Image *img_create(int width, int height, int channels, int max_value) {
    if (width <= 0 || height <= 0 || (channels != 1 && channels != 3))
        return NULL;
    Image *im = (Image *)malloc(sizeof(Image));
    if (!im) return NULL;
    im->width = width;
    im->height = height;
    im->channels = channels;
    im->max_value = (max_value > 0) ? max_value : 255;
    im->data = (unsigned char *)calloc((size_t)width * height * channels, 1);
    if (!im->data) {
        free(im);
        return NULL;
    }
    return im;
}

void img_free(Image *img) {
    if (!img) return;
    free(img->data);
    free(img);
}

Image *img_copy(const Image *src) {
    if (!src) return NULL;
    Image *dst = img_create(src->width, src->height, src->channels, src->max_value);
    if (!dst) return NULL;
    memcpy(dst->data, src->data,
           (size_t)src->width * src->height * src->channels);
    return dst;
}

Image *img_to_gray(const Image *src) {
    if (!src) return NULL;
    if (src->channels == 1) return img_copy(src);

    Image *dst = img_create(src->width, src->height, 1, src->max_value);
    if (!dst) return NULL;
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            double r = img_get(src, x, y, 0);
            double g = img_get(src, x, y, 1);
            double b = img_get(src, x, y, 2);
            /* Luminance perceptuelle (norme ITU-R BT.601) */
            double gray = 0.299 * r + 0.587 * g + 0.114 * b;
            img_set(dst, x, y, 0, clamp255(gray));
        }
    }
    return dst;
}

int img_is_gray(const Image *im) {
    return im && im->channels == 1;
}

unsigned char clamp255(double v) {
    if (v < 0.0) return 0;
    if (v > 255.0) return 255;
    return (unsigned char)(v + 0.5);
}
