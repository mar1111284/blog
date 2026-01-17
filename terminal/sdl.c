#include "sdl.h"
#include "global.h"


int init_sdl(SDLContext *ctx, const char *title, int w, int h, int font_size) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    if (TTF_Init() != 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }

    ctx->width  = w;
    ctx->height = h;

    ctx->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_SHOWN
    );
    if (!ctx->window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx->renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    int rw, rh;
    SDL_GetRendererOutputSize(ctx->renderer, &rw, &rh);
    SDL_RenderSetLogicalSize(ctx->renderer, rw, rh);

    ctx->font = TTF_OpenFont("font.ttf", font_size);
    if (!ctx->font) {
        printf("TTF_OpenFont failed\n");
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    TTF_SetFontStyle(ctx->font, TTF_STYLE_BOLD);
    SDL_StartTextInput();

    return 1;
}

void cleanup_sdl(SDLContext *ctx) {
    if (ctx->font) TTF_CloseFont(ctx->font);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);

    TTF_Quit();
    SDL_Quit();
}
