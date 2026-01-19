#include "sdl.h"
#include "global.h"
#include "editor.h"
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif

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
    
    // ensure pixelated canvas in browser (Canvas2D fallback)
	EM_ASM({
		if (Module.canvas) {
		    Module.canvas.style.imageRendering = "pixelated";
		    Module.canvas.style.imageRendering = "-moz-crisp-edges";
		    Module.canvas.style.imageRendering = "-webkit-optimize-contrast";
		    Module.canvas.style.imageRendering = "crisp-edges";
		}
	});

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

void editor_handle_key(SDL_Event *e) {
    if (!_editor.active) return;

    if (e->type == SDL_KEYDOWN) {
        SDL_Keycode key = e->key.keysym.sym;
        /*

        switch (key) {
            case SDLK_LEFT:
                editor_move_cursor(-1, 0);
                break;

            case SDLK_RIGHT:
                editor_move_cursor(1, 0);
                break;

            case SDLK_UP:
                editor_move_cursor(0, -1);
                break;

            case SDLK_DOWN:
                editor_move_cursor(0, 1);
                break;

            case SDLK_BACKSPACE:
                editor_delete_char();
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                editor_newline();
                break;

            case SDLK_PAGEUP:
                editor.scroll_offset -= 5;
                if (editor.scroll_offset < 0) editor.scroll_offset = 0;
                editor.dirty = TRUE;
                break;

            case SDLK_PAGEDOWN:
                editor.scroll_offset += 5;
                if (editor.scroll_offset > editor.line_count - 1)
                    editor.scroll_offset = editor.line_count - 1;
                editor.dirty = TRUE;
                break;

            case SDLK_HOME:
                editor.cursor_x = 0;
                break;

            case SDLK_END:
                editor.cursor_x = strlen(editor.lines[editor.cursor_y]);
                break;

            case SDLK_ESCAPE:
                editor_close();
                break;
        }
        
        */
    }

    if (e->type == SDL_TEXTINPUT) {
        //editor_insert_char(e->text.text[0]);  // simple for now: single char
    }
}

