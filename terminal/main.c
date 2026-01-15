#include <SDL.h>
#include <SDL_ttf.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif
#include <string.h>
#include <stdio.h>

#include "base64.h"
#include "data_codec.h"
#include "settings.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define WIDTH 800
#define HEIGHT 480

#define MAX_LINES 512
#define MAX_PNG_SIZE (50 * 1024 * 1024) // 50MB for high-res export

#define MAX_HISTORY 64
#define MAX_LINE_LENGTH 512

char *history[MAX_HISTORY];      // FIFO buffer for commands
int history_count = 0;           // number of commands currently stored
int history_pos = -1;            // -1 = not browsing history

typedef struct {
    const char *name;
    double lat;
    double lon;
} City;

typedef struct {
    int chars_wide;        // number of ASCII characters per line
    int font_size;         // pixel font size (font_6)
    uint8_t fg[3];         // font color (RGB)
    uint8_t bg[3];         // background color (RGB)
    const char *filename;  // output PNG filename
} ExportOptions;

int root_active = 0;          // tracks if root is active
int flag_js_root_activate = 0;
int awaiting_sudo_password = 0; // tracks if sudo is waiting for password
static int translation_pending = 0;
static int weather_forecast_pending = 0;
static int image_processing_pending = 0;
static int image_download_pending = 0;
int cursor_pos = 0; // cursor position within input
int terminal_fullscreen = 0;
static int line_heights[MAX_LINES] = {0};

TTF_Font *font_6;

// Global variable for the small font
TTF_Font *font_8 = NULL;


static SDL_Texture *pixel_art_texture = NULL;
static SDL_Rect pixel_art_dst = {0};  // position & size
static ExportOptions global_opts;

#define ROOT_PASSWORD "rekav"

char input[MAX_LINE_LENGTH];
int input_len = 0;

char terminal[MAX_LINES][MAX_LINE_LENGTH];
int line_count = 0;
int scroll_offset_px = 0; // scroll in pixels
int running = 1;





static City cities[] = {
    {"Paris", 48.8566, 2.3522},
    {"London", 51.5074, -0.1278},
    {"New_York", 40.7128, -74.0060},
    {"Tokyo", 35.6895, 139.6917},
    {"Sydney", -33.8688, 151.2093},
    {"Moscow", 55.7558, 37.6173},
    {"Berlin", 52.52, 13.4050},
    {"Toronto", 43.6532, -79.3832},
    {"Rio", -22.9068, -43.1729},
    {"Cape_Town", -33.9249, 18.4241}
};
static const int city_count = sizeof(cities) / sizeof(cities[0]);

// Struct to store each cmd
typedef void (*command_func)(const char *args);

typedef struct {
    const char *name;
    const char *desc;
    command_func func;
} Command;


// cmd prototypes 
void add_line(const char *text);
void clear_terminal();
void cmd_help(const char *args);
void cmd_clear(const char *args);
void cmd_echo(const char *args);
void cmd_open(const char *args);
void execute_command(const char *cmd);
void cmd_ls(const char *args);
void cmd_cat(const char *args);
void cmd_sudo(const char *args);
void cmd_exit(const char *args);
void cmd_settings(const char *args);
void submit_input();
void cmd_fullscreen(const char *args);
void cmd_translate(const char *args);
void cmd_man(const char *args);
void cmd_weather(const char *args);
int poll_translate_result();
void cmd_to_ascii(const char *args);
int poll_image_result(void);
int poll_forecast_result(void);
void process_image_to_pixels(unsigned char *raw_data, int raw_size);
void add_line_with_font(const char *text, TTF_Font *font_override, int line_height_override);
void export_ascii(unsigned char *raw_data, int raw_size, ExportOptions opts);

Command commands[] = {
    {"help",  "Show help",      cmd_help},
    {"clear", "Clear terminal", cmd_clear},
    {"echo",  "Print text",     cmd_echo},
    {"open",  "Open a page",    cmd_open},
    {"ls",    "List",    		cmd_ls},
	{"cat",    "open file",    	cmd_cat},
    {"sudo",    "root login",   cmd_sudo},
    {"exit",    "exit root",   cmd_exit},
    {"settings", "Modify terminal appearance", cmd_settings},
    {"fullscreen", "Toggle fullscreen mode", cmd_fullscreen},
    {"translate",  "Translate text",          cmd_translate},
    {"weather",  "Weather Info.",          cmd_weather},
    {"man",  "Display documentation",          cmd_man},
    {"to_ascii", "Convert image from URL to ASCII art", cmd_to_ascii},

};

