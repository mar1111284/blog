#include <string.h>
#include <stdio.h>
#include "global.h"
#include "settings.h"

TerminalSettings settings;

const NamedColor predefined_colors[] = {
    // Standard readable colors
    { "white",       {255, 255, 255, 255} },
    { "black",       {0,   0,   0,   255} },
    { "red",         {255, 0,   0,   255} },
    { "green",       {0,   255, 0,   255} },
    { "blue",        {0,   0,   255, 255} },
    { "yellow",      {255, 255, 0, 255} },
    { "cyan",        {0, 255, 255, 255} },
    { "magenta",     {255, 0, 255, 255} },

    // Terminal & thematic colors
    { "pink",        {255, 20, 147, 255} },
    { "amber",       {255, 191, 0, 255} },
    { "charcoal",    {18, 18, 18, 255} },
    { "dos_blue",    {0, 0, 128, 255} },
    { "lime",        {50, 205, 50, 255} },
    { "orange",      {255, 140, 0, 255} },
    { "violet",      {138, 43, 226, 255} },
    { "turquoise",   {64, 224, 208, 255} },
    { "hot_pink",    {255, 105, 180, 255} },
    { "salmon",      {250, 128, 114, 255} },
    { "light_gray",  {211, 211, 211, 255} },
    { "dark_gray",   {105, 105, 105, 255} },

    // Fun / ironic / themed colors
    { "neon_green",  {57, 255, 20, 255} },
    { "bubblegum",   {255, 182, 193, 255} },
    { "candy_pink",  {255, 105, 180, 255} },
    { "fire_red",    {178, 34, 34, 255} },
    { "deep_purple", {75, 0, 130, 255} },
};


static const Theme predefined_themes[] = {
    {
        "default",
        {18, 18, 18, 255},
        NULL,
        {255, 255, 255, 255},  // font color
        {0, 0, 0, 0},          // line bg color
        16,                    // font size
        19,                    // line height
        0                      // letter spacing
    },
    {
        "msdos",
        {0, 0, 128, 255},
        NULL,
        {255, 255, 255, 255},
        {0, 0, 0, 0},
        16,
        18,
        0
    },
    {
        "barbie",
        {0, 0, 0, 255},
        "assets/bg_barbie.png",
        {255, 255, 147, 255},
        {0, 0, 0, 0},
        16,
        20,
        0
    },
    {
        "jurassic",
        {0, 0, 0, 255},
        "assets/bg_jurassic.png",
        {255, 140, 20, 255},
        {0, 0, 0, 0},
        16,
        20,
        0
    },
    {
        "inferno",
        {30, 0, 0, 255},
        NULL,
        {255, 140, 0, 255},
        {0, 0, 0, 0},
        16,
        20,
        0
    },
    {
        "mobile",
        {30, 0, 0, 255},
        NULL,
        {255, 140, 0, 255},
        {0, 0, 0, 0},
        13,
        15,
        0
    },
    {
        "neon_night",
        {5, 5, 20, 255},
        NULL,
        {57, 255, 20, 255},
        {0, 0, 0, 0},
        16,
        18,
        0
    },
    {
        "bubblegum",
        {255, 182, 193, 255},
        NULL,
        {138, 43, 226, 255},
        {0, 0, 0, 0},
        16,
        20,
        0
    },
    {
        "retro_hacker",
        {0, 0, 0, 255},
        NULL,
        {0, 255, 0, 255},
        {0, 0, 0, 0},
        14,
        18,
        0
    },
    {
        "sunset_trash",
        {25, 0, 0, 255},
        NULL,
        {255, 105, 180, 255},
        {0, 0, 0, 0},
        16,
        19,
        0
    },
    {
        "toxic_cyber",
        {10, 10, 10, 255},
        NULL,
        {0, 255, 255, 255},
        {0, 0, 0, 0},
        16,
        20,
        0
    }
};

static const int PREDEFINED_THEME_COUNT = sizeof(predefined_themes) / sizeof(predefined_themes[0]);

const Theme *get_theme_info(const char *name) {
    if (!name) return NULL;

    for (int i = 0; i < PREDEFINED_THEME_COUNT; i++) {
        if (strcmp(predefined_themes[i].name, name) == 0)
            return &predefined_themes[i];
    }
    return NULL;
}

