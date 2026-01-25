#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif

// === Configuration =======================================================

#define WINDOW_W        334
#define WINDOW_H        124

#define CELL_SIZE       4
#define GRID_COLS       67
#define GRID_ROWS       25

#define CHAR_WIDTH      5
#define CHAR_HEIGHT     7

#define COLOR_BG    0x2e0d3a     // RGB(21,21,21) bg

#define COLOR_ON    0xCCCCCC
#define COLOR_BLUE      0xFF0066FF

// === Globals =============================================================

extern SDL_Window   *window;
extern SDL_Renderer *renderer;

typedef struct {
    uint32_t cells[GRID_ROWS][GRID_COLS];
} PixelGrid;

extern PixelGrid grid;

// === Functions ===========================================================

bool init_sdl(void);
void cleanup_sdl(void);
void clear_grid(uint32_t color);
void draw_grid_to_renderer(void);
void draw_char(int grid_x, int grid_y, int value, uint32_t color);

#endif
