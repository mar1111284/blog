#include "ascii_converter.h"
#include "global.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "base64.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif

static SDL_Texture *pixel_art_texture = NULL;
static SDL_Rect pixel_art_dst = {0};  // position & size
ExportOptions global_opts = {0};

static void mem_write_func(void *context, void *data, int size)
{
    mem_writer_t *w = (mem_writer_t *)context;
    if (size <= 0) return;
    size_t needed = w->size + (size_t)size;
    if (needed > w->capacity) {
        char log[128];
        snprintf(log, sizeof(log), "mem_write_func: overflow (need %zu, have %zu)", needed, w->capacity);
        add_terminal_line(log, LINE_FLAG_ERROR);
        return;
    }
    memcpy(w->buf + w->size, data, (size_t)size);
    w->size += (size_t)size;
}

void export_ascii(unsigned char *raw_data, int raw_size, ExportOptions opts) {
    add_terminal_line("\n", LINE_FLAG_NONE);
    add_terminal_line("export_ascii: starting ASCII PNG export...", LINE_FLAG_SYSTEM);

    // ── Dump the full user options (opts) ───────────────────────────────────
    char buf[256];
    add_terminal_line("User options (opts):", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  chars_wide:   %d", opts.chars_wide);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  font_size:    %d", opts.font_size);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  bg color:     (%d,%d,%d,%d)", opts.bg[0], opts.bg[1], opts.bg[2], 255);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  fg color:     (%d,%d,%d,%d)", opts.fg[0], opts.fg[1], opts.fg[2], 255);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  filename:     %s", opts.filename ? opts.filename : "(null)");
    add_terminal_line(buf, LINE_FLAG_NONE);

    // ── Input validation ────────────────────────────────────────────────────
    if (raw_size <= 0 || raw_size > 20 * 1024 * 1024) {
        add_terminal_line("export_ascii: invalid image size", LINE_FLAG_ERROR);
        return;
    }

    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height, &channels, 4);
    if (!pixels) {
        add_terminal_line("export_ascii: stbi_load_from_memory failed", LINE_FLAG_ERROR);
        return;
    }
    add_terminal_line("Image loaded OK", LINE_FLAG_SYSTEM);

    // ── Target dimensions with safe fallbacks ───────────────────────────────
    int target_width = opts.chars_wide;
    if (target_width <= 0 || target_width > 500) {
        target_width = 80;  // sane default
        snprintf(buf, sizeof(buf), "Warning: invalid chars_wide (%d) → using default %d", opts.chars_wide, target_width);
        add_terminal_line(buf, LINE_FLAG_SYSTEM);
    }

    const float char_aspect = 2.2f;
    int target_height = (int)((float)(height * target_width) / (float)width / char_aspect);
    if (target_height <= 0 || target_height > 1000) {
        target_height = 50;  // sane default
        snprintf(buf, sizeof(buf), "Warning: target_height invalid (%d) → using default %d", target_height, 50);
        add_terminal_line(buf, LINE_FLAG_SYSTEM);
    }

    snprintf(buf, sizeof(buf), "Target dimensions: %d chars wide × %d chars high", target_width, target_height);
    add_terminal_line(buf, LINE_FLAG_NONE);

    // ── ASCII buffer & dithering ────────────────────────────────────────────
    const char *ramp = opts.ramp;
    int ramp_len = strlen(ramp);

    size_t buf_size = (target_width + 1LL) * target_height + 1;
    char *ascii = (char *)malloc(buf_size);
    if (!ascii) {
        add_terminal_line("malloc failed for ASCII buffer", LINE_FLAG_ERROR);
        stbi_image_free(pixels);
        return;
    }
    char *p = ascii;

    float *error = (float *)calloc(target_width + 4, sizeof(float));
    float *next_row = (float *)calloc(target_width + 4, sizeof(float));
    if (!error || !next_row) {
        add_terminal_line("malloc failed for dithering arrays", LINE_FLAG_ERROR);
        free(ascii);
        if (error) free(error);
        if (next_row) free(next_row);
        stbi_image_free(pixels);
        return;
    }

    // Dithering loop (unchanged)
    for (int y = 0; y < target_height; y++) {
        for (int x = 0; x < target_width; x++) {
            int sx = (x * width) / target_width;
            int sy = (y * height) / target_height;
            unsigned char *px = pixels + (sy * width + sx) * 4;

            float r = px[0], g = px[1], b = px[2];
            float gray = 0.299f*r + 0.587f*g + 0.114f*b + error[x+1];

            if (gray < 0) gray = 0;
            if (gray > 255) gray = 255;

            int idx = (int)(gray * ramp_len / 256.0f);
            if (idx >= ramp_len) idx = ramp_len-1;
            if (idx < 0) idx = 0;

            *p++ = ramp[idx];

            float quant_error = gray - (idx * 255.0f / (ramp_len-1));
            error[x+1]      += quant_error * 7.0f/16.0f;
            next_row[x]      += quant_error * 3.0f/16.0f;
            next_row[x+1]    += quant_error * 5.0f/16.0f;
            next_row[x+2]    += quant_error * 1.0f/16.0f;
        }
        *p++ = '\n';
        memcpy(error, next_row, (target_width+4)*sizeof(float));
        memset(next_row, 0, (target_width+4)*sizeof(float));
    }
    free(error);
    free(next_row);
    *p = '\0';

    add_terminal_line("ASCII art generated OK", LINE_FLAG_SYSTEM);

    // ── Font & surface ──────────────────────────────────────────────────────
    if (TTF_WasInit() == 0) TTF_Init();
    TTF_Font *font = TTF_OpenFont(FONT_PATH, opts.font_size);
    if (!font) {
        add_terminal_line("export_ascii: failed to load font", LINE_FLAG_ERROR);
        free(ascii);
        stbi_image_free(pixels);
        return;
    }
    add_terminal_line("Font opened OK", LINE_FLAG_SYSTEM);

    int char_width, char_height;
    TTF_SizeUTF8(font, "A", &char_width, NULL);
    char_height = TTF_FontLineSkip(font);

    int img_width  = target_width * char_width;
    int img_height = target_height * char_height;

    if (img_width <= 0 || img_height <= 0) {
        add_terminal_line("Error: invalid final image dimensions (w/h <= 0)", LINE_FLAG_ERROR);
        TTF_CloseFont(font);
        free(ascii);
        stbi_image_free(pixels);
        return;
    }

    snprintf(buf, sizeof(buf), "Creating surface: %d × %d pixels", img_width, img_height);
    add_terminal_line(buf, LINE_FLAG_NONE);

    SDL_Surface *final_surf = SDL_CreateRGBSurfaceWithFormat(0, img_width, img_height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!final_surf) {
        add_terminal_line("SDL_CreateRGBSurfaceWithFormat failed", LINE_FLAG_ERROR);
        TTF_CloseFont(font);
        free(ascii);
        stbi_image_free(pixels);
        return;
    }

    if (final_surf->pixels == NULL || final_surf->pitch <= 0) {
        add_terminal_line("Error: final_surf is empty or invalid (pixels NULL or pitch <=0)", LINE_FLAG_ERROR);
        SDL_FreeSurface(final_surf);
        TTF_CloseFont(font);
        free(ascii);
        stbi_image_free(pixels);
        return;
    }

    SDL_FillRect(final_surf, NULL, SDL_MapRGBA(final_surf->format, opts.bg[0], opts.bg[1], opts.bg[2], 255));

    // ── Render loop (unchanged) ─────────────────────────────────────────────
    int y_offset = 0;
    char *line = ascii;
    while (*line) {
        char *nl = strchr(line, '\n');
        if (!nl) nl = line + strlen(line);
        int len = nl - line;
        char tmp[1024];
        if (len >= sizeof(tmp)) len = sizeof(tmp)-1;
        strncpy(tmp, line, len);
        tmp[len] = '\0';

        SDL_Color fg = {opts.fg[0], opts.fg[1], opts.fg[2], 255};
        SDL_Surface *surf = TTF_RenderUTF8_Blended(font, tmp, fg);
        if (surf) {
            SDL_Rect dst = {0, y_offset, surf->w, surf->h};
            SDL_BlitSurface(surf, NULL, final_surf, &dst);
            SDL_FreeSurface(surf);
        }
        y_offset += char_height;
        line = (*nl) ? nl+1 : nl;
    }

    add_terminal_line("Text rendered to surface OK", LINE_FLAG_SYSTEM);

    // ── PNG write (unchanged except for safety) ─────────────────────────────
    mem_writer_t writer = {0};
    writer.buf = malloc(MAX_PNG_SIZE);
    writer.size = 0;
    writer.capacity = MAX_PNG_SIZE;
    if (!writer.buf) {
        add_terminal_line("malloc for PNG buffer failed", LINE_FLAG_ERROR);
        SDL_FreeSurface(final_surf);
        TTF_CloseFont(font);
        free(ascii);
        stbi_image_free(pixels);
        return;
    }

    int write_result = stbi_write_png_to_func(mem_write_func, &writer, final_surf->w, final_surf->h, 4, final_surf->pixels, final_surf->pitch);

    if (write_result == 0) {
        add_terminal_line("stbi_write_png_to_func FAILED", LINE_FLAG_ERROR);
    } else {
        snprintf(buf, sizeof(buf), "PNG written OK, size: %zu bytes", writer.size);
        add_terminal_line(buf, LINE_FLAG_SYSTEM);
    }

    SDL_FreeSurface(final_surf);
    TTF_CloseFont(font);
    free(ascii);
    stbi_image_free(pixels);

    if (writer.size > 0) {
        EM_ASM_({
            const dataPtr = $0;
            const dataLen = $1;
            const filename = UTF8ToString($2);
            const bytes = new Uint8Array(Module.HEAPU8.buffer, dataPtr, dataLen);
            const blob = new Blob([bytes], {type:"image/png"});
            const url = URL.createObjectURL(blob);
            const a = document.createElement("a");
            a.href = url;
            a.download = filename;
            a.click();
            URL.revokeObjectURL(url);
        }, writer.buf, writer.size, opts.filename);

        add_terminal_line("PNG download triggered!", LINE_FLAG_SYSTEM);
    } else {
        add_terminal_line("No data written to PNG - download skipped", LINE_FLAG_ERROR);
    }

    free(writer.buf);
}

