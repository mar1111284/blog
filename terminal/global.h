#ifndef GLOBAL_H
#define GLOBAL_H

#include "settings.h"
#include "sdl.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>

// GENERAL / APP INFO
#define ZERO_MEMORY           0
#define APP_NAME             "Rekav terminal"
#define FONT_PATH            "font.ttf"

// TERMINAL DIMENSIONS
#define DESKTOP_WIDTH        800
#define MIN_DESKTOP_WIDTH    490
#define DESKTOP_HEIGHT       400

// TERMINAL PADDING
#define TERMINAL_PADDING_LEFT    10
#define TERMINAL_PADDING_RIGHT   20
#define TERMINAL_PADDING_TOP     10
#define TERMINAL_PADDING_BOTTOM  10
#define WRAP_ADJUSTMENT          70

// TERMINAL TEXT
#define CURSOR_CHAR            "_"
#define PASSWORD_CHAR          "*"
#define DEFAULT_PROMPT        "user@rekav:~$ "

// TERMINAL SCROLL
#define SCROLL_DELTA_MULTIPLIER 32

// INPUT / HISTORY
#define MAX_LINES             256
#define MAX_LINE_LENGTH       512
#define MAX_HISTORY_COMMANDS  128
#define INPUT_MAX_CHARS       256
#define INPUT_MAX_VISIBLE     (INPUT_MAX_CHARS - 1)

// FILES / EXPORT
#define MAX_PNG_SIZE  (50 * 1024 * 1024)

// SECURITY
#define ROOT_PASSWORD        "rekav"

// SHORTCUT MACROS
#define _terminal           app.terminal
#define _sdl                app.sdl
#define _config             app.config
#define _root_active        app.root_active
#define _awaiting_sudo_password      app.awaiting_sudo_password
#define _translation_pending         app.translation_pending
#define _weather_forecast_pending    app.weather_forecast_pending
#define _image_processing_pending    app.image_processing_pending
#define _image_download_pending      app.image_download_pending
#define _last_activity      app.last_activity
#define _history            app.terminal.history


typedef enum {
    FAST  = 16,
    MEDIUM = 32,
    SLOW   = 64
} RENDER_TICK;

typedef enum {
    XS = 13,
    S  = 15,
    M  = 17,
    L  = 19
} FONT_SIZE;

typedef enum{
	TRUE =1,
	FALSE =0
}BOOL;

typedef struct AppConfig {
    int font_size;
    int viewport_width;
    int viewport_height;
} AppConfig;

typedef enum {
    LINE_FLAG_NONE      = 0,
    LINE_FLAG_PROMPT    = 1 << 0,
    LINE_FLAG_ERROR     = 1 << 1,
    LINE_FLAG_SYSTEM    = 1 << 2,
    LINE_FLAG_HIGHLIGHT = 1 << 3,
    LINE_FLAG_WARNING = 1 << 4
} TerminalLineFlags;

typedef struct TerminalLine {
    char text[MAX_LINE_LENGTH];
    TTF_Font   *font;
    SDL_Color   font_color;
    SDL_Color   background_color;
    int         letter_spacing;
    int         width;     // pixel width of the texture
    int         height;    // pixel height of the texture
    int         line_height;
    SDL_Texture *texture;
    TerminalLineFlags flags;
    BOOL dirty; 
    int cursor_index;   // <-- new: logical cursor for this line
} TerminalLine;

typedef struct Terminal {
    int running;
    int width;
    int height;
    TerminalSettings settings;
    TerminalLine lines[MAX_LINES];
    int line_count;
    int scroll_offset_px;
    int max_scroll;
    BOOL dirty;

    // ── NEW: current input state ───────────────────────────────
    struct {
        char buffer[INPUT_MAX_CHARS];       // includes prompt + user text
        int cursor_pos;                     // byte index (0 = right after prompt)
        int prompt_len;                     // length of current prompt (protected zone)
        SDL_Texture *texture;
        int texture_w;
        int texture_h;
        BOOL dirty;
    } input;

    struct {
        char *commands[MAX_HISTORY_COMMANDS];
        int count;
        int pos;
    } history;
    
} Terminal;

typedef struct {
    // Configuration & window
    AppConfig     config;
    SDLContext    sdl;

    // Core terminal state
    Terminal      terminal;

    // State flags
    int			  is_mobile_view;
    int           fullscreen;
    int           root_active;
    int           awaiting_sudo_password;
    int           translation_pending;
    int           weather_forecast_pending;
    int           image_processing_pending;
    int           image_download_pending;

    // Timing / activity
    Uint32        last_activity;

    // Add future globals here when needed
} AppContext;

extern AppContext app;

// App
void app_init(void);
void app_cleanup(void);
void app_debug_dump(const char *arg);

// Render & update
void render_terminal(void);
void render_text_line(int x, int y, const char *text, TTF_Font *font, SDL_Color *color);
void reset_current_input();
void update_input_texture();

// Terminal helpers
int is_at_bottom(void);
int terminal_content_height(void);
void clear_terminal(void);
void add_terminal_line(const char *text, TerminalLineFlags flags);
void submit_input(void);
void parse_color(const char *name, uint8_t *r, uint8_t *g, uint8_t *b);
void update_max_scroll(void);
#endif

