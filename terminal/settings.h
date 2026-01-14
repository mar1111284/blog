#ifndef SETTINGS_H
#define SETTINGS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL2/SDL_image.h>

typedef struct {
    SDL_Color background;
    SDL_Color font_color;
    int font_size;     // 10 - 20
    int line_height;   // 15 - 25
    const char *theme_name;
    SDL_Texture *background_texture;
    TTF_Font *font;    // include font here
} TerminalSettings;

typedef struct {
    const char *name;
    SDL_Color color;
} NamedColor;

typedef struct {
    const char *name;

    // background
    SDL_Color background;
    const char *background_image; // NULL if none

    // text
    SDL_Color font_color;
    int font_size;
    int line_height;
} Theme;


// Global settings variable
extern TerminalSettings settings;
extern const NamedColor predefined_colors[];
extern SDL_Renderer *renderer;

// Functions
void init_settings(); // initialize defaults
SDL_Texture *load_background(const char *path);
void clear_background_texture();

int set_background_color(const char *name);
int set_font_color(const char *name);
int set_font_size(int size);
int set_line_height(int height);
int apply_theme(const char *name);
const Theme *find_theme(const char *name);
const NamedColor *find_color(const char *name);




#endif

