#include "global.h"
#include "time.h"
#include <stdlib.h> // for rand()

// === Global variables ====================================================
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
PixelGrid grid = {0};

// Your 5×7 font (unchanged)
static const uint8_t font_5x7[12][7] = {
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
    {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x11, 0x0E}, // 9
    {0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00}, // :
    {0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10}  // /
};

// Drawing functions (unchanged)
void draw_char(int grid_x, int grid_y, int value, uint32_t color)
{
    if (value < 0 || value > 11) return;
    const uint8_t *pattern = font_5x7[value];
    for (int row = 0; row < CHAR_HEIGHT; row++) {
        uint8_t bits = pattern[row];
        for (int col = 0; col < CHAR_WIDTH; col++) {
            int bit_pos = 4 - col;
            if (bits & (1 << bit_pos)) {
                int tx = grid_x + col;
                int ty = grid_y + row;
                if (tx >= 0 && tx < GRID_COLS && ty >= 0 && ty < GRID_ROWS) {
                    grid.cells[ty][tx] = color;
                }
            }
        }
    }
}

void draw_digit(int grid_x, int grid_y, int digit, uint32_t color) {
    if (digit >= 0 && digit <= 9) {
        draw_char(grid_x, grid_y, digit, color);
    }
}

void draw_two_digits(int grid_x, int grid_y, int value, uint32_t color) {
    int tens = value / 10;
    int units = value % 10;
    draw_digit(grid_x, grid_y, tens, color);
    draw_digit(grid_x + CHAR_WIDTH + 1, grid_y, units, color);
}

// === Retro effect helpers (flicker removed) ==============================
void apply_scanlines(void) {
    for (int y = 0; y < GRID_ROWS; y++) {
        if (y % 3 != 0) continue; // every 3rd row – subtle
        for (int x = 0; x < GRID_COLS; x++) {
            uint32_t c = grid.cells[y][x];
            if (c == COLOR_BG) continue;
            int r = ((c >> 16) & 0xFF) * 180 / 255; // ~70%
            int g = ((c >> 8)  & 0xFF) * 180 / 255;
            int b = ( c        & 0xFF) * 180 / 255;
            grid.cells[y][x] = (r<<16) | (g<<8) | b | 0xFF000000;
        }
    }
}

void add_light_noise(void) {
    #define NOISE_STRENGTH 8
    for (int y = 0; y < GRID_ROWS; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            if (grid.cells[y][x] == COLOR_BG) continue;
            int n = (rand() % (NOISE_STRENGTH*2 + 1)) - NOISE_STRENGTH;
            uint32_t c = grid.cells[y][x];
            int r = ((c >> 16) & 0xFF) + n;
            int g = ((c >> 8)  & 0xFF) + n;
            int b = ( c        & 0xFF) + n;
            r = (r<0)?0:(r>255?255:r);
            g = (g<0)?0:(g>255?255:g);
            b = (b<0)?0:(b>255?255:b);
            grid.cells[y][x] = (r<<16)|(g<<8)|b|0xFF000000;
        }
    }
}

// === SDL & grid functions ================================================
bool init_sdl(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    float scale = 0.5f;
    int new_w = (int)(WINDOW_W * scale + 0.5f);
    int new_h = (int)(WINDOW_H * scale + 0.5f);
    window = SDL_CreateWindow(
        "Retro Pixel Clock",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        new_w, new_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    SDL_RenderSetScale(renderer, scale, scale);
    SDL_SetRenderDrawColor(renderer, 46, 13, 58, 0);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    return true;
}

void cleanup_sdl(void)
{
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void clear_grid(uint32_t color)
{
    for (int y = 0; y < GRID_ROWS; y++)
        for (int x = 0; x < GRID_COLS; x++)
            grid.cells[y][x] = color;
}

void draw_grid_to_renderer(void)
{
    SDL_SetRenderDrawColor(renderer, 46, 13, 58, 0); // here to fix
    SDL_RenderClear(renderer);
    for (int y = 0; y < GRID_ROWS; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            uint32_t color = grid.cells[y][x];
            if (color == COLOR_BG || (color & 0xFF000000) == 0) continue;
            int r = (color >> 16) & 0xFF;
            int g = (color >> 8) & 0xFF;
            int b = (color >> 0) & 0xFF;
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            int screen_x = x * (CELL_SIZE + 1);
            int screen_y = y * (CELL_SIZE + 1);
            SDL_Rect rect = {screen_x, screen_y, CELL_SIZE, CELL_SIZE};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_RenderPresent(renderer);
}

// === Main loop – flicker removed =========================================
void main_loop(void)
{
    clear_grid(COLOR_BG);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (!t) return;

    int h = t->tm_hour;
    int m = t->tm_min;
    int s = t->tm_sec;
    int day   = t->tm_mday;
    int month = t->tm_mon + 1;
    int year  = t->tm_year + 1900;

    // Use stable color (no per-frame random variation)
    uint32_t color = COLOR_ON;

    // ──────────────────────── TIME ────────────────────────
    int time_y = 5;
    int step = 6;
    int time_start_x = 10;
    draw_two_digits(time_start_x + 0*step, time_y, h, color);
    draw_char(time_start_x + 2*step, time_y, 10, color); // :
    draw_two_digits(time_start_x + 3*step, time_y, m, color);
    draw_char(time_start_x + 5*step, time_y, 10, color); // :
    draw_two_digits(time_start_x + 6*step, time_y, s, color);

    // ──────────────────────── DATE ────────────────────────
    int date_y = 13;
    int date_start_x = 4;
    draw_two_digits(date_start_x + 0*step, date_y, day, color);
    draw_char(date_start_x + 2*step, date_y, 11, color); // /
    draw_two_digits(date_start_x + 3*step, date_y, month, color);
    draw_char(date_start_x + 5*step, date_y, 11, color); // /
    draw_two_digits(date_start_x + 6*step, date_y, year / 100, color);
    draw_two_digits(date_start_x + 8*step, date_y, year % 100, color);

    // Apply retro effects (still gives character without flicker)
    apply_scanlines();
    add_light_noise();

    draw_grid_to_renderer();
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (!init_sdl()) {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        main_loop();
        SDL_Delay(256);           // ~4 fps – keeps CPU calm
    }
#endif

    cleanup_sdl();
    return 0;
}
