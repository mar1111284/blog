# Makefile for Emscripten + SDL2 terminal project
# Put this in the same folder as main.c (or adjust paths)

# === Configuration ===
TARGET = terminal
SOURCES = main.c base64.c data_codec.c settings.c
ASSETS = welcome.txt info.txt manifesto.txt font.ttf assets/bg_barbie.png assets/bg_jurassic.png

# Define the flags as a variable (no indentation here)
EMSDK_FLAGS = \
	-s USE_SDL=2 \
	-s USE_SDL_TTF=2 \
	-s USE_SDL_IMAGE=2 \
	-s WASM=1 \
	-s INITIAL_MEMORY=64MB \
	-s STACK_SIZE=1048576 \
	-s TOTAL_STACK=1048576 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s MAXIMUM_MEMORY=4GB \
	-s EXPORTED_FUNCTIONS='["_main"]' \
	-s EXPORTED_RUNTIME_METHODS='["cwrap", "ccall", "HEAPU8"]' \
	-s ERROR_ON_UNDEFINED_SYMBOLS=0 \
	-s ASSERTIONS=2 \
	-s SAFE_HEAP=1 \
	-s STACK_OVERFLOW_CHECK=2 \
	-s GL_DEBUG=1 \
	-s ENVIRONMENT=web \
	-s MIN_WEBGL_VERSION=2 \
	-s SDL2_IMAGE_FORMATS='["png"]'

# Debug flags
DEBUG_FLAGS = -gsource-map -O0 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2

# Preload all your text/font files
PRELOAD = $(foreach f,$(ASSETS),--preload-file $(f))

# Tools
EMCC = emcc

# === Rules ===

all: $(TARGET).js terminal.html

# Build JS/WASM every time
$(TARGET).js: $(SOURCES) $(ASSETS)
	@echo "Building WebAssembly (JS + WASM)..."
	$(EMCC) $(SOURCES) \
	    $(EMSDK_FLAGS) \
	    $(PRELOAD) \
	    -o $(TARGET).js \
	    $(DEBUG_FLAGS)

# Generate HTML only if it does not exist
terminal.html:
	@if [ ! -f $@ ]; then \
	    echo "Generating terminal.html..."; \
	    # Copy from a template HTML file you provide \
	    cp template.html $@; \
	else \
	    echo "$@ exists — keeping existing HTML"; \
	fi

# Clean JS/WASM only (keep HTML)
clean:
	rm -f $(TARGET).js $(TARGET).wasm $(TARGET).data
	rm -f $(TARGET).js.map $(TARGET).wasm.map   # if using source maps
	@echo "Clean complete. $(TARGET).html was NOT removed."

# Phony targets
.PHONY: all clean

