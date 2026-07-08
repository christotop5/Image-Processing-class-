#include "io_pnm.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Lit le prochain entier de l'entete PNM en ignorant espaces et commentaires */
static int read_header_int(FILE *f, int *value) {
    int c, n = 0, got = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return 0;
        if (c == '#') {                 /* commentaire jusqu'a fin de ligne */
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (isspace(c)) {
            if (got) break;
            continue;
        }
        if (!isdigit(c)) return 0;
        n = n * 10 + (c - '0');
        got = 1;
    }
    *value = n;
    return 1;
}

Image *pnm_read(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir '%s'\n", path);
        return NULL;
    }

    char magic[3] = {0};
    if (fscanf(f, "%2s", magic) != 1) {
        fclose(f);
        return NULL;
    }

    int channels, binary;
    if (magic[0] == 'P' && magic[1] == '2') { channels = 1; binary = 0; }
    else if (magic[0] == 'P' && magic[1] == '5') { channels = 1; binary = 1; }
    else if (magic[0] == 'P' && magic[1] == '3') { channels = 3; binary = 0; }
    else if (magic[0] == 'P' && magic[1] == '6') { channels = 3; binary = 1; }
    else {
        fprintf(stderr, "Erreur: format PNM non reconnu (%s)\n", magic);
        fclose(f);
        return NULL;
    }

    int w, h, maxv;
    if (!read_header_int(f, &w) || !read_header_int(f, &h) ||
        !read_header_int(f, &maxv)) {
        fprintf(stderr, "Erreur: entete PNM invalide\n");
        fclose(f);
        return NULL;
    }

    Image *im = img_create(w, h, channels, maxv);
    if (!im) { fclose(f); return NULL; }

    size_t count = (size_t)w * h * channels;
    if (binary) {
        /* read_header_int a deja consomme le separateur apres maxval */
        if (fread(im->data, 1, count, f) != count) {
            fprintf(stderr, "Erreur: donnees pixels incompletes\n");
            img_free(im);
            fclose(f);
            return NULL;
        }
    } else {
        for (size_t i = 0; i < count; i++) {
            int v;
            if (!read_header_int(f, &v)) {
                fprintf(stderr, "Erreur: donnees pixels incompletes\n");
                img_free(im);
                fclose(f);
                return NULL;
            }
            im->data[i] = (unsigned char)v;
        }
    }

    fclose(f);
    return im;
}

int pnm_write(const char *path, const Image *img) {
    if (!img) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ecrire '%s'\n", path);
        return -1;
    }
    const char *magic = (img->channels == 1) ? "P5" : "P6";
    fprintf(f, "%s\n%d %d\n%d\n", magic, img->width, img->height,
            img->max_value > 0 ? img->max_value : 255);
    size_t count = (size_t)img->width * img->height * img->channels;
    fwrite(img->data, 1, count, f);
    fclose(f);
    return 0;
}