int command_count = sizeof(commands) / sizeof(commands[0]);

static TTF_Font* line_fonts[MAX_LINES] = {0};

int get_total_content_height(void) {
    int h = 0;
    for (int i = 0; i < line_count; i++) {
        h += line_heights[i];
    }
    return h;
}

#ifdef __EMSCRIPTEN__
void save_history_to_storage() {
    // Build a JS array as string
    emscripten_run_script(
        "sessionStorage.setItem('rekav_history', JSON.stringify(Module.history_array));"
    );
}
#endif



typedef struct {
    int chars_wide;       // number of characters in width
    int chars_high;       // number of characters in height
    int font_size;        // font_6
    uint8_t fg[3];        // font color
    uint8_t bg[3];        // background color
    const char *filename; // PNG filename
} TestExportOptions;

typedef struct {
    unsigned char *buf;
    size_t size;
    size_t capacity;
} mem_writer_t;

void mem_write_func(void *context, void *data, int size) {
    mem_writer_t *w = (mem_writer_t *)context;
    if (w->size + size > w->capacity) return;
    memcpy(w->buf + w->size, data, size);
    w->size += size;
}
void export_ascii(unsigned char *raw_data, int raw_size, ExportOptions opts) {
    add_line("export_ascii: starting ASCII PNG export...");

    if (raw_size <= 0 || raw_size > 20 * 1024 * 1024) {
        add_line("export_ascii: invalid image size");
        return;
    }

    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height, &channels, 4);
    if (!pixels) {
        add_line("export_ascii: decode failed");
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
        add_line("export_ascii: cannot allocate ASCII buffer");
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
        add_line("export_ascii: failed to load font");
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
    add_line("export_ascii: PNG download triggered!");
}


void add_line_with_font(const char *text, TTF_Font *font_override, int line_height_override) {
    if (line_count >= MAX_LINES) {
        memmove(terminal, terminal + 1, sizeof(terminal) - sizeof(terminal[0]));
        memmove(line_fonts, line_fonts + 1, sizeof(line_fonts) - sizeof(line_fonts[0]));
        memmove(line_heights, line_heights + 1, sizeof(line_heights) - sizeof(line_heights[0]));
        line_count--;
    }

    // Copy text
    strncpy(terminal[line_count], text, MAX_LINE_LENGTH - 1);
    terminal[line_count][MAX_LINE_LENGTH - 1] = '\0';

    // Store font
    line_fonts[line_count] = font_override;

    // Store line height (0 = use default)
    line_heights[line_count] = (line_height_override > 0) ? line_height_override : settings.line_height;

    line_count++;
    
    // -------- Auto-scroll if already at bottom --------
	int input_height = 40;
	int visible_height = HEIGHT - input_height;

	// Where is the bottom right now?
	int total_height_before = get_total_content_height() - line_heights[line_count - 1];
	int bottom_offset_before = total_height_before - visible_height;
	if (bottom_offset_before < 0) bottom_offset_before = 0;

	// If user WAS at bottom → keep them at bottom
	if (scroll_offset_px >= bottom_offset_before - 1) {
		int total_height_now = get_total_content_height();
		scroll_offset_px = total_height_now - visible_height;
		if (scroll_offset_px < 0) scroll_offset_px = 0;
	}

}

void process_image_to_pixels(unsigned char *raw_data, int raw_size) {
    add_line("Starting ultra-detailed ASCII art...");

    // Sanity & decode
    if (raw_size <= 0 || raw_size > 20 * 1024 * 1024) {
        add_line("Error: Invalid image size");
        return;
    }

    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height, &channels, 4);
    if (!pixels) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Decode failed: %s", stbi_failure_reason() ? stbi_failure_reason() : "unknown");
        add_line(msg);
        return;
    }

    char info[128];
    snprintf(info, sizeof(info), "Image decoded: %d × %d (%d ch)", width, height, channels);
    add_line(info);

    // Settings - tune these!
    int target_width = 130;  // chars wide (120–200 best)
    const float char_aspect = 2.2f;  // lower = less horizontal compression (try 1.8–2.1)
    int target_height = (int)((float)(height * target_width) / (float)width / char_aspect);
	
	/*
    if (target_height > 100) {
        target_height = 100;
        target_width = (int)((float)(width * target_height) * char_aspect / (float)height);
        char cap_msg[128];
        snprintf(cap_msg, sizeof(cap_msg), "Height capped at 100 lines (width adjusted to %d)", target_width);
        add_line(cap_msg);
    }
    */

    // Debug size
    char size_dbg[128];
    snprintf(size_dbg, sizeof(size_dbg), "Target size: %d wide × %d high (aspect %.1f)", target_width, target_height, char_aspect);
    add_line(size_dbg);

    // High-quality ramp
    const char *ramp = " .'`^\",:;Il!i~+_-?][}{1)(|\\/*tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    int ramp_len = strlen(ramp);

    // Allocate output
    size_t buf_size = (target_width + 1LL) * target_height + 1;
    char *ascii = (char *)malloc(buf_size);
    if (!ascii) {
        add_line("Error: Cannot allocate ASCII buffer");
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
    extern TTF_Font *font_8;
    add_line("Using font size 8");

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

            add_line_with_font(line_buf, font_8,9);
            printed++;

            p = nl + 1;
        } else {
            add_line_with_font(p, font_8,9);
            printed++;
            break;
        }
    }

    char debug[128];
    snprintf(debug, sizeof(debug), "Printed %d lines (expected ~%d)", printed, target_height);
    add_line(debug);

    free(ascii);
    stbi_image_free(pixels);

    add_line("Ultra-detailed ASCII complete!");
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

