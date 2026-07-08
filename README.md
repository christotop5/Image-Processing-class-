# TP Traitement d'images — INF372 (Dr Tapamo)

Implémentation modulaire en **C** de tous les traitements présents dans les
supports de cours, avec une interface interactive en ligne de commande et une
compilation gérée par **Make**.

> Pour **comprendre et savoir défendre** le projet (concepts, formules,
> décisions de conception, questions de soutenance), lis le guide d'étude
> [`COMPRENDRE.md`](COMPRENDRE.md).

---

## 1. Format et type de données choisis

Le cours (support *01-Introduction*) recommande explicitement les formats **non
compressés** pour l'entrée/sortie des traitements :

- **PGM** : images en niveaux de gris 8 bits (magic `P5` binaire / `P2` ASCII)
- **PPM** : images couleur RVB 24 bits (magic `P6` binaire / `P3` ASCII)

Ce sont donc les formats utilisés par le projet. En interne, une image est
représentée par la structure `Image` (`include/image.h`) :

```c
typedef struct {
    int width, height;     // dimensions (N x M)
    int channels;          // 1 = gris, 3 = RVB
    int max_value;         // 255 en 8 bits
    unsigned char *data;   // pixels contigus (taille = w*h*channels)
} Image;
```

- Le stockage en `unsigned char` correspond exactement au format 8 bits PGM/PPM.
- Les calculs intermédiaires susceptibles de sortir de `[0,255]` (gradient,
  laplacien, Fourier…) sont faits en `double` puis ramenés dans `[0,255]` via
  `clamp255()`.

## 2. Liste des traitements implémentés

Regroupés par support de cours :

### Support 01 — Introduction
- Lecture/écriture PGM et PPM (`io_pnm`)
- Conversion couleur → niveaux de gris (luminance ITU-R 601)

### Support 02 — Traitement de base (`histogram`, `point_ops`, `geometry`)
- Histogramme + image de l'histogramme
- Luminance (moyenne), contraste (écart-type), dynamique (min/max)
- Transformation linéaire (étirement min–max, via LUT)
- Transformation linéaire avec saturation
- Transformation linéaire par morceaux
- Transformation non linéaire (correction gamma)
- Négatif
- Égalisation d'histogramme (globale et locale)
- Étirement de dynamique (histogram stretch)
- Opérations arithmétiques : addition, soustraction, multiplication, mélange
- Opérations logiques : ET, OU, XOR, NON
- Interpolation / changement d'échelle : plus proche voisin, bilinéaire, bicubique

### Support 03 — Convolution (`convolution`)
- Convolution générique (gestion des bords : zéro, miroir, réplication)
- Filtre moyenneur NxN (passe-bas)
- Filtre gaussien 5x5 (noyau du cours)
- Filtre médian NxN (non linéaire)
- Filtres min / max

### Support 04 — Transformée de Fourier (`fourier`)
- FFT 2D (Cooley-Tukey, zero-padding en puissances de 2)
- Spectre de Fourier rehaussé `log(1+|F|)` recentré
- Filtrage passe-bas fréquentiel
- Filtrage passe-haut fréquentiel

### Support 05 — Détection de contours (`edges`)
- Roberts, Prewitt, Sobel (norme du gradient)
- Laplacien (masques 4-connexe et 8-connexe)
- Gradient de Sobel + seuillage
- Transformée de Hough (accumulateur ρ-θ + tracé des droites)

### Support 06 — Segmentation (`segmentation`)
- Seuillage global
- Multi-seuillage (n+1 classes)
- Seuillage automatique de **Otsu**
- Seuillage local adaptatif
- k-moyennes (k-means) sur l'intensité
- Croissance de région (region growing)
- Division-fusion (split & merge, quadtree)

### Support 07 — Images binaires & morphologie (`morphology`)
- Binarisation
- Érosion, dilatation, ouverture, fermeture (connexité 4/8)
- Gradients morphologiques (interne, externe, morphologique)
- Étiquetage en composantes connexes (colorisé en PPM)
- Carte de distance D4 (Manhattan) / D8 (échiquier)
- Seuillage par hystérésis

## 3. Architecture des fichiers

