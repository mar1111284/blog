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
static ExportOptions global_opts = {0};

void mem_write_func(void *context, void *data, int size) {
    mem_writer_t *w = (mem_writer_t *)context;
    if (w->size + size > w->capacity) return;
    memcpy(w->buf + w->size, data, size);
    w->size += size;
}

void export_ascii(unsigned char *raw_data, int raw_size, ExportOptions opts) {
    //add_line("export_ascii: starting ASCII PNG export...");

    if (raw_size <= 0 || raw_size > 20 * 1024 * 1024) {
        //add_line("export_ascii: invalid image size");
        return;
    }

    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height, &channels, 4);
    if (!pixels) {
        //add_line("export_ascii: decode failed");
        return;
    }

    // --- Settings ---
    int target_width = opts.chars_wide;
    const float char_aspect = 2.2f;
    int target_height = (int)((float)(height * target_width) / (float)width / char_aspect);

    const char *ramp = " .'`^\",:;Il!i~+_-?][}{1)(|\\/*tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    int ramp_len = strlen(ramp);

    // --- Allocate ASCII buffer ---
    size_t buf_size = (target_width + 1LL) * target_height + 1;
    char *ascii = (char *)malloc(buf_size);
    if (!ascii) {
        //add_line("export_ascii: cannot allocate ASCII buffer");
        stbi_image_free(pixels);
        return;
    }
    char *p = ascii;

    // --- Dithering arrays ---
    float *error = (float *)calloc(target_width + 4, sizeof(float));
    float *next_row = (float *)calloc(target_width + 4, sizeof(float));

    // --- ASCII conversion ---
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
            error[x+1]      += quant_error * 7.0f/16.0f;    // right
            next_row[x]      += quant_error * 3.0f/16.0f;    // below-left
            next_row[x+1]    += quant_error * 5.0f/16.0f;    // below
            next_row[x+2]    += quant_error * 1.0f/16.0f;    // below-right
        }
        *p++ = '\n';
        memcpy(error, next_row, (target_width+4)*sizeof(float));
        memset(next_row, 0, (target_width+4)*sizeof(float));
    }
    free(error);
    free(next_row);
    *p = '\0';

    // --- Initialize TTF and surface ---
    if (TTF_WasInit() == 0) TTF_Init();
    char font_path[256];
    snprintf(font_path, sizeof(font_path), "font.ttf"); // adjust font
    TTF_Font *font = TTF_OpenFont(font_path, opts.font_size);
    if (!font) {
        //add_line("export_ascii: failed to load font");
        free(ascii);
        stbi_image_free(pixels);
        return;
    }

    int char_width, char_height;
    TTF_SizeUTF8(font, "A", &char_width, NULL);
    char_height = TTF_FontLineSkip(font);

    int img_width  = target_width * char_width;
    int img_height = target_height * char_height;

    SDL_Surface *final_surf = SDL_CreateRGBSurfaceWithFormat(0, img_width, img_height, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(final_surf, NULL, SDL_MapRGBA(final_surf->format, opts.bg[0], opts.bg[1], opts.bg[2], 255));

    // --- Render ASCII onto surface ---
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

    TTF_CloseFont(font);
    free(ascii);
    stbi_image_free(pixels);

    // --- Write PNG to memory ---
    mem_writer_t writer;
    writer.buf = malloc(MAX_PNG_SIZE);
    writer.size = 0;
    writer.capacity = MAX_PNG_SIZE;
    stbi_write_png_to_func(mem_write_func, &writer, final_surf->w, final_surf->h, 4, final_surf->pixels, final_surf->pitch);
    SDL_FreeSurface(final_surf);

    // --- Trigger download ---
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
    }, writer.buf, (int)writer.size, opts.filename);

    free(writer.buf);
    //add_line("export_ascii: PNG download triggered!");
}

