#ifndef IMAGE_H
#define IMAGE_H

/*
 * Type de donnee central pour stocker une image.
 *
 * Choix du format:
 *  - Les pixels sont stockes dans un tableau contigu d'octets (unsigned char),
 *    ce qui correspond exactement aux formats non compresses PGM (gris 8 bits)
 *    et PPM (couleur 24 bits) recommandes dans le cours (support 01).
 *  - `channels` vaut 1 pour une image en niveaux de gris, 3 pour une image
 *    couleur RVB (Rouge, Vert, Bleu entrelaces: R,G,B,R,G,B,...).
 *  - `max_value` est la valeur maximale d'un pixel (255 pour du 8 bits).
 *
 * Les valeurs intermediaires pouvant sortir de [0,255] (gradient, laplacien,
 * Fourier...) sont calculees en `double` puis ramenees dans [0,255] via clamp().
 */
typedef struct {
    int width;             /* nombre de colonnes (N) */
    int height;            /* nombre de lignes   (M) */
    int channels;          /* 1 = gris, 3 = RVB */
    int max_value;         /* valeur max d'un pixel, en general 255 */
    unsigned char *data;   /* taille = width * height * channels */
} Image;

/* Allocation / liberation */
Image *img_create(int width, int height, int channels, int max_value);
void   img_free(Image *img);
Image *img_copy(const Image *src);

/* Acces aux pixels (avec verification de bornes cote appelant) */
static inline unsigned char img_get(const Image *im, int x, int y, int c) {
    return im->data[(y * im->width + x) * im->channels + c];
}
static inline void img_set(Image *im, int x, int y, int c, unsigned char v) {
    im->data[(y * im->width + x) * im->channels + c] = v;
}

/* Conversion couleur -> niveaux de gris (luminance ITU-R 601) */
Image *img_to_gray(const Image *src);

/* Vrai (1) si l'image est en niveaux de gris */
int img_is_gray(const Image *im);

/* Ramene une valeur reelle dans l'intervalle [0,255] */
unsigned char clamp255(double v);

#endif /* IMAGE_H */