// Command: to_ascii <image_url>
void cmd_to_ascii(const char *args) {
    image_processing_pending = 1;  // flag for polling

    if (!args || strlen(args) == 0) {
        add_line("Usage: to_ascii <image_url> [download=1 wide=300 font_size=6 bg=black color=white name=output]");
        image_processing_pending = 0;
        return;
    }

    // Split first word (URL) and the rest (options)
    char url[1024];
    char options_str[1024] = {0};
    sscanf(args, "%1023s %[^\n]", url, options_str);

    // Basic URL validation
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        add_line("Error: Please provide a valid http/https URL");
        image_processing_pending = 0;
        return;
    }

    // Parse options
    ExportOptions opts;
    opts.chars_wide = 130;    // default
    opts.font_size = 6;       // default
    opts.fg[0]=255; opts.fg[1]=255; opts.fg[2]=255; // default white
    opts.bg[0]=0; opts.bg[1]=0; opts.bg[2]=0;       // default black
    opts.filename = "ascii_highres.png";

    image_download_pending = 0;

    char key[64], val[64];
    const char *ptr = options_str;
    while (*ptr) {
        if (sscanf(ptr, "%63[^=]=%63s", key, val) == 2) {
            if (strcmp(key, "download") == 0 && atoi(val) != 0) {
                image_download_pending = 1;
            } else if (strcmp(key, "wide") == 0) {
                opts.chars_wide = atoi(val);
            } else if (strcmp(key, "font_size") == 0) {
                opts.font_size = atoi(val);
            } else if (strcmp(key, "bg") == 0) {
                parse_color(val, &opts.bg[0], &opts.bg[1], &opts.bg[2]);
            } else if (strcmp(key, "color") == 0) {
                parse_color(val, &opts.fg[0], &opts.fg[1], &opts.fg[2]);
            } else if (strcmp(key, "name") == 0) {
                opts.filename = strdup(val);
            }
        }

        // Skip to next option
        const char *next = strchr(ptr, ' ');
        if (!next) break;
        ptr = next + 1;
    }

#ifdef __EMSCRIPTEN__
    // Store the request in sessionStorage
    EM_ASM({
        const url = UTF8ToString($0);
        sessionStorage.setItem("rekav_image_request", url);
        console.log("C -> JS image request queued:", url);
    }, url);
#endif

    add_line("Fetching and processing image...");
    add_line("(This may take a few seconds depending on image size)");

    // Save opts globally if download is requested
    if (image_download_pending) {
        // copy opts to a global variable for poll_image_result
        global_opts = opts;
    }
}

void cmd_weather(const char *args) {
    weather_forecast_pending = 1;
    if (!args || strlen(args) == 0) {
        add_line("Usage: weather <city>");
        add_line("Available cities:");
        for (int i = 0; i < city_count; i++) {
            add_line(cities[i].name);
        }
        return;
    }

    // Find city in array (case-insensitive)
    City *selected = NULL;
    for (int i = 0; i < city_count; i++) {
        if (strcasecmp(args, cities[i].name) == 0) { // <-- case-insensitive
            selected = &cities[i];
            break;
        }
    }

    if (!selected) {
        add_line("City not found. Type 'weather' to see available cities.");
        return;
    }

#ifdef __EMSCRIPTEN__
    EM_ASM({
        const lat = $0;
        const lon = $1;
        const city = UTF8ToString($2);

        // Store request in sessionStorage for JS to process
        sessionStorage.setItem(
            "rekav_weather_request",
            JSON.stringify({latitude: lat, longitude: lon, city: city})
        );

        console.log("C -> JS weather request queued:", city, lat, lon);
    }, selected->lat, selected->lon, selected->name);
#endif

    char buf[128];
    snprintf(buf, sizeof(buf), "Fetching weather for %s…", selected->name);
    add_line(buf);
}