```
.
├── Makefile              # compilation modulaire
├── README.md
├── include/              # en-têtes (interfaces des modules)
│   ├── image.h  io_pnm.h  histogram.h  point_ops.h  geometry.h
│   ├── convolution.h  fourier.h  edges.h  segmentation.h  morphology.h
├── src/                  # implémentations (un module par thème)
│   ├── image.c  io_pnm.c  histogram.c  point_ops.c  geometry.c
│   ├── convolution.c  fourier.c  edges.c  segmentation.c  morphology.c
│   └── main.c            # menu interactif dynamique
├── tools/
│   └── gen_samples.c     # génère des images de test
├── images/               # images d'entrée (PGM/PPM)
├── output/               # résultats des traitements
└── build/                # fichiers objets (.o)
```

Chaque traitement vit dans son module (`.h` + `.c`), indépendant et réutilisable.

## 4. Compilation et exécution

```bash
make            # compile le programme principal -> ./tp_image
make samples    # génère les images de test dans images/
make demo       # applique TOUS les traitements aux images de test -> output/
make run        # compile puis lance le programme
make clean      # supprime les objets
make mrproper   # supprime tout (objets + binaires)
```

### Générer tous les traitements d'un coup (`make demo`)

`make demo` applique automatiquement chaque traitement aux images de test et
enregistre les résultats dans `output/`, classés par support de cours :

```
output/
├── 01_couleur_vers_gris.pgm
├── 02_base/          (lineaire, gamma, egalisation, addition, resize, …)
├── 03_convolution/   (moyenneur, gaussien, median, min, max)
├── 04_fourier/       (spectre, passe_bas, passe_haut)
├── 05_contours/      (roberts, prewitt, sobel, laplacien, hough, …)
├── 06_segmentation/  (seuil, otsu, kmeans, croissance, split_merge, …)
└── 07_morphologie/   (erosion, dilatation, gradient, etiquettes, distance, …)
```

C'est le moyen le plus rapide de **voir l'effet de chaque traitement** : ouvrez
les images du dossier `output/` dans votre visionneuse et comparez-les à
l'image d'origine dans `images/`.

Prérequis : `gcc`, `make` (et `libm`, standard).

## 5. Utilisation

```bash
make            # (une fois)
make samples    # (une fois, pour créer des images de test)
./tp_image
```

Le menu principal propose 9 catégories correspondant aux supports de cours.
Déroulé typique :

1. Menu **1 → Charger une image** (ex. `shapes.pgm`).
2. Choisir une catégorie (ex. **7 → Détection de contours**), puis un
   traitement (ex. Sobel).
3. Le résultat **devient l'image courante**, est enregistré dans `output/`, et
   le programme propose de l'ouvrir dans la visionneuse (`xdg-open`).
4. On peut enchaîner les traitements : chaque opération s'applique au résultat
   précédent.

### Voir l'effet d'un traitement
- Les statistiques (moyenne, écart-type, min/max) sont affichées avant/après.
- Chaque résultat est écrit dans `output/<nom>.pgm|ppm` et peut être ouvert
  directement (option « Ouvrir dans la visionneuse »).

## 6. Images de test fournies (`make samples`)

| Fichier            | Contenu                          | Utile pour                          |
|--------------------|----------------------------------|-------------------------------------|
| `gradient.pgm`     | dégradé horizontal               | étirement, égalisation, gamma       |
| `lowcontrast.pgm`  | faible contraste                 | amélioration de contraste, k-means  |
| `shapes.pgm`       | disque + rectangle               | contours, segmentation, morphologie |
| `checker.pgm`      | damier                           | Fourier, convolution, Hough         |
| `noise.pgm`        | formes + bruit poivre et sel     | filtre médian, morphologie          |
| `color.ppm`        | dégradé couleur RVB              | conversion en gris, opérations      |

Vous pouvez aussi déposer vos propres images PGM/PPM dans `images/`.
Pour convertir une image JPEG/PNG en PGM/PPM (ex. avec ImageMagick) :

```bash
convert photo.jpg -compress none images/photo.ppm   # couleur
convert photo.jpg -colorspace Gray images/photo.pgm # gris
```
