# Makefile - TP Traitement d'images (INF372, Dr Tapamo)
# Compilation modulaire du projet.

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS := -lm

SRC_DIR := src
OBJ_DIR := build
INC_DIR := include

# Modules (tout sauf main.c) : bibliotheque de traitements
LIB_SRC := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c))
LIB_OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRC))

MAIN_OBJ := $(OBJ_DIR)/main.o
BIN      := tp_image
GEN_BIN  := gen_samples
DEMO_BIN := gen_demo

.PHONY: all clean samples demo run mrproper

all: $(BIN)

# Programme principal interactif
$(BIN): $(LIB_OBJ) $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "==> Compilation terminee: ./$(BIN)"

# Compilation des .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generateur d'images de test
$(GEN_BIN): tools/gen_samples.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ tools/gen_samples.c $(LIB_OBJ) $(LDFLAGS)

# Genere les images de test dans images/
samples: $(GEN_BIN)
	./$(GEN_BIN)

# Applique tous les traitements aux images de test -> output/
$(DEMO_BIN): tools/gen_demo.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ tools/gen_demo.c $(LIB_OBJ) $(LDFLAGS)

demo: $(DEMO_BIN) samples
	./$(DEMO_BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: all
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR)

mrproper: clean
	rm -f $(BIN) $(GEN_BIN) $(DEMO_BIN)
