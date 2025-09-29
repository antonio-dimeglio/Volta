# OCaml native build for your language project

OCAMLOPT := ocamlopt
LIB_DIR := lib
BIN_DIR := bin
BUILD_DIR := _build

SRC_LIB := $(wildcard $(LIB_DIR)/*.ml)
LIB_OBJS := $(patsubst $(LIB_DIR)/%.ml, $(BUILD_DIR)/%.cmx, $(SRC_LIB))

MAIN := main
MAIN_SRC := $(BIN_DIR)/$(MAIN).ml
MAIN_EXE := $(BUILD_DIR)/$(MAIN)

all: $(MAIN_EXE)

# Build library modules
$(BUILD_DIR)/%.cmx: $(LIB_DIR)/%.ml | $(BUILD_DIR)
	$(OCAMLOPT) -c $< -o $@

# Build main executable
$(MAIN_EXE): $(MAIN_SRC) $(LIB_OBJS) | $(BUILD_DIR)
	$(OCAMLOPT) -o $@ $(LIB_OBJS) $<

# Ensure build dir exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) *.o *.cm* *.cmi

run: all
	./$(MAIN_EXE)

.PHONY: all clean run