void process_image_to_pixels(unsigned char *raw_data, int raw_size) {
    add_terminal_line("\n", LINE_FLAG_SYSTEM);
    add_terminal_line("Starting ASCII art Generation preview...", LINE_FLAG_SYSTEM);

    if (raw_size <= 0 || raw_size > 20 * 1024 * 1024) {
        add_terminal_line("Error: Invalid image size", LINE_FLAG_ERROR);
        return;
    }

    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height, &channels, 4);
    if (!pixels) {
        add_terminal_line("Error: Failed to decode image", LINE_FLAG_ERROR);
        return;
    }

    char info[128];
    snprintf(info, sizeof(info), "Image decoded: %d × %d (%d ch)", width, height, channels);
    add_terminal_line(info, LINE_FLAG_SYSTEM);

    int target_width = 130;
    const float char_aspect = 2.2f;
    int target_height = (int)((float)(height * target_width) / (float)width / char_aspect);

    char size_dbg[128];
    snprintf(size_dbg, sizeof(size_dbg), "Target size: %d wide × %d high (aspect %.1f)", target_width, target_height, char_aspect);
    add_terminal_line(size_dbg, LINE_FLAG_SYSTEM);
    add_terminal_line("\n", LINE_FLAG_NONE);

    const char *ramp = RAMP_1;
    int ramp_len = strlen(ramp);

    size_t buf_size = (target_width + 1LL) * target_height + 1;
    char *ascii = (char *)malloc(buf_size);
    if (!ascii) {
        add_terminal_line("Error: Cannot allocate ASCII buffer", LINE_FLAG_ERROR);
        stbi_image_free(pixels);
        return;
    }
    char *p = ascii;

    float *error = (float *)calloc(target_width + 4, sizeof(float));
    float *next_row = (float *)calloc(target_width + 4, sizeof(float));

    for (int y = 0; y < target_height; y++) {
        for (int x = 0; x < target_width; x++) {
            int sx = (x * width) / target_width;
            int sy = (y * height) / target_height;
            unsigned char *px = pixels + (sy * width + sx) * 4;
            float r = px[0], g = px[1], b = px[2];
            float gray = 0.299f * r + 0.587f * g + 0.114f * b + error[x + 1];
            if (gray < 0) gray = 0;
            if (gray > 255) gray = 255;
            int idx = (int)(gray * ramp_len / 256.0f);
            if (idx >= ramp_len) idx = ramp_len - 1;
            if (idx < 0) idx = 0;
            *p++ = ramp[idx];
            float quant_error = gray - (idx * 255.0f / (ramp_len - 1));
            error[x + 1] += quant_error * 7.0f / 16.0f;
            next_row[x] += quant_error * 3.0f / 16.0f;
            next_row[x + 1] += quant_error * 5.0f / 16.0f;
            next_row[x + 2] += quant_error * 1.0f / 16.0f;
        }
        *p++ = '\n';
        memcpy(error, next_row, (target_width + 4) * sizeof(float));
        memset(next_row, 0, (target_width + 4) * sizeof(float));
    }

    free(error);
    free(next_row);
    *p = '\0';

    p = ascii;
    char line_buf[1024];
    int printed = 0;
    int prev_font_size = _terminal.settings.font_size;
    set_font_size(&_terminal.settings, 8);

    while (*p) {
        char *nl = strchr(p, '\n');
        if (nl) {
            size_t len = nl - p;
            if (len >= sizeof(line_buf)) len = sizeof(line_buf) - 1;
            strncpy(line_buf, p, len);
            line_buf[len] = '\0';
            add_terminal_line(line_buf, LINE_FLAG_NONE);
            printed++;
            p = nl + 1;
        } else {
            add_terminal_line(p, LINE_FLAG_NONE);
            printed++;
            break;
        }
    }

    set_font_size(&_terminal.settings, prev_font_size);

    char debug[128];
    snprintf(debug, sizeof(debug), "Printed %d lines (expected ~%d)", printed, target_height);
    add_terminal_line("\n", LINE_FLAG_NONE);
    add_terminal_line(debug, LINE_FLAG_SYSTEM);
    add_terminal_line("\n", LINE_FLAG_NONE);

    free(ascii);
    stbi_image_free(pixels);
}