void process_image_to_pixels(unsigned char *raw_data, int raw_size) {
    //add_line("Starting ultra-detailed ASCII art...");

    // Sanity & decode
    if (raw_size <= 0 || raw_size > 20 * 1024 * 1024) {
        //add_line("Error: Invalid image size");
        return;
    }

    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height, &channels, 4);
    if (!pixels) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Decode failed: %s", stbi_failure_reason() ? stbi_failure_reason() : "unknown");
        //add_line(msg);
        return;
    }

    char info[128];
    snprintf(info, sizeof(info), "Image decoded: %d × %d (%d ch)", width, height, channels);
    //add_line(info);

    // Settings - tune these!
    int target_width = 130;  // chars wide (120–200 best)
    const float char_aspect = 2.2f;  // lower = less horizontal compression (try 1.8–2.1)
    int target_height = (int)((float)(height * target_width) / (float)width / char_aspect);


    // Debug size
    char size_dbg[128];
    snprintf(size_dbg, sizeof(size_dbg), "Target size: %d wide × %d high (aspect %.1f)", target_width, target_height, char_aspect);
    //add_line(size_dbg);

    // High-quality ramp
    const char *ramp = " .'`^\",:;Il!i~+_-?][}{1)(|\\/*tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    int ramp_len = strlen(ramp);

    // Allocate output
    size_t buf_size = (target_width + 1LL) * target_height + 1;
    char *ascii = (char *)malloc(buf_size);
    if (!ascii) {
        //add_line("Error: Cannot allocate ASCII buffer");
        stbi_image_free(pixels);
        return;
    }
    char *p = ascii;

    // Dithering - your improved version
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

            error[x + 1] += quant_error * 7.0f / 16.0f;      // right
            next_row[x] += quant_error * 3.0f / 16.0f;       // below-left
            next_row[x + 1] += quant_error * 5.0f / 16.0f;   // below
            next_row[x + 2] += quant_error * 1.0f / 16.0f;   // below-right
        }

        *p++ = '\n';

        // Swap error rows
        memcpy(error, next_row, (target_width + 4) * sizeof(float));
        memset(next_row, 0, (target_width + 4) * sizeof(float));
    }

    free(error);
    free(next_row);
    *p = '\0';

    // Print with small font
    //add_line("Using font size 8");

    p = ascii;
    char line_buf[1024];
    int printed = 0;

    while (*p) {
        char *nl = strchr(p, '\n');
        if (nl) {
            size_t len = nl - p;
            if (len >= sizeof(line_buf)) len = sizeof(line_buf) - 1;
            strncpy(line_buf, p, len);
            line_buf[len] = '\0';

            //add_line_with_font(line_buf, font_8,9);
            printed++;

            p = nl + 1;
        } else {
            //add_line_with_font(p, font_8,9);
            printed++;
            break;
        }
    }

    char debug[128];
    snprintf(debug, sizeof(debug), "Printed %d lines (expected ~%d)", printed, target_height);
    //add_line(debug);

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
    if (!image_processing_pending) return 0;

    char status_buf[256];
    status_buf[0] = '\0';

    // Do NOT allocate 20MB on stack! Use heap instead
    static char* base64_buf = NULL;
    static size_t base64_buf_max = 20 * 1024 * 1024 + 1;

    if (!base64_buf) {
        base64_buf = (char*)malloc(base64_buf_max);
        if (!base64_buf) {
            //add_line("Error: Cannot allocate base64 buffer");
            image_processing_pending = 0;
            return 1;
        }
    }
    base64_buf[0] = '\0';

    // Fetch base64 string from sessionStorage
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

    // Error case
    if (strncmp(base64_buf, "__ERROR__:", 10) == 0) {
        image_processing_pending = 0;
        //add_line("Image fetch error:");
        //add_line(base64_buf + 10);
        return 1;
    }

    if (strcmp(base64_buf, "TOO_LARGE") == 0) {
        image_processing_pending = 0;
        //add_line("Error: Image too large (base64 exceeds buffer)");
        return 1;
    }

    // Decode using your function
    static unsigned char* image_raw = NULL;
    static size_t image_raw_max = 15 * 1024 * 1024;

    if (!image_raw) {
        image_raw = (unsigned char*)malloc(image_raw_max);
        if (!image_raw) {
            //add_line("Error: Cannot allocate image buffer");
            image_processing_pending = 0;
            return 1;
        }
    }

    int decoded_bytes = base64_decode(base64_buf, image_raw, image_raw_max);

    if (decoded_bytes < 0) {
        image_processing_pending = 0;
        //add_line("Error: Invalid base64 data");
        return 1;
    }

    image_processing_pending = 0;

    char msg[128];
    snprintf(msg, sizeof(msg), "Image received and decoded (%d bytes)", decoded_bytes);
    //add_line(msg);

    // Now process
    process_image_to_pixels(image_raw, decoded_bytes);

	if (image_download_pending) {
		export_ascii(image_raw, decoded_bytes, global_opts);
		image_download_pending = 0;
	}

    return 1;
}
#endif

