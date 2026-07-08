#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "image.h"

/*
 * Support 06 - Segmentation.
 * Seuillage (simple, multiple, Otsu, adaptatif), k-moyennes, croissance de
 * regions et division-fusion (split & merge).
 */

/* Seuillage global: pixel -> 255 si >= T, sinon 0 */
Image *seg_threshold(const Image *im, int T);

/* Multi-seuillage: n seuils tries -> n+1 classes reparties sur [0,255] */
Image *seg_threshold_multi(const Image *im, const int *seuils, int n);

/* Calcule le seuil optimal de Otsu (minimise la variance intra-classes) */
int    seg_otsu_value(const Image *im);
Image *seg_threshold_otsu(const Image *im);

/* Seuillage local adaptatif: chaque bloc a son propre seuil (Otsu local) */
Image *seg_threshold_adaptive(const Image *im, int block_size);

/* Segmentation par k-moyennes sur l'intensite (k classes) */
Image *seg_kmeans(const Image *im, int k);

/* Croissance de region a partir d'un germe (seedx, seedy) et d'une tolerance */
Image *seg_region_growing(const Image *im, int seedx, int seedy, int tolerance);

/* Division-fusion (quadtree) selon un seuil de variance d'homogeneite */
Image *seg_split_merge(const Image *im, double var_threshold);

#endif /* SEGMENTATION_H */
