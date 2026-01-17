#ifndef ASCII_CONVERTER_H
#define ASCII_CONVERTER_H

#include <stddef.h>
#include <stdint.h>
#include "sdl.h"

#define RAMP_1 " .'`^\",:;Il!i~+_-?][}{1)(|\\/*tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"

/* ASCII export options (PNG generation, colors, font size) */
typedef struct {
    int chars_wide;        /* number of characters per line */
    int font_size;         /* font size used for rendering */
    uint8_t fg[3];         /* foreground color (RGB) */
    uint8_t bg[3];         /* background color (RGB) */
    const char *filename;  /* output filename (PNG) */
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
static ExportOptions global_opts;

/* Core API */
void process_image_to_pixels(unsigned char *raw_data, int raw_size);
void export_ascii(unsigned char *raw_data, int raw_size, ExportOptions opts);
void mem_write_func(void *context, void *data, int size);
int poll_image_result(void);

#endif /* ASCII_CONVERTER_H */

