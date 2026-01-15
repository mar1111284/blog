#include "settings.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string.h>
#include <stdio.h>

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
        {255, 255, 255, 255},
        16,
        19
    },
    {
        "msdos",
        {0, 0, 128, 255},
        NULL,
        {255, 255, 255, 255},
        16,
        18
    },
    {
        "barbie",
        {0, 0, 0, 255}, // unused when image is present
        "assets/bg_barbie.png",
        {255, 255, 147, 255},
        16,
        20
    },
    {
        "jurassic",
        {0, 0, 0, 255},
        "assets/bg_jurassic.png",
        {255, 140, 20, 255},
        16,
        20
    },
    {
        "inferno",
        {30, 0, 0, 255},
        NULL,
        {255, 140, 0, 255},
        16,
        20
    },
    {
        "neon_night",
        {5, 5, 20, 255},
        NULL,
        {57, 255, 20, 255},
        16,
        18
    },
    {
        "bubblegum",
        {255, 182, 193, 255},
        NULL,
        {138, 43, 226, 255},
        16,
        20
    },
    {
        "retro_hacker",
        {0, 0, 0, 255},
        NULL,
        {0, 255, 0, 255},
        14,
        18
    },
    {
        "sunset_trash",
        {25, 0, 0, 255},
        NULL,
        {255, 105, 180, 255},
        16,
        19
    },
    {
        "toxic_cyber",
        {10, 10, 10, 255},
        NULL,
        {0, 255, 255, 255},
        16,
        20
    }
};

SDL_Texture *settings_background_texture = NULL;

int set_line_height(int height) {
    if (height < 8 || height > 25) {
        return 0; // invalid
    }

    settings.line_height = height;
    return 1; // success
}


SDL_Texture* load_background(const char *path) {
    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        printf("Failed to load background: %s\n", path);
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    settings.background_texture = texture;
    return texture;
}

void clear_background_texture() {
    if (settings.background_texture) {
        SDL_DestroyTexture(settings.background_texture);
        settings.background_texture = NULL;
    }
}


void init_settings() {
    // Default colors
    settings.background = (SDL_Color){18, 18, 18, 255}; // Charcoal
    settings.font_color = (SDL_Color){255, 255, 255, 255}; // White
    settings.font_size = 16;
    settings.line_height = 15;
    settings.theme_name = "Default";
    settings.font = TTF_OpenFont("font.ttf", settings.font_size);
    if (!settings.font) {
        printf("Failed to load font.ttf in init_settings\n");
    } else {
        TTF_SetFontStyle(settings.font, TTF_STYLE_BOLD);
    }
    settings.background_texture = NULL;
}


int apply_theme(const char *name) {
    for (int i = 0; i < sizeof(predefined_themes)/sizeof(predefined_themes[0]); i++) {
        if (strcmp(predefined_themes[i].name, name) == 0) {
            // Copy colors and sizes
            settings.background = predefined_themes[i].background;
            settings.font_color = predefined_themes[i].font_color;
            settings.font_size = predefined_themes[i].font_size;
            settings.line_height = predefined_themes[i].line_height;
            settings.theme_name = predefined_themes[i].name;

            // Load background image if present
            if (predefined_themes[i].background_image) {
                // Free previous texture if exists
                if (settings.background_texture) {
                    SDL_DestroyTexture(settings.background_texture);
                    settings.background_texture = NULL;
                }
                SDL_Surface *surface = IMG_Load(predefined_themes[i].background_image);
                if (surface) {
                    settings.background_texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_FreeSurface(surface);
                }
            } else {
                // No image for this theme
                if (settings.background_texture) {
                    SDL_DestroyTexture(settings.background_texture);
                    settings.background_texture = NULL;
                }
            }

            // Reload font for this theme size
            set_font_size(settings.font_size);
            set_line_height(settings.line_height);

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


int set_background_color(const char *name) {
    for (int i = 0; i < sizeof(predefined_colors)/sizeof(predefined_colors[0]); i++) {
        if (strcmp(predefined_colors[i].name, name) == 0) {
            settings.background = predefined_colors[i].color;
            return 1; // success
        }
    }
    return 0; // unknown color
}

int set_font_color(const char *name) {
    for (int i = 0; i < sizeof(predefined_colors)/sizeof(predefined_colors[0]); i++) {
        if (strcmp(predefined_colors[i].name, name) == 0) {
            settings.font_color = predefined_colors[i].color;
            return 1;
        }
    }
    return 0;
}

int set_font_size(int size) {
    if (size < 7 || size > 20) {
        return 0; // invalid size
    }

    if (settings.font) {
        TTF_CloseFont(settings.font);
        settings.font = NULL;
    }

    settings.font = TTF_OpenFont("font.ttf", size);
    if (!settings.font) {
        return 0; // failed to load font
    }

    TTF_SetFontStyle(settings.font, TTF_STYLE_BOLD);
    settings.font_size = size;

    return 1; // success
}






