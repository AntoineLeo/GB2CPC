# GB2CPC -- top-level developer Makefile
#
# This drives the parts that work without a full retro toolchain (asset
# pipeline + tests) and provides a best-effort SDCC syntax-check of the C HAL.
# Producing a runnable .dsk for the CPC is done through CPCtelera -- see
# `make help` and docs/BUILD.md.

PYTHON ?= python
SDCC   ?= sdcc

INCLUDE_DIR := include
SRC_DIR     := src
EXAMPLE     := examples/hello_sprite
BUILD_DIR   := build

.PHONY: help test assets syntax-check clean

help:
	@echo "GB2CPC make targets:"
	@echo "  make test         - run the Python asset-pipeline unit tests"
	@echo "  make assets       - regenerate the demo sprite header from player.2bpp"
	@echo "  make syntax-check - compile-check the C HAL with SDCC (if installed)"
	@echo "  make clean        - remove build artifacts"
	@echo ""
	@echo "Full CPC build (needs CPCtelera): see docs/BUILD.md"

test:
	$(PYTHON) tests/test_pipeline.py

assets:
	$(PYTHON) tools/asset_converter.py \
		-i $(EXAMPLE)/player.2bpp -o $(EXAMPLE)/assets.h --name player

# Best-effort: parse/type-check the core + demo against SDCC for Z80.
# This does NOT link a CPC binary (that needs CPCtelera's headers/linker); it
# only catches C-level errors early. Requires a CPCtelera include path in
# CPCT_INCLUDE for the <cpctelera.h> dependency to resolve.
syntax-check:
	@mkdir -p $(BUILD_DIR)
	$(SDCC) -mz80 -c --std-c11 \
		-I$(INCLUDE_DIR) $(if $(CPCT_INCLUDE),-I$(CPCT_INCLUDE),) \
		$(SRC_DIR)/gb2cpc_core.c -o $(BUILD_DIR)/gb2cpc_core.rel

clean:
	rm -rf $(BUILD_DIR)
	rm -rf tests/__pycache__ tools/gb2cpc/__pycache__