void cmd_man(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: man <command>");
        return;
    }

    // Search in man_db
    for (int i = 0; i < man_db_size; i++) {
        if (strcmp(args, man_db[i].cmd) == 0) {
            // Print each line of description
            const char *desc = man_db[i].description;
            const char *line = desc;
            while (*line) {
                const char *next = strchr(line, '\n');
                if (!next) next = line + strlen(line);
                char buf[512];
                int len = next - line;
                if (len >= sizeof(buf)) len = sizeof(buf) - 1;
                memcpy(buf, line, len);
                buf[len] = '\0';
                add_line(buf);
                line = (*next) ? next + 1 : next;
            }
            return;
        }
    }

    add_line("No manual entry for this command.");
}


void cmd_settings(const char *args) {
    char option[64];
    char value[64];

	if (!args || strlen(args) == 0) {
		add_line("");
		add_line("--------------------------------------------------");
		add_line("                   SETTINGS                       ");
		add_line("--------------------------------------------------");
		add_line("");

		add_line("Usage: settings <option> <value>");
		add_line("");
		add_line("Options:");
		add_line("  bg           - Background color (charcoal, dos_blue, pink, green, red)");
		add_line("  font_color   - Font color (white, green, pink, red, orange)");
		add_line("  font_size    - Font size (10-20)");
		add_line("  line_height  - Line height (15-25)");
		add_line("  theme        - Predefined themes (Default, MS-DOS, Barbie, Jurassic, Inferno)");
		
		add_line("");
		add_line("--------------------------------------------------");
		add_line("       Example: settings bg charcoal            ");
		add_line("--------------------------------------------------");
		add_line("");
		
		return;
	}

    // Parse option and value from args
    int matched = sscanf(args, "%63s %63s", option, value);
    if (matched != 2) {
        add_line("Invalid usage! Example: settings font_size 18");
        return;
    }

	if (strcmp(option, "bg") == 0) {
		if (set_background_color(value)) {
		    add_line("Background color applied.");
		} else {
		    add_line("Unknown background color.");
		}
	} 
	else if (strcmp(option, "font_color") == 0) {
		if (set_font_color(value)) {
		    add_line("Font color applied.");
		} else {
		    add_line("Unknown font color.");
		}
	} 
	else if (strcmp(option, "font_size") == 0) {
		int size = atoi(value);
		if (set_font_size(size)) {
		    add_line("Font size applied.");
		} else {
		    add_line("Font size must be between 10 and 20.");
		}
	} 
	else if (strcmp(option, "line_height") == 0) {
		int height = atoi(value);
		if (set_line_height(height)) {
		    add_line("Line height applied.");
		} else {
		    add_line("Line height must be between 15 and 25.");
		}
	} 
	else if (strcmp(option, "theme") == 0) {
		if (apply_theme(value)) {
		    add_line("Theme applied.");
		} else {
		    add_line("Unknown theme.");
		}
	} 
	else {
		add_line("Unknown setting option.");
	}


}