SDL_Texture* load_background(const char *path) {
    if (!path) return NULL;

    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        printf("Failed to load background: %s\n", path);
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(_sdl.renderer, surface);
    SDL_FreeSurface(surface);

    // Optionally store in settings if you want a "current" background
    _terminal.settings.background_texture = texture;

    return texture;
}

int apply_theme(TerminalSettings *s, const char *theme_name) {
    if (!s || !theme_name) return 0;

    for (int i = 0; i < sizeof(predefined_themes)/sizeof(predefined_themes[0]); i++) {
        if (strcmp(predefined_themes[i].name, theme_name) == 0) {
            // --- Copy theme properties ---
            s->background           = predefined_themes[i].background;
            s->font_color           = predefined_themes[i].font_color;
            s->line_background_color= predefined_themes[i].line_background_color;
            s->font_size            = predefined_themes[i].font_size;
            s->line_height          = predefined_themes[i].line_height;
            s->letter_spacing       = predefined_themes[i].letter_spacing;
            s->theme_name           = predefined_themes[i].name;

            // --- Background texture ---
            if (s->background_texture) {
                SDL_DestroyTexture(s->background_texture);
                s->background_texture = NULL;
            }

            if (predefined_themes[i].background_image) {
                SDL_Surface *surface = IMG_Load(predefined_themes[i].background_image);
                if (surface) {
                    s->background_texture = SDL_CreateTextureFromSurface(_sdl.renderer, surface);
                    SDL_FreeSurface(surface);
                } else {
                    printf("apply_theme: failed to load background %s\n", predefined_themes[i].background_image);
                }
            }

            // --- Load font ---
            if (s->font) {
                TTF_CloseFont(s->font);
                s->font = NULL;
            }

            s->font = TTF_OpenFont("font.ttf", s->font_size);
            if (s->font) TTF_SetFontStyle(s->font, TTF_STYLE_BOLD);
            else printf("apply_theme: failed to load font.ttf size %d\n", s->font_size);

            return 1; // success
        }
    }

    return 0; // theme not found
}

const NamedColor *find_color(const char *name) {
    for (int i = 0; i < sizeof(predefined_colors) / sizeof(predefined_colors[0]); i++) {
        if (strcmp(predefined_colors[i].name, name) == 0) {
            return &predefined_colors[i];
        }
    }
    return NULL;
}

const Theme *find_theme(const char *name) {
    for (int i = 0; i < sizeof(predefined_themes) / sizeof(predefined_themes[0]); i++) {
        if (strcmp(predefined_themes[i].name, name) == 0) {
            return &predefined_themes[i];
        }
    }
    return NULL;
}

int set_line_height(TerminalSettings *s, int height) {
    if (!s || height < 8 || height > 25) return 0;
    s->line_height = height;
    return 1;
}

void clear_background_texture(TerminalSettings *s) {
    if (!s) return;
    if (s->background_texture) {
        SDL_DestroyTexture(s->background_texture);
        s->background_texture = NULL;
    }
}

int set_background_color(TerminalSettings *s, const char *name) {
    if (!s || !name) return 0;

    for (int i = 0; i < sizeof(predefined_colors)/sizeof(predefined_colors[0]); i++) {
        if (strcmp(predefined_colors[i].name, name) == 0) {
            s->background = predefined_colors[i].color;
            clear_background_texture(s); // remove any existing background image
            return 1;
        }
    }
    return 0;
}

int set_font_color(TerminalSettings *s, const char *name) {
    if (!s || !name) return 0;

    for (int i = 0; i < sizeof(predefined_colors)/sizeof(predefined_colors[0]); i++) {
        if (strcmp(predefined_colors[i].name, name) == 0) {
            s->font_color = predefined_colors[i].color;
            return 1;
        }
    }
    return 0;
}

int set_font_size(TerminalSettings *s, int size) {
    if (!s || size < 7 || size > 20) return 0;

    if (s->font) {
        TTF_CloseFont(s->font);
        s->font = NULL;
    }

    s->font = TTF_OpenFont("font.ttf", size);
    if (!s->font) return 0;

    TTF_SetFontStyle(s->font, TTF_STYLE_BOLD);
    s->font_size = size;

    return 1;
}






