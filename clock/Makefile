TARGET  = clock
SOURCES = main.c

EMSDK_FLAGS = \
	-s USE_SDL=2 \
	-s WASM=1 \
	-s INITIAL_MEMORY=32MB \
	-s STACK_SIZE=524288 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s ERROR_ON_UNDEFINED_SYMBOLS=0 \
	-s ASSERTIONS=2 \
	-s MIN_SAFARI_VERSION=140000 \
	-s ENVIRONMENT=web

DEBUG_FLAGS = \
	-gsource-map \
	-O0 \
	-s ASSERTIONS=2

EMCC = emcc

all: env $(TARGET).html $(TARGET).js

env:
	@if [ -z "$$EMSDK" ]; then \
		echo "⚠ EMSDK not set. Run:"; \
		echo "   source ~/emsdk/emsdk_env.sh"; \
		exit 1; \
	else \
		echo "✔ EMSDK environment detected"; \
	fi

# JS/WASM rebuilds normally
$(TARGET).js: $(SOURCES)
	@echo "Building JS + WASM..."
	$(EMCC) $(SOURCES) \
		$(EMSDK_FLAGS) \
		$(DEBUG_FLAGS) \
		-o $(TARGET).js

# HTML is created ONCE
$(TARGET).html:
	@echo "Creating HTML shell (once)"
	$(EMCC) $(SOURCES) \
		$(EMSDK_FLAGS) \
		--shell-file minimal.html \
		-o $(TARGET).html

clean:
	rm -f $(TARGET).js $(TARGET).wasm $(TARGET).data
	rm -f $(TARGET).js.map $(TARGET).wasm.map
	@echo "Clean complete."

.PHONY: all clean env

