/*
 * TP Traitement d'images - Programme principal interactif.
 *
 * Menu dynamique regroupant tous les traitements des supports de cours
 * (Dr Tapamo, INF372). On charge une image PGM/PPM, on lui applique un
 * traitement, le resultat devient l'image courante et est enregistre dans
 * output/ pour pouvoir le visualiser.
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
#include <string.h>
#include <dirent.h>

static Image *g_current = NULL;   /* image de travail courante */
static char   g_name[256] = "";   /* nom court de l'image courante */

/* ------------------------------------------------------------------ */
/* Utilitaires d'entree                                               */
/* ------------------------------------------------------------------ */
static void flush_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

/* Efface l'ecran (nouvelle "page" du menu) */
static void clear_screen(void) {
    if (system("clear 2>/dev/null") != 0) {
        /* repli si 'clear' indisponible : sequence ANSI */
        printf("\033[H\033[2J");
        fflush(stdout);
    }
}

/* Met en pause pour laisser le temps de lire le resultat avant d'effacer */
static void pause_enter(void) {
    printf("\n  Appuyez sur Entree pour continuer...");
    fflush(stdout);
    char buf[64];
    if (!fgets(buf, sizeof(buf), stdin)) {}
}

static int read_int(const char *prompt, int def) {
    printf("%s [%d]: ", prompt, def);
    fflush(stdout);
    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) return def;
    if (buf[0] == '\n') return def;
    return atoi(buf);
}

static double read_double(const char *prompt, double def) {
    printf("%s [%.2f]: ", prompt, def);
    fflush(stdout);
    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) return def;
    if (buf[0] == '\n') return def;
    return atof(buf);
}

static void read_string(const char *prompt, char *out, int n) {
    printf("%s: ", prompt);
    fflush(stdout);
    if (!fgets(out, n, stdin)) { out[0] = '\0'; return; }
    size_t len = strlen(out);
    if (len > 0 && out[len - 1] == '\n') out[len - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* Gestion des fichiers et du resultat                                */
/* ------------------------------------------------------------------ */
static void list_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) { printf("  (dossier '%s' introuvable)\n", path); return; }
    struct dirent *e;
    int count = 0;
    while ((e = readdir(d)) != NULL) {
        const char *n = e->d_name;
        size_t l = strlen(n);
        if (l > 4 && (strcmp(n + l - 4, ".pgm") == 0 ||
                      strcmp(n + l - 4, ".ppm") == 0 ||
                      strcmp(n + l - 4, ".pnm") == 0)) {
            printf("   - %s\n", n);
            count++;
        }
    }
    if (count == 0) printf("   (aucune image, utilisez l'option 'Generer les images de test')\n");
    closedir(d);
}

static void print_info(const Image *im, const char *title) {
    if (!im) { printf("  (pas d'image)\n"); return; }
    int mn, mx;
    Image *g = img_to_gray(im);
    img_minmax(g, &mn, &mx);
    printf("  %s : %dx%d, %s, moyenne=%.1f, ecart-type=%.1f, min=%d, max=%d\n",
           title, im->width, im->height,
           im->channels == 1 ? "gris" : "couleur",
           img_mean(g), img_stddev(g), mn, mx);
    img_free(g);
}

/* Enregistre le resultat, le rend courant, propose de le visualiser */
static void set_result(Image *res, const char *opname) {
    if (!res) { printf("  ** Echec du traitement (image invalide ?)\n"); return; }
    const char *ext = (res->channels == 1) ? "pgm" : "ppm";
    char path[512];
    snprintf(path, sizeof(path), "output/%s.%s", opname, ext);
    pnm_write(path, res);

    img_free(g_current);
    g_current = res;
    snprintf(g_name, sizeof(g_name), "%s", opname);

    printf("  -> Resultat enregistre dans %s\n", path);
    print_info(res, "Resultat");

    printf("  Ouvrir le resultat dans la visionneuse ? (o/N): ");
    fflush(stdout);
    char buf[16];
    if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'o' || buf[0] == 'O')) {
        char cmd[600];
        snprintf(cmd, sizeof(cmd), "xdg-open '%s' >/dev/null 2>&1 &", path);
        int rc = system(cmd);
        (void)rc;
    }
}

