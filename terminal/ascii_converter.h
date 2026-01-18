#ifndef ASCII_CONVERTER_H
#define ASCII_CONVERTER_H

#include <stddef.h>
#include <stdint.h>
#include "sdl.h"

/*
ASCII RAMP PRESETS

RAMP 1 — Wide tonal range (smooth gradients)
RAMP 2 — High contrast (bold shapes)
RAMP 3 — Monospace optimized (terminal-friendly)
RAMP 4 — Unicode enhanced (highest visual fidelity)
*/

#define RAMP_1 " .'`^\",:;Il!i~+_-?][}{1)(|\\/*tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"
#define RAMP_2 " .:-=+*#%@"
#define RAMP_3 "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
#define RAMP_4 " ░▒▓█"

/* ASCII export options (PNG generation, colors, font size) */
typedef struct {
    int chars_wide;        /* number of characters per line */
    int font_size;         /* font size used for rendering */
    uint8_t fg[3];         /* foreground color (RGB) */
    uint8_t bg[3];         /* background color (RGB) */
    const char *filename;  /* output filename (PNG) */
    const char *ramp;
} ExportOptions;

typedef struct {
    unsigned char *buf;
    size_t size;
    size_t capacity;
} mem_writer_t;

// Global State
extern int image_processing_pending;
extern int image_download_pending;

static SDL_Texture *pixel_art_texture;
static SDL_Rect pixel_art_dst;
extern ExportOptions global_opts;

/* Core API */
void process_image_to_pixels(unsigned char *raw_data, int raw_size);
void export_ascii(unsigned char *raw_data, int raw_size, ExportOptions opts);
int poll_image_result(void);

#endif /* ASCII_CONVERTER_H */

