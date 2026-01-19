#ifndef EDITOR_H
#define EDITOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define MAX_CHARS_PER_LINE   66
#define INITIAL_LINES_CAPACITY  256
#define EDITOR_BUFFER_SIZE   (MAX_CHARS_PER_LINE + 16)  // small extra for safety
#define LINE_NUMBER_PADDING   8
#define LINE_NUMBER_MARGIN    12
#define LINE_NUMBER_BG        (SDL_Color){20, 20, 26, 255}
#define LINE_NUMBER_FG        (SDL_Color){120, 120, 140, 255}

typedef struct {
    char        c;
    SDL_Color   fg;
    SDL_Color   bg;
    SDL_Texture *texture;       // NULL = not rendered yet (lazy)
} CharCell;

typedef struct {
    CharCell    chars[EDITOR_BUFFER_SIZE];
    int         length;             // real number of characters
    SDL_Color   bg_color;           // line background (usually same as editor bg)
} Line;

typedef struct Editor {

	int active;
	int init;
	
	
    // Font & metrics
    TTF_Font   *font;
    int         font_size;
    int         line_height;
    int         char_advance;       // monospace width
    int line_number_width;
	int scroll_line;
	int visible_lines;



    // Document
    Line       *lines;
    int         capacity;           // allocated lines
    int         line_count;         // used lines

    // Cursor (0-based indices)
    int         cursor_line;
    int         cursor_col;

    // Visual/input state
    bool        cursor_visible;
    Uint32      cursor_blink_timer;
    Uint32      last_blink_time;

    // Colors
    SDL_Color   default_fg;
    SDL_Color   default_bg;
    SDL_Color   cursor_color;
    
    bool        is_insert_mode;         // true = typing, false = command/normal
    Uint32      last_save_time;         // SDL_GetTicks() of last successful save
    char        filename[256];          // optional - for display
    bool        modified;               // dirty flag - * for modified
    
} Editor;

extern Editor g_editor;   // global editor state

// Public interface
void editor_init(void);
void editor_cleanup(void);

void editor_handle_event(const SDL_Event *event);
void editor_render(void);

// Helpers you will probably need soon
void editor_insert_char(char c);
void editor_backspace(void);
void editor_newline(void);
void editor_move_cursor_left(void);
void editor_move_cursor_right(void);
void editor_move_cursor_up(void);
void editor_move_cursor_down(void);
void editor_update_textures_line(int line_idx);
void clamp_cursor(void);

#endif // EDITOR_H