void cmd_translate(const char *args) {
    translation_pending = 1;

    if (!args || strlen(args) == 0) {
        add_line("Usage: translate <source> <target> <text>");
        return;
    }

    char source[16];
    char target[16];
    char text[256];

    // Parse: source, target, rest of line
    if (sscanf(args, "%15s %15s %[^\n]", source, target, text) != 3) {
        add_line("Invalid usage! Example: translate en fr Hello world");
        return;
    }

#ifdef __EMSCRIPTEN__
    // Store translate request in sessionStorage for JS to process
    EM_ASM({
        const source = UTF8ToString($0);
        const target = UTF8ToString($1);
        const text   = UTF8ToString($2);

        sessionStorage.setItem(
            "rekav_translate_request",
            JSON.stringify({
                source: source,
                target: target,
                text: text
            })
        );

        console.log("C -> JS translate request queued:", source, target, text);
    }, source, target, text);
#endif

    add_line("Translating…");
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
            add_line("Error: Cannot allocate base64 buffer");
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
        add_line("Image fetch error:");
        add_line(base64_buf + 10);
        return 1;
    }

    if (strcmp(base64_buf, "TOO_LARGE") == 0) {
        image_processing_pending = 0;
        add_line("Error: Image too large (base64 exceeds buffer)");
        return 1;
    }

    // Decode using your function
    static unsigned char* image_raw = NULL;
    static size_t image_raw_max = 15 * 1024 * 1024;

    if (!image_raw) {
        image_raw = (unsigned char*)malloc(image_raw_max);
        if (!image_raw) {
            add_line("Error: Cannot allocate image buffer");
            image_processing_pending = 0;
            return 1;
        }
    }

    int decoded_bytes = base64_decode(base64_buf, image_raw, image_raw_max);

    if (decoded_bytes < 0) {
        image_processing_pending = 0;
        add_line("Error: Invalid base64 data");
        return 1;
    }

    image_processing_pending = 0;

    char msg[128];
    snprintf(msg, sizeof(msg), "Image received and decoded (%d bytes)", decoded_bytes);
    add_line(msg);

    // Now process
    process_image_to_pixels(image_raw, decoded_bytes);

	if (image_download_pending) {
		export_ascii(image_raw, decoded_bytes, global_opts);
		image_download_pending = 0;
	}

    return 1;
}
#endif


#ifdef __EMSCRIPTEN__
int poll_translate_result(void) {
    if (!translation_pending) return 0;

    // small internal buffer
    char buf[512];
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_translate_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) return;

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_translate_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {
        translation_pending = 0;
        add_line(buf);  // prints automatically
        return 1;
    }

    return 0;
}
#endif


#ifdef __EMSCRIPTEN__
int poll_forecast_result(void) {
    if (!weather_forecast_pending) return 0;

    // Internal buffer for JS → C string
    char buf[8192];  // big enough for multi-line text
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_weather_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) return;

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_weather_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {
        weather_forecast_pending = 0;

        // Split by '\n' and print line by line
        char *line = buf;
        while (*line) {
            char *next = strchr(line, '\n');
            if (!next) next = line + strlen(line);

            char tmp[512];
            int len = next - line;
            if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
            memcpy(tmp, line, len);
            tmp[len] = '\0';

            // Print each line to terminal
            add_line(tmp);

            line = (*next) ? next + 1 : next;
        }

        add_line(""); // extra space after forecast
        return 1;
    }

    return 0;
}
#endif


void cmd_fullscreen(const char *args) {

#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            let canvas = Module['canvas'];
            if (!document.fullscreenElement) {
                canvas.requestFullscreen();
            } else {
                document.exitFullscreen();
            }
        } catch(e) {
            console.error('Fullscreen toggle error:', e);
        }
    });
    add_line("Toggled fullscreen mode.");


#else
    add_line("Fullscreen not supported in native mode.");
#endif
}


// cmd implementation
void cmd_help(const char *args) {
    add_line("");
    add_line("--------------------------------------------------");
    add_line("                     HELP                         ");
    add_line("--------------------------------------------------");
    add_line("");

    add_line("Commands:");
    add_line("  help                         - Show this help message");
    add_line("  clear                        - Clear terminal output");
    add_line("  open <page>                  - Open a page in a new tab");
    add_line("  ls                           - List available pages and files");
    add_line("  sudo                         - Root login");
    add_line("  cat <file>                   - Display the content (info.txt...)");
    add_line("  exit                         - Exit root access");
    add_line("  translate <src> <tgt> <text> - Translate text via MyMemory API");
    add_line("  man <command>                - Show command documentation");
    add_line("  to_ascii <link>              - Convert image to ASCII");
    
    add_line("");
    add_line("--------------------------------------------------");
    add_line("        Type 'man <command>' for more info       ");
    add_line("--------------------------------------------------");
    add_line("");
}


void cmd_clear(const char *args) {
    clear_terminal();
}

void cmd_exit(const char *args) {
    root_active = 0;  // exit root

    #ifdef __EMSCRIPTEN__
    // Clear all session storage (root + articles)
    emscripten_run_script("sessionStorage.clear();");
    #endif

    add_line("Access: guess");
}

void cmd_echo(const char *args) {
    if (args && strlen(args) > 0)
        add_line(args);
}


// List of allowed pages
const char *available_pages[] = {
    "gallery",
    "random",
    "shitpost",
    "about"
};
const int page_count = sizeof(available_pages) / sizeof(available_pages[0]);