/* Charge une deuxieme image (pour les operations binaires) */
static Image *load_second(void) {
    printf("\n  Images disponibles dans images/:\n");
    list_dir("images");
    char name[256];
    read_string("  Nom du fichier (dans images/)", name, sizeof(name));
    char path[512];
    snprintf(path, sizeof(path), "images/%s", name);
    Image *im = pnm_read(path);
    if (!im) printf("  ** Impossible de charger '%s'\n", path);
    return im;
}

/* Liste les images disponibles et charge celle choisie comme image courante.
 * Renvoie 1 si une image a bien ete chargee, 0 sinon. */
static int do_load_image(void) {
    printf("\n  Images disponibles dans images/:\n");
    list_dir("images");
    char name[256];
    read_string("  Nom du fichier (vide pour annuler)", name, sizeof(name));
    if (name[0] == '\0') return 0;
    char path[512];
    snprintf(path, sizeof(path), "images/%s", name);
    Image *im = pnm_read(path);
    if (!im) { printf("  ** Impossible de charger '%s'\n", path); return 0; }
    img_free(g_current);
    g_current = im;
    snprintf(g_name, sizeof(g_name), "%s", name);
    print_info(im, "Chargee");
    return 1;
}

/* Garantit qu'une image est chargee : sinon, propose d'en charger une. */
static int require_image(void) {
    if (g_current) return 1;
    printf("\n  Aucune image chargee.\n");
    return do_load_image();
}

/* ------------------------------------------------------------------ */
/* Menus par categorie                                                */
/* ------------------------------------------------------------------ */
static void menu_file(void) {
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== FICHIER / IMAGE ===\n");
        printf(" 1. Charger une image (images/)\n");
        printf(" 2. Enregistrer l'image courante\n");
        printf(" 3. Informations sur l'image courante\n");
        printf(" 4. Convertir en niveaux de gris\n");
        printf(" 5. Generer les images de test\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        if (c == 1) {
            do_load_image();
        } else if (c == 2) {
            if (!require_image()) continue;
            char name[256];
            read_string("  Nom de sortie (sans extension)", name, sizeof(name));
            const char *ext = (g_current->channels == 1) ? "pgm" : "ppm";
            char path[512];
            snprintf(path, sizeof(path), "output/%s.%s", name, ext);
            if (pnm_write(path, g_current) == 0) printf("  Enregistre: %s\n", path);
        } else if (c == 3) {
            print_info(g_current, "Courante");
        } else if (c == 4) {
            if (!require_image()) continue;
            set_result(img_to_gray(g_current), "gris");
        } else if (c == 5) {
            int rc = system("./gen_samples");
            if (rc != 0) printf("  ** Compilez d'abord: make samples\n");
        }
    }
}

static void menu_point(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== TRAITEMENTS PONCTUELS / HISTOGRAMME (support 02) ===\n");
        printf(" 1. Afficher statistiques (luminance, contraste, dynamique)\n");
        printf(" 2. Enregistrer l'image de l'histogramme\n");
        printf(" 3. Transformation lineaire (etirement min-max)\n");
        printf(" 4. Transformation lineaire avec saturation\n");
        printf(" 5. Transformation lineaire par morceaux\n");
        printf(" 6. Correction gamma (non lineaire)\n");
        printf(" 7. Negatif\n");
        printf(" 8. Egalisation d'histogramme (globale)\n");
        printf(" 9. Egalisation d'histogramme (locale)\n");
        printf("10. Etirement de dynamique (histogram stretch)\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        switch (c) {
            case 1: {
                int mn, mx;
                Image *g = img_to_gray(g_current);
                img_minmax(g, &mn, &mx);
                printf("  Luminance (moyenne) = %.2f\n", img_mean(g));
                printf("  Contraste (ecart-type) = %.2f\n", img_stddev(g));
                printf("  Dynamique = [%d, %d]\n", mn, mx);
                img_free(g);
                break;
            }
            case 2: set_result(histogram_to_image(g_current, 200), "histogramme"); break;
            case 3: set_result(op_linear(g_current), "lineaire"); break;
            case 4: {
                int smin = read_int("  Seuil bas (smin)", 50);
                int smax = read_int("  Seuil haut (smax)", 200);
                set_result(op_linear_saturation(g_current, smin, smax), "saturation");
                break;
            }
            case 5: {
                int i1 = read_int("  in1", 80);
                int o1 = read_int("  out1", 30);
                int i2 = read_int("  in2", 175);
                int o2 = read_int("  out2", 225);
                set_result(op_piecewise_linear(g_current, i1, o1, i2, o2), "morceaux");
                break;
            }
            case 6: {
                double gm = read_double("  gamma (<1 eclaircit, >1 assombrit)", 0.5);
                set_result(op_gamma(g_current, gm), "gamma");
                break;
            }
            case 7: set_result(op_negative(g_current), "negatif"); break;
            case 8: set_result(histogram_equalize(g_current), "egalisation"); break;
            case 9: {
                int w = read_int("  Taille de la fenetre (impair)", 7);
                set_result(histogram_equalize_local(g_current, w), "egalisation_locale");
                break;
            }
            case 10: set_result(histogram_stretch(g_current), "etirement"); break;
        }
    }
}