void parse_color(const char *name, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (strcmp(name, "black") == 0) { *r=0; *g=0; *b=0; }
    else if (strcmp(name, "white") == 0) { *r=255; *g=255; *b=255; }
    else if (strcmp(name, "red") == 0) { *r=255; *g=0; *b=0; }
    else if (strcmp(name, "green") == 0) { *r=0; *g=255; *b=0; }
    else if (strcmp(name, "blue") == 0) { *r=0; *g=0; *b=255; }
    else if (strcmp(name, "pink") == 0) { *r=255; *g=192; *b=203; }
    else if (strcmp(name, "purple") == 0) { *r=128; *g=0; *b=128; }
    else { *r=255; *g=255; *b=255; } // default white
}

#ifdef __EMSCRIPTEN__
int poll_image_result(void) {
    if (!_image_processing_pending) return 0;

    static char* base64_buf = NULL;
    static size_t base64_buf_max = 20 * 1024 * 1024 + 1;

    if (!base64_buf) {
        base64_buf = (char*)malloc(base64_buf_max);
        if (!base64_buf) {
            _image_processing_pending = 0;
            return 1;
        }
    }
    base64_buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;
        const res = sessionStorage.getItem("rekav_image_array");
        if (!res) return;
        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) {
            stringToUTF8("TOO_LARGE", out_ptr, maxlen);
            return;
        }
        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_image_array");
    }, base64_buf, base64_buf_max);

    if (base64_buf[0] == '\0') return 0;

    if (strncmp(base64_buf, "__ERROR__:", 10) == 0) {
        _image_processing_pending = 0;
        add_terminal_line(base64_buf + 10, LINE_FLAG_ERROR);
        return 1;
    }

    if (strcmp(base64_buf, "TOO_LARGE") == 0) {
        _image_processing_pending = 0;
        add_terminal_line("Error: Image too large (base64 exceeds buffer)", LINE_FLAG_ERROR);
        return 1;
    }

    static unsigned char* image_raw = NULL;
    static size_t image_raw_max = 15 * 1024 * 1024;

    if (!image_raw) {
        image_raw = (unsigned char*)malloc(image_raw_max);
        if (!image_raw) {
            _image_processing_pending = 0;
            add_terminal_line("Error: Cannot allocate image buffer", LINE_FLAG_ERROR);
            return 1;
        }
    }

    int decoded_bytes = base64_decode(base64_buf, image_raw, image_raw_max);
    if (decoded_bytes < 0) {
        _image_processing_pending = 0;
        add_terminal_line("Error: Invalid base64 data", LINE_FLAG_ERROR);
        return 1;
    }

    _image_processing_pending = 0;

    char msg[128];
    snprintf(msg, sizeof(msg), "Image received and decoded (%d bytes)", decoded_bytes);
    add_terminal_line(msg, LINE_FLAG_SYSTEM);

    process_image_to_pixels(image_raw, decoded_bytes);

    if (_image_download_pending) {
    	add_terminal_line("call export Ascii", LINE_FLAG_SYSTEM);
        export_ascii(image_raw, decoded_bytes, global_opts);
        _image_download_pending = 0;
    }

    return 1;
}
#endif