void cmd_open(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: open <page>");
        return;
    }

    // Copy input to mutable buffer
    char page[128];
    strncpy(page, args, sizeof(page) - 1);
    page[sizeof(page) - 1] = '\0';

    // Remove ".html" if user typed it
    size_t len = strlen(page);
    if (len > 5 && strcmp(page + len - 5, ".html") == 0) {
        page[len - 5] = '\0';
    }

    // Check if page is in allowed list
    int found = 0;
    for (int i = 0; i < page_count; i++) {
        if (strcmp(page, available_pages[i]) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        add_line("Page not found or unavailable.");
        return;
    }

#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            var page = UTF8ToString($0);
            window.open('/' + page + '.html', '_blank');
        } catch(e) {
            console.error('cmd_open JS error:', e);
        }
    }, page);
#endif

    char buf[128];
    snprintf(buf, sizeof(buf), "Opening %s...", page);
    add_line(buf);
}



void cmd_cat(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: cat <file>");
        return;
    }

    FILE *f = fopen(args, "r");
    if (!f) {
        char buf[256];
        snprintf(buf, sizeof(buf), "cat: %s: No such fileeee", args);
        add_line(buf);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0; // remove newline
        add_line(line);
    }
    fclose(f);
}


void cmd_ls(const char *args) {
    add_line("gallery.html");
    add_line("random.html");
    add_line("shitpost.html");
    add_line("about.html");
    add_line("info.txt");
    add_line("manifesto.txt");
    add_line("welcome.txt");
}

void cmd_sudo(const char *args) {
    if (root_active) {
        add_line("You are already root.");
    } else {
        awaiting_sudo_password = 1;
        input_len = 0;
        input[0] = '\0';
    }
}

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

/* ---------- Terminal helpers ---------- */

void add_line(const char *text) {

	add_line_with_font(text, NULL,0);
	
	/*
    if (line_count >= MAX_LINES) {
        memmove(terminal, terminal + 1, sizeof(terminal) - sizeof(terminal[0]));
        line_count--;
    }
    strncpy(terminal[line_count++], text, MAX_LINE_LENGTH - 1);

    // Auto-scroll if already at bottom
    int input_height = 40;
	int line_height = settings.line_height;
    int max_visible_lines = (HEIGHT - input_height) / line_height;

    if (scroll_offset + max_visible_lines >= line_count - 1) {
        scroll_offset = line_count - max_visible_lines;
        if (scroll_offset < 0) scroll_offset = 0;
    }
    */
}

void clear_terminal() {
    line_count = 0;
    scroll_offset_px = 0;  // reset pixel-based scroll
}


void execute_command(const char *cmd) {
    if (!cmd || strlen(cmd) == 0)
        return;

    char command[MAX_LINE_LENGTH];
    char *args = NULL;

    strncpy(command, cmd, MAX_LINE_LENGTH - 1);
    command[MAX_LINE_LENGTH - 1] = '\0';

    args = strchr(command, ' ');
    if (args) {
        *args = '\0';
        args++;
    }

    for (int i = 0; i < command_count; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            commands[i].func(args);
            return;
        }
    }

    add_line("Unknown command. Type 'help'");
}


void submit_input() {
	if (awaiting_sudo_password) {
		printf("DEBUG: Checking password: '%s'\n", input); // C-side debug

	if (strcmp(input, ROOT_PASSWORD) == 0) {
		root_active = 1;
		
		#ifdef __EMSCRIPTEN__
		emscripten_run_script("sessionStorage.setItem('rekav_root_session','true'); console.log('Root session set via emscripten_run_script');");
		#endif
		
		push_base64_to_storage(data_encoded, "rekav_data");
		const char *articles[] = { article_0, article_1, article_2 };
		const char *keys[]     = { "rekav_article_0", "rekav_article_1", "rekav_article_2" };
		int article_count = 3;

		for (int i = 0; i < article_count; i++) {
			push_base64_to_storage(articles[i], keys[i]);
		}

		add_line("Root access granted.");

	} else {
		add_line("sudo: Access denied.");
	}
	
		// Reset password state
		awaiting_sudo_password = 0;
		input_len = 0;
		input[0] = '\0';
		return;
	}


    // Normal command mode
    char line[MAX_LINE_LENGTH + 3];
    snprintf(line, sizeof(line), "> %s", input);
    add_line(line);

    execute_command(input);
    
    // --- Push input to history ---
	if (input_len > 0) {
		// Copy command
		char *cmd_copy = strdup(input);
		
		if (history_count == MAX_HISTORY) {
		    // Buffer full, remove oldest
		    free(history[0]);
		    memmove(&history[0], &history[1], sizeof(char*) * (MAX_HISTORY - 1));
		    history[MAX_HISTORY - 1] = cmd_copy;
		} else {
		    history[history_count++] = cmd_copy;
		}

		// Reset history browsing
		history_pos = -1;

	#ifdef __EMSCRIPTEN__
		// Update sessionStorage
		EM_ASM({
		    if (!Module.history_array) Module.history_array = [];
		    let cmd = UTF8ToString($0);
		    Module.history_array.push(cmd);
		    if (Module.history_array.length > 64) Module.history_array.shift();
		    sessionStorage.setItem('rekav_history', JSON.stringify(Module.history_array));
		}, cmd_copy);
	#endif
	}

    input_len = 0;
    input[0] = '\0';
}