static void menu_arith(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== OPERATIONS ARITHMETIQUES ET LOGIQUES (support 02) ===\n");
        printf(" 1. Addition avec une autre image\n");
        printf(" 2. Soustraction avec une autre image\n");
        printf(" 3. Multiplication par un facteur\n");
        printf(" 4. Melange (blend) avec une autre image\n");
        printf(" 5. ET logique\n");
        printf(" 6. OU logique\n");
        printf(" 7. OU exclusif (XOR)\n");
        printf(" 8. NON (complement)\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        if (c == 3) {
            double r = read_double("  Facteur", 1.5);
            set_result(op_mul_scalar(g_current, r), "multiplication");
            continue;
        }
        if (c == 8) { set_result(op_not(g_current), "non"); continue; }

        Image *b = load_second();
        if (!b) continue;
        /* aligne les canaux/tailles au besoin en convertissant en gris */
        if (b->channels != g_current->channels ||
            b->width != g_current->width || b->height != g_current->height) {
            printf("  ** Les deux images doivent avoir memes dimensions et type.\n");
            printf("     (Astuce: redimensionnez ou convertissez en gris avant.)\n");
            img_free(b);
            continue;
        }
        switch (c) {
            case 1: set_result(op_add(g_current, b), "addition"); break;
            case 2: set_result(op_sub(g_current, b), "soustraction"); break;
            case 4: {
                double a = read_double("  alpha (poids image courante)", 0.5);
                set_result(op_blend(g_current, b, a), "blend");
                break;
            }
            case 5: set_result(op_and(g_current, b), "et"); break;
            case 6: set_result(op_or(g_current, b), "ou"); break;
            case 7: set_result(op_xor(g_current, b), "xor"); break;
        }
        img_free(b);
    }
}

static void menu_geometry(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== GEOMETRIE / INTERPOLATION (support 02) ===\n");
        printf(" 1. Redimensionner - plus proche voisin\n");
        printf(" 2. Redimensionner - bilineaire\n");
        printf(" 3. Redimensionner - bicubique\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        int nw = read_int("  Nouvelle largeur", g_current->width * 2);
        int nh = read_int("  Nouvelle hauteur", g_current->height * 2);
        switch (c) {
            case 1: set_result(resize_nearest(g_current, nw, nh), "resize_nn"); break;
            case 2: set_result(resize_bilinear(g_current, nw, nh), "resize_bilineaire"); break;
            case 3: set_result(resize_bicubic(g_current, nw, nh), "resize_bicubique"); break;
        }
    }
}

static void menu_convolution(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== CONVOLUTION / FILTRAGE SPATIAL (support 03) ===\n");
        printf(" 1. Filtre moyenneur (passe-bas)\n");
        printf(" 2. Filtre gaussien 5x5\n");
        printf(" 3. Filtre median (non lineaire)\n");
        printf(" 4. Filtre min\n");
        printf(" 5. Filtre max\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        int k;
        switch (c) {
            case 1: k = read_int("  Taille (impair)", 3); set_result(filter_mean(g_current, k), "moyenneur"); break;
            case 2: set_result(filter_gaussian(g_current), "gaussien"); break;
            case 3: k = read_int("  Taille (impair)", 3); set_result(filter_median(g_current, k), "median"); break;
            case 4: k = read_int("  Taille (impair)", 3); set_result(filter_min(g_current, k), "min"); break;
            case 5: k = read_int("  Taille (impair)", 3); set_result(filter_max(g_current, k), "max"); break;
        }
    }
}

