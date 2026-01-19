#ifndef SDL_CONTEXT_H
#define SDL_CONTEXT_H

#include <SDL.h>
#include <SDL_ttf.h>

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    int width;
    int height;
} SDLContext;

int init_sdl(SDLContext *ctx, const char *title, int w, int h, int font_size);
void cleanup_sdl(SDLContext *ctx);

// Editor
void editor_handle_key(SDL_Event *e);	

#endif /* SDL_CONTEXT_H */

