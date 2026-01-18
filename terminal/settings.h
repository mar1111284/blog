#ifndef SETTINGS_H
#define SETTINGS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL2/SDL_image.h>

/* ================= THEMES ================= */

#define THEME_DEFAULT        "default"
#define THEME_MSDOS          "msdos"
#define THEME_BARBIE         "barbie"
#define THEME_JURASSIC       "jurassic"
#define THEME_INFERNO        "inferno"
#define THEME_MOBILE         "mobile"
#define THEME_NEON_NIGHT     "neon_night"
#define THEME_BUBBLEGUM      "bubblegum"
#define THEME_RETRO_HACKER   "retro_hacker"
#define THEME_SUNSET_TRASH   "sunset_trash"
#define THEME_TOXIC_CYBER    "toxic_cyber"

/* ================= COLORS ================= */

#define COLOR_WHITE          "white"
#define COLOR_BLACK          "black"
#define COLOR_RED            "red"
#define COLOR_GREEN          "green"
#define COLOR_BLUE           "blue"
#define COLOR_YELLOW         "yellow"
#define COLOR_CYAN           "cyan"
#define COLOR_MAGENTA        "magenta"

#define COLOR_PINK           "pink"
#define COLOR_AMBER          "amber"
#define COLOR_CHARCOAL       "charcoal"
#define COLOR_DOS_BLUE       "dos_blue"
#define COLOR_LIME           "lime"
#define COLOR_ORANGE         "orange"
#define COLOR_VIOLET         "violet"
#define COLOR_TURQUOISE      "turquoise"
#define COLOR_HOT_PINK       "hot_pink"
#define COLOR_SALMON         "salmon"
#define COLOR_LIGHT_GRAY     "light_gray"
#define COLOR_DARK_GRAY      "dark_gray"

#define COLOR_NEON_GREEN     "neon_green"
#define COLOR_BUBBLEGUM      "bubblegum"
#define COLOR_CANDY_PINK     "candy_pink"
#define COLOR_FIRE_RED       "fire_red"
#define COLOR_DEEP_PURPLE    "deep_purple"

/* ================= COLORS (RGB) ================= */

#define COLOR_WHITE_RGB          ((SDL_Color){255, 255, 255, 255})
#define COLOR_BLACK_RGB          ((SDL_Color){0,   0,   0,   255})
#define COLOR_RED_RGB            ((SDL_Color){255, 0,   0,   255})
#define COLOR_GREEN_RGB          ((SDL_Color){0,   255, 0,   255})
#define COLOR_BLUE_RGB           ((SDL_Color){0,   0,   255, 255})
#define COLOR_YELLOW_RGB         ((SDL_Color){255, 255, 0,   255})
#define COLOR_CYAN_RGB           ((SDL_Color){0,   255, 255, 255})
#define COLOR_MAGENTA_RGB        ((SDL_Color){255, 0,   255, 255})

#define COLOR_PINK_RGB           ((SDL_Color){255, 20,  147, 255})
#define COLOR_AMBER_RGB          ((SDL_Color){255, 191, 0,   255})
#define COLOR_CHARCOAL_RGB       ((SDL_Color){18,  18,  18,  255})
#define COLOR_DOS_BLUE_RGB       ((SDL_Color){0,   0,   128, 255})
#define COLOR_LIME_RGB           ((SDL_Color){50,  205, 50,  255})
#define COLOR_ORANGE_RGB         ((SDL_Color){255, 140, 0,   255})
#define COLOR_VIOLET_RGB         ((SDL_Color){138, 43,  226, 255})
#define COLOR_TURQUOISE_RGB      ((SDL_Color){64,  224, 208, 255})
#define COLOR_HOT_PINK_RGB       ((SDL_Color){255, 105, 180, 255})
#define COLOR_SALMON_RGB         ((SDL_Color){250, 128, 114, 255})
#define COLOR_LIGHT_GRAY_RGB     ((SDL_Color){211, 211, 211, 255})
#define COLOR_DARK_GRAY_RGB      ((SDL_Color){105, 105, 105, 255})

#define COLOR_NEON_GREEN_RGB     ((SDL_Color){57,  255, 20,  255})
#define COLOR_BUBBLEGUM_RGB      ((SDL_Color){255, 182, 193, 255})
#define COLOR_CANDY_PINK_RGB     ((SDL_Color){255, 105, 180, 255})
#define COLOR_FIRE_RED_RGB       ((SDL_Color){178, 34,  34,  255})
#define COLOR_DEEP_PURPLE_RGB    ((SDL_Color){75,  0,   130, 255})

typedef struct {
    // terminal-wide visuals
    SDL_Color   background;
    SDL_Texture *background_texture;
    const char *theme_name;

    // default text appearance (for new lines)
    SDL_Color   font_color;
    SDL_Color   line_background_color;
    int         font_size;        // e.g. 10–20
    int         line_height;      // vertical padding / multiplier
    int         letter_spacing;

    // default font
    TTF_Font   *font;
} TerminalSettings;

typedef struct {
    const char *name;
    SDL_Color color;
} NamedColor;

typedef struct {
    const char *name;
    SDL_Color background;
    const char *background_image; // NULL if none
    SDL_Color font_color;
    SDL_Color line_background_color;  // optional line bg for highlight or prompt
    int font_size;                     // pixels, e.g. 10–20
    int line_height;                   // vertical spacing / line height
    int letter_spacing;                // spacing between characters
} Theme;

// Global settings variable
extern const NamedColor predefined_colors[];

SDL_Texture* load_background(const char *path);
void clear_background_texture(TerminalSettings *s);

int set_background_color(TerminalSettings *s, const char *name);
int set_font_color(TerminalSettings *s, const char *name);
int set_font_size(TerminalSettings *s, int size);
int set_line_height(TerminalSettings *s, int height);
int apply_theme(TerminalSettings *s, const char *theme_name);
const Theme *find_theme(const char *name);
const Theme *get_theme_info(const char *theme_name);
const NamedColor *find_color(const char *name);

void list_themes(void);
void list_colors(void);
void print_settings_help(void);




#endif