static void menu_fourier(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== TRANSFORMEE DE FOURIER (support 04) ===\n");
        printf(" 1. Spectre de Fourier (log magnitude)\n");
        printf(" 2. Filtrage passe-bas frequentiel\n");
        printf(" 3. Filtrage passe-haut frequentiel\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        switch (c) {
            case 1: set_result(fourier_spectrum(g_current), "spectre_fourier"); break;
            case 2: { double r = read_double("  Rayon de coupure", 40); set_result(fourier_lowpass(g_current, r), "fft_passe_bas"); break; }
            case 3: { double r = read_double("  Rayon de coupure", 20); set_result(fourier_highpass(g_current, r), "fft_passe_haut"); break; }
        }
    }
}

static void menu_edges(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== DETECTION DE CONTOURS (support 05) ===\n");
        printf(" 1. Roberts\n");
        printf(" 2. Prewitt\n");
        printf(" 3. Sobel\n");
        printf(" 4. Laplacien (4-connexe)\n");
        printf(" 5. Laplacien (8-connexe)\n");
        printf(" 6. Gradient de Sobel + seuillage\n");
        printf(" 7. Hough - image de l'accumulateur\n");
        printf(" 8. Hough - droites tracees\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        switch (c) {
            case 1: set_result(edge_roberts(g_current), "roberts"); break;
            case 2: set_result(edge_prewitt(g_current), "prewitt"); break;
            case 3: set_result(edge_sobel(g_current), "sobel"); break;
            case 4: set_result(edge_laplacian(g_current, 0), "laplacien4"); break;
            case 5: set_result(edge_laplacian(g_current, 1), "laplacien8"); break;
            case 6: { int s = read_int("  Seuil", 60); set_result(edge_gradient_threshold(g_current, s), "gradient_seuil"); break; }
            case 7: { int s = read_int("  Seuil contour", 80); set_result(edge_hough_lines(g_current, s, 0, 1), "hough_accumulateur"); break; }
            case 8: {
                int s = read_int("  Seuil contour", 80);
                int v = read_int("  Votes minimum (0=auto)", 0);
                set_result(edge_hough_lines(g_current, s, v, 0), "hough_droites");
                break;
            }
        }
    }
}

static void menu_segmentation(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== SEGMENTATION (support 06) ===\n");
        printf(" 1. Seuillage global\n");
        printf(" 2. Multi-seuillage (3 classes)\n");
        printf(" 3. Seuillage automatique de Otsu\n");
        printf(" 4. Seuillage local adaptatif\n");
        printf(" 5. k-moyennes\n");
        printf(" 6. Croissance de region\n");
        printf(" 7. Division-fusion (split & merge)\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        switch (c) {
            case 1: { int t = read_int("  Seuil", 128); set_result(seg_threshold(g_current, t), "seuil"); break; }
            case 2: {
                int seuils[2];
                seuils[0] = read_int("  Seuil 1", 85);
                seuils[1] = read_int("  Seuil 2", 170);
                set_result(seg_threshold_multi(g_current, seuils, 2), "multiseuil");
                break;
            }
            case 3: {
                int t = seg_otsu_value(g_current);
                printf("  Seuil de Otsu calcule = %d\n", t);
                set_result(seg_threshold_otsu(g_current), "otsu");
                break;
            }
            case 4: { int b = read_int("  Taille des blocs", 32); set_result(seg_threshold_adaptive(g_current, b), "adaptatif"); break; }
            case 5: { int k = read_int("  Nombre de classes k", 3); set_result(seg_kmeans(g_current, k), "kmeans"); break; }
            case 6: {
                int sx = read_int("  Germe x", g_current->width / 2);
                int sy = read_int("  Germe y", g_current->height / 2);
                int tol = read_int("  Tolerance", 15);
                set_result(seg_region_growing(g_current, sx, sy, tol), "croissance");
                break;
            }
            case 7: { double v = read_double("  Seuil de variance", 100); set_result(seg_split_merge(g_current, v), "split_merge"); break; }
        }
    }
}

