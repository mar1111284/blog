# Makefile for Emscripten + SDL2 terminal project
# Put this in the same folder as main.c (or adjust paths)

# === Configuration ===
TARGET = terminal
SOURCES = main.c base64.c data_codec.c settings.c             # Add more .c files here if you have them, e.g. terminal.c utils.c
ASSETS = welcome.txt info.txt manifesto.txt font.ttf assets/bg_barbie.png assets/bg_jurassic.png

EMSDK_FLAGS = \
    -s USE_SDL=2 \
    -s USE_SDL_TTF=2 \
    -s USE_SDL_IMAGE=2 \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=64MB \
    -s MAXIMUM_MEMORY=4GB \
    -s EXPORTED_FUNCTIONS='["_main"]' \
    -s EXPORTED_RUNTIME_METHODS='["cwrap", "ccall", "cwrap"]' \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s ASSERTIONS=2 \
    -s SAFE_HEAP=1 \
    -s STACK_OVERFLOW_CHECK=2 \
    -s GL_DEBUG=1 \
    -s ENVIRONMENT=web \
    -s MIN_WEBGL_VERSION=2 \
    -s SDL2_IMAGE_FORMATS='["png"]'

# No optimization — makes debugging much easier
# (we use -O0 in DEBUG_FLAGS instead)

# Optional: useful during debugging/development
DEBUG_FLAGS = -gsource-map -O0 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2

# Release / optimized build flags (uncomment when ready)
# RELEASE_FLAGS = -O3 -s ASSERTIONS=0

# Preload all your text/font files
PRELOAD = $(foreach f,$(ASSETS),--preload-file $(f))

# === Tools ===
EMCC = emcc

# === Rules ===

all: $(TARGET).html

$(TARGET).html: $(SOURCES) $(ASSETS)
	@echo "Building WebAssembly version..."
	$(EMCC) $(SOURCES) \
		$(EMSDK_FLAGS) \
		$(PRELOAD) \
		-o $@ \
		$(DEBUG_FLAGS)   # remove/comment this line for release build

# Clean old build artifacts (very useful!)
clean:
	rm -f $(TARGET).js $(TARGET).wasm $(TARGET).html $(TARGET).data
	rm -f $(TARGET).js.map $(TARGET).wasm.map   # if using source maps
	@echo "Clean complete."

# Phony targets
.PHONY: all clean