/* ---------- Rendering ---------- */

void draw_text(int x, int y, const char *text, TTF_Font *override_font) {
    TTF_Font *f = override_font ? override_font : settings.font;
    SDL_Color color = settings.font_color;

    SDL_Surface *surface = TTF_RenderUTF8_Blended(f, text, color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


void render() {
    // Draw background
	if (settings.background_texture) {
		SDL_RenderCopy(renderer, settings.background_texture, NULL, NULL);
	} else {
		SDL_SetRenderDrawColor(
		    renderer,
		    settings.background.r,
		    settings.background.g,
		    settings.background.b,
		    255
		);
		SDL_RenderClear(renderer);
	}

	int y = 10 - scroll_offset_px; // start drawing relative to pixel scroll
	int input_height = 40;

	// Draw terminal lines until we reach the bottom
	for (int i = 0; i < line_count; i++) {
		if (y + line_heights[i] < 10) { 
		    // line is above the visible area
		    y += line_heights[i];
		    continue;
		}
		if (y > HEIGHT - input_height) break; 
		    // line is below the visible area
		draw_text(10, y, terminal[i], line_fonts[i]);
		y += line_heights[i];
	}

    // ---------- Draw input prompt ----------
    char display[MAX_LINE_LENGTH + 2]; // +1 for cursor, +1 for '\0'

    if (awaiting_sudo_password) {
        // Mask input for password
        for (int i = 0; i < input_len; i++) {
            display[i] = '*';
        }
        display[input_len] = '\0';
    } else {
        // Normal input
        strcpy(display, input);
    }

    int len = strlen(display);

    // Clamp cursor_pos
    if (cursor_pos < 0) cursor_pos = 0;
    if (cursor_pos > len) cursor_pos = len;

    // Insert cursor underscore '_'
    memmove(display + cursor_pos + 1, display + cursor_pos, len - cursor_pos + 1); // shift tail including '\0'
    display[cursor_pos] = '_';

    char prompt_line[MAX_LINE_LENGTH + 32];
    if (awaiting_sudo_password){
        snprintf(prompt_line, sizeof(prompt_line), "> password: %s", display);
    }else{
        snprintf(prompt_line, sizeof(prompt_line), "> %s", display);
    }


    draw_text(10, HEIGHT - input_height + 10, prompt_line, settings.font);

    SDL_RenderPresent(renderer);
}


static int last_fullscreen_state = -1;

/* ---------- Main loop ---------- */
void main_loop() {


    SDL_Event e;
    
	#ifdef __EMSCRIPTEN__
	EM_ASM({
		try {
		    let canvas = Module['canvas'];
		    Module.isFullscreen = (document.fullscreenElement === canvas);
		} catch(e) {
		    console.error(e);
		}
	});
	#endif


    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            running = 0;

        // Text input for normal typing
		if (e.type == SDL_TEXTINPUT) {
			int len = strlen(e.text.text);
			if (input_len + len < MAX_LINE_LENGTH) {
				// Move tail to make room
				memmove(input + cursor_pos + len, input + cursor_pos, input_len - cursor_pos + 1); // +1 for '\0'
				memcpy(input + cursor_pos, e.text.text, len);
				input_len += len;
				cursor_pos += len;
			}
		}


        if (e.type == SDL_KEYDOWN) {
            // Backspace handling
			if (e.key.keysym.sym == SDLK_BACKSPACE && cursor_pos > 0) {
				memmove(input + cursor_pos - 1, input + cursor_pos, input_len - cursor_pos + 1); // +1 for '\0'
				cursor_pos--;
				input_len--;
			}
			else if (e.key.keysym.sym == SDLK_DELETE && cursor_pos < input_len) {
				memmove(input + cursor_pos, input + cursor_pos + 1, input_len - cursor_pos);
				input_len--;
			}
            // Enter submits input
            else if (e.key.keysym.sym == SDLK_RETURN) {
                submit_input();  // <-- submit_input() now handles sudo/password logic
            }
            // Scroll terminal lines
			else if (e.key.keysym.sym == SDLK_UP) {
				if (history_count > 0) {
					if (history_pos == -1) 
						history_pos = history_count - 1;
					else if (history_pos > 0) 
						history_pos--;

					strncpy(input, history[history_pos], MAX_LINE_LENGTH);
					input_len = strlen(input);       // length of current command
					input[input_len] = '\0';
					cursor_pos = input_len;          // move cursor to end
				}
			}
			else if (e.key.keysym.sym == SDLK_DOWN) {
				if (history_count > 0 && history_pos != -1) {
					if (history_pos < history_count - 1) 
						history_pos++;
					else {
						history_pos = -1;
						input[0] = '\0';
						input_len = 0;
						cursor_pos = 0;             // reset cursor
						return;
					}

					strncpy(input, history[history_pos], MAX_LINE_LENGTH);
					input_len = strlen(input);      // length of current command
					input[input_len] = '\0';
					cursor_pos = input_len;         // move cursor to end
				}
			}




            else if (e.key.keysym.sym == SDLK_LEFT && cursor_pos > 0) {
				cursor_pos--;
			}
			else if (e.key.keysym.sym == SDLK_RIGHT && cursor_pos < input_len) {
				cursor_pos++;
			}

        }

		if (e.type == SDL_MOUSEWHEEL) {
			int scroll_pixels = 0;
			int acc = 0;
			int i;
			// Convert 3 lines per tick to pixel height
			int lines_to_scroll = 3 * e.wheel.y; // positive = up, negative = down
			if (lines_to_scroll > 0) { // scroll up
				for (i = 0; i < line_count && i < lines_to_scroll; i++)
				    scroll_pixels += line_heights[i];
				scroll_offset_px -= scroll_pixels;
				if (scroll_offset_px < 0) scroll_offset_px = 0;
			} else if (lines_to_scroll < 0) { // scroll down
				lines_to_scroll = -lines_to_scroll;
				for (i = 0; i < line_count && i < lines_to_scroll; i++)
				    scroll_pixels += line_heights[i];
				scroll_offset_px += scroll_pixels;
				// clamp to max scroll
				int total_height = 0;
				for (i = 0; i < line_count; i++) total_height += line_heights[i];
				int max_scroll = total_height - (HEIGHT - 40);
				if (scroll_offset_px > max_scroll) scroll_offset_px = max_scroll;
			}
		}

    }
    

	poll_translate_result(); // prints automatically if translation arrived
	poll_forecast_result();
	poll_image_result();
	
	#ifdef __EMSCRIPTEN__
		int is_fullscreen = EM_ASM_INT({
		    return Module.isFullscreen ? 1 : 0;
		});

		if (is_fullscreen != last_fullscreen_state) {
		    last_fullscreen_state = is_fullscreen;

		    if (is_fullscreen) {
		        set_font_size(11);
		    } else {
		        set_font_size(15);
		    }
		}
	#endif



    render();
}


/* ---------- Init ---------- */

int main() {

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    init_settings();
    
    EM_ASM({
		const canvas = Module.canvas;
		const dpr = window.devicePixelRatio || 1;

		const width  = canvas.clientWidth  * dpr;
		const height = canvas.clientHeight * dpr;

		if (canvas.width !== width || canvas.height !== height) {
		    canvas.width  = width;
		    canvas.height = height;
		}
	});


    window = SDL_CreateWindow(
        "Rekav - 2026",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	int rw, rh;
	SDL_GetRendererOutputSize(renderer, &rw, &rh);
	SDL_RenderSetLogicalSize(renderer, rw, rh);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    font = TTF_OpenFont("font.ttf", 15); // Bigger font
    if (!font) {
        printf("Failed to load font\n");
        return 1;
    }
    
	font_8 = TTF_OpenFont("font.ttf", 8);
	if (!font_8) {
		printf("Failed to load small font size 8: %s\n", TTF_GetError());
		// Optional: fallback to main font or error handling
		// font_8 = font;  // use main font as fallback
	}
	
	font_6 = TTF_OpenFont("font.ttf", 6);
	if (!font_6) {
		printf("Failed to load small font size 6: %s\n", TTF_GetError());
		// Optional: fallback to main font or error handling
		// font_8 = font;  // use main font as fallback
	}
	
    TTF_SetFontStyle(font, TTF_STYLE_BOLD); // Bold font

    SDL_StartTextInput();
    cmd_cat("welcome.txt"); // prints banner at startup

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}