static void menu_morphology(void) {
    if (!require_image()) { pause_enter(); return; }
    int first = 1;
    for (;;) {
        if (!first) pause_enter();
        first = 0;
        clear_screen();
        printf("\n=== IMAGES BINAIRES / MORPHOLOGIE (support 07) ===\n");
        printf("(Les operations binarisent automatiquement via Otsu si besoin)\n");
        printf(" 1. Binarisation (seuil)\n");
        printf(" 2. Erosion\n");
        printf(" 3. Dilatation\n");
        printf(" 4. Ouverture\n");
        printf(" 5. Fermeture\n");
        printf(" 6. Gradient morphologique\n");
        printf(" 7. Gradient interne\n");
        printf(" 8. Gradient externe\n");
        printf(" 9. Etiquetage en composantes connexes\n");
        printf("10. Carte de distance (D4/D8)\n");
        printf("11. Seuillage par hysteresis\n");
        printf(" 0. Retour\n");
        int c = read_int("Choix", 0);
        if (c == 0) return;
        int conn;
        switch (c) {
            case 1: { int t = read_int("  Seuil", 128); set_result(morph_binarize(g_current, t), "binaire"); break; }
            case 2: conn = read_int("  Connexite (4/8)", 8); set_result(morph_erode(g_current, conn), "erosion"); break;
            case 3: conn = read_int("  Connexite (4/8)", 8); set_result(morph_dilate(g_current, conn), "dilatation"); break;
            case 4: conn = read_int("  Connexite (4/8)", 8); set_result(morph_open(g_current, conn), "ouverture"); break;
            case 5: conn = read_int("  Connexite (4/8)", 8); set_result(morph_close(g_current, conn), "fermeture"); break;
            case 6: conn = read_int("  Connexite (4/8)", 8); set_result(morph_gradient(g_current, conn), "grad_morpho"); break;
            case 7: conn = read_int("  Connexite (4/8)", 8); set_result(morph_gradient_internal(g_current, conn), "grad_interne"); break;
            case 8: conn = read_int("  Connexite (4/8)", 8); set_result(morph_gradient_external(g_current, conn), "grad_externe"); break;
            case 9: {
                conn = read_int("  Connexite (4/8)", 8);
                int nb = 0;
                Image *r = morph_label(g_current, conn, &nb);
                printf("  Nombre de composantes connexes = %d\n", nb);
                set_result(r, "etiquettes");
                break;
            }
            case 10: { int m = read_int("  Metrique (4=Manhattan, 8=echiquier)", 4); set_result(morph_distance(g_current, m), "distance"); break; }
            case 11: {
                int lo = read_int("  Seuil bas", 40);
                int hi = read_int("  Seuil haut", 90);
                set_result(morph_hysteresis(g_current, lo, hi), "hysteresis");
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
int main(void) {
    for (;;) {
        clear_screen();
        printf("========================================================\n");
        printf("   TP TRAITEMENT D'IMAGES - INF372 at INF-L3 UY1 \n");
        printf("   Implementation modulaire en C des supports de cours\n");
        printf("========================================================\n");
        printf("Formats supportes: PGM (gris) et PPM (couleur), non compresses.\n");
        printf("\n############### MENU PRINCIPAL ###############\n");
        printf("Image courante: %s\n", g_name[0] ? g_name : "(aucune)");
        printf(" 1. Fichier / Image (charger, enregistrer, infos)\n");
        printf(" 2. Traitements ponctuels / histogramme   (support 02)\n");
        printf(" 3. Operations arithmetiques et logiques  (support 02)\n");
        printf(" 4. Geometrie / interpolation             (support 02)\n");
        printf(" 5. Convolution / filtrage spatial        (support 03)\n");
        printf(" 6. Transformee de Fourier                (support 04)\n");
        printf(" 7. Detection de contours                 (support 05)\n");
        printf(" 8. Segmentation                          (support 06)\n");
        printf(" 9. Images binaires / morphologie         (support 07)\n");
        printf(" 0. Quitter\n");
        int c = read_int("Votre choix", 0);
        switch (c) {
            case 1: menu_file(); break;
            case 2: menu_point(); break;
            case 3: menu_arith(); break;
            case 4: menu_geometry(); break;
            case 5: menu_convolution(); break;
            case 6: menu_fourier(); break;
            case 7: menu_edges(); break;
            case 8: menu_segmentation(); break;
            case 9: menu_morphology(); break;
            case 0:
                img_free(g_current);
                printf("Au revoir !\n");
                return 0;
            default:
                printf("Choix invalide.\n");
                pause_enter();
        }
    }
    (void)flush_line;
    return 0;
}
