/*
 * Generateur de demonstration : applique TOUS les traitements des supports de
 * cours aux images de test et enregistre chaque resultat dans output/ (classe
 * par support). Permet de voir d'un coup l'effet de chaque traitement.
 *
 * Compilation/execution via le Makefile: `make demo`
 */
#include "image.h"
#include "io_pnm.h"
#include "histogram.h"
#include "point_ops.h"
#include "geometry.h"
#include "convolution.h"
#include "fourier.h"
#include "edges.h"
#include "segmentation.h"
#include "morphology.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Enregistre une image puis la libere. Affiche une ligne de suivi. */
static void save(Image *im, const char *path) {
    if (!im) { printf("  [ECHEC] %s\n", path); return; }
    pnm_write(path, im);
    printf("  -> %s (%dx%d, %s)\n", path, im->width, im->height,
           im->channels == 1 ? "gris" : "couleur");
    img_free(im);
}

/* Charge une image de test, en signalant une erreur eventuelle */
static Image *load(const char *name) {
    Image *im = pnm_read(name);
    if (!im) printf("  ** impossible de charger %s (lancez 'make samples')\n", name);
    return im;
}

int main(void) {
    mkdir("output", 0755);
    mkdir("output/02_base", 0755);
    mkdir("output/03_convolution", 0755);
    mkdir("output/04_fourier", 0755);
    mkdir("output/05_contours", 0755);
    mkdir("output/06_segmentation", 0755);
    mkdir("output/07_morphologie", 0755);

    Image *gradient    = load("images/gradient.pgm");
    Image *lowc        = load("images/lowcontrast.pgm");
    Image *shapes      = load("images/shapes.pgm");
    Image *checker     = load("images/checker.pgm");
    Image *noise       = load("images/noise.pgm");
    Image *color       = load("images/color.ppm");

    /* ---------- Support 01 : conversion couleur -> gris ---------- */
    printf("\n[01] Introduction\n");
    if (color) save(img_to_gray(color), "output/01_couleur_vers_gris.pgm");

    /* ---------- Support 02 : traitements de base ---------- */
    printf("\n[02] Traitements de base\n");
    if (lowc) {
        save(histogram_to_image(lowc, 200), "output/02_base/histogramme.pgm");
        save(op_linear(lowc),               "output/02_base/lineaire.pgm");
        save(op_linear_saturation(lowc, 90, 140), "output/02_base/saturation.pgm");
        save(op_piecewise_linear(lowc, 90, 30, 140, 225), "output/02_base/par_morceaux.pgm");
        save(op_gamma(lowc, 0.5),           "output/02_base/gamma_0.5.pgm");
        save(op_gamma(lowc, 2.0),           "output/02_base/gamma_2.0.pgm");
        save(op_negative(lowc),             "output/02_base/negatif.pgm");
        save(histogram_equalize(lowc),      "output/02_base/egalisation.pgm");
        save(histogram_equalize_local(lowc, 15), "output/02_base/egalisation_locale.pgm");
        save(histogram_stretch(lowc),       "output/02_base/etirement.pgm");
    }
    if (shapes && gradient) {
        save(op_add(shapes, gradient), "output/02_base/addition.pgm");
        save(op_sub(shapes, gradient), "output/02_base/soustraction.pgm");
        save(op_mul_scalar(shapes, 1.5), "output/02_base/multiplication.pgm");
        save(op_blend(shapes, gradient, 0.5), "output/02_base/blend.pgm");
        save(op_and(shapes, checker), "output/02_base/et_logique.pgm");
        save(op_or(shapes, checker),  "output/02_base/ou_logique.pgm");
        save(op_xor(shapes, checker), "output/02_base/xor_logique.pgm");
    }
    if (shapes) {
        save(resize_nearest(shapes, 128, 128),  "output/02_base/resize_ppv.pgm");
        save(resize_bilinear(shapes, 512, 512), "output/02_base/resize_bilineaire.pgm");
        save(resize_bicubic(shapes, 512, 512),  "output/02_base/resize_bicubique.pgm");
    }

    /* ---------- Support 03 : convolution / filtrage ---------- */
    printf("\n[03] Convolution / filtrage\n");
    if (noise) {
        save(filter_mean(noise, 3),   "output/03_convolution/moyenneur_3x3.pgm");
        save(filter_mean(noise, 5),   "output/03_convolution/moyenneur_5x5.pgm");
        save(filter_gaussian(noise),  "output/03_convolution/gaussien_5x5.pgm");
        save(filter_median(noise, 3), "output/03_convolution/median_3x3.pgm");
        save(filter_min(noise, 3),    "output/03_convolution/min_3x3.pgm");
        save(filter_max(noise, 3),    "output/03_convolution/max_3x3.pgm");
    }

    /* ---------- Support 04 : Fourier ---------- */
    printf("\n[04] Transformee de Fourier\n");
    if (checker) {
        save(fourier_spectrum(checker),    "output/04_fourier/spectre.pgm");
        save(fourier_lowpass(checker, 40), "output/04_fourier/passe_bas.pgm");
        save(fourier_highpass(checker, 20),"output/04_fourier/passe_haut.pgm");
    }

    /* ---------- Support 05 : contours ---------- */
    printf("\n[05] Detection de contours\n");
    if (shapes) {
        save(edge_roberts(shapes),  "output/05_contours/roberts.pgm");
        save(edge_prewitt(shapes),  "output/05_contours/prewitt.pgm");
        save(edge_sobel(shapes),    "output/05_contours/sobel.pgm");
        save(edge_laplacian(shapes, 0), "output/05_contours/laplacien_4.pgm");
        save(edge_laplacian(shapes, 1), "output/05_contours/laplacien_8.pgm");
        save(edge_gradient_threshold(shapes, 60), "output/05_contours/gradient_seuil.pgm");
    }
    if (checker) {
        save(edge_hough_lines(checker, 80, 0, 1), "output/05_contours/hough_accumulateur.pgm");
        save(edge_hough_lines(checker, 80, 0, 0), "output/05_contours/hough_droites.pgm");
    }

    /* ---------- Support 06 : segmentation ---------- */
    printf("\n[06] Segmentation\n");
    if (shapes) {
        save(seg_threshold(shapes, 128),        "output/06_segmentation/seuil_128.pgm");
        int s2[2] = {85, 170};
        save(seg_threshold_multi(shapes, s2, 2),"output/06_segmentation/multiseuil.pgm");
        save(seg_threshold_otsu(shapes),        "output/06_segmentation/otsu.pgm");
        save(seg_region_growing(shapes, 85, 128, 20), "output/06_segmentation/croissance.pgm");
        save(seg_split_merge(shapes, 100),      "output/06_segmentation/split_merge.pgm");
    }
    if (lowc) {
        save(seg_threshold_adaptive(lowc, 32),  "output/06_segmentation/adaptatif.pgm");
        save(seg_kmeans(lowc, 4),               "output/06_segmentation/kmeans_4.pgm");
    }

    /* ---------- Support 07 : morphologie ---------- */
    printf("\n[07] Images binaires / morphologie\n");
    if (shapes) {
        save(morph_binarize(shapes, 100),      "output/07_morphologie/binaire.pgm");
        save(morph_erode(shapes, 8),           "output/07_morphologie/erosion.pgm");
        save(morph_dilate(shapes, 8),          "output/07_morphologie/dilatation.pgm");
        save(morph_open(noise ? noise : shapes, 8),  "output/07_morphologie/ouverture.pgm");
        save(morph_close(noise ? noise : shapes, 8), "output/07_morphologie/fermeture.pgm");
        save(morph_gradient(shapes, 8),          "output/07_morphologie/gradient.pgm");
        save(morph_gradient_internal(shapes, 8), "output/07_morphologie/gradient_interne.pgm");
        save(morph_gradient_external(shapes, 8), "output/07_morphologie/gradient_externe.pgm");
        int nb = 0;
        Image *lab = morph_label(shapes, 8, &nb);
        printf("  (etiquetage: %d composantes connexes)\n", nb);
        save(lab, "output/07_morphologie/etiquettes.ppm");
        save(morph_distance(shapes, 4), "output/07_morphologie/distance_D4.pgm");
        save(morph_distance(shapes, 8), "output/07_morphologie/distance_D8.pgm");
        /* hysteresis applique sur la carte de gradient (Sobel) */
        Image *grad = edge_sobel(shapes);
        if (grad) {
            save(morph_hysteresis(grad, 40, 90), "output/07_morphologie/hysteresis.pgm");
            img_free(grad);
        }
    }

    img_free(gradient); img_free(lowc); img_free(shapes);
    img_free(checker);  img_free(noise); img_free(color);

    printf("\nTermine. Tous les resultats sont dans le dossier output/.\n");
    return 0;
}
