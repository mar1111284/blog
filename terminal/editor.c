#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include "global.h"

Editor g_editor = {0};

static const SDL_Color COLOR_DEFAULT_FG   = {235, 235, 240, 255};
static const SDL_Color COLOR_DEFAULT_BG   = { 28,  28,  34, 255};
static const SDL_Color COLOR_CURSOR       = {180, 180, 255, 200};
static const SDL_Color COLOR_PREPROC      = {180, 140, 255, 255};  // purple-ish
static const SDL_Color COLOR_KEYWORD      = { 86, 156, 214, 255};  // blue-ish
static const SDL_Color COLOR_COMMENT      = {106, 153,  85, 255};  // green-ish

void editor_scroll_up(void){
    if (g_editor.scroll_line > 0) g_editor.scroll_line--;
}
void editor_scroll_down(void){
    if (g_editor.scroll_line < g_editor.line_count - g_editor.visible_lines) g_editor.scroll_line++;
}

static void clamp_scroll(void)
{
    if (g_editor.scroll_line < 0)
        g_editor.scroll_line = 0;
    
    int max_scroll = g_editor.line_count - g_editor.visible_lines;
    if (max_scroll < 0) max_scroll = 0;
    
    if (g_editor.scroll_line > max_scroll)
        g_editor.scroll_line = max_scroll;
}


void update_scroll(void)
{
    clamp_cursor();
    if (g_editor.cursor_line < g_editor.scroll_line)
    {
        g_editor.scroll_line = g_editor.cursor_line;
    }
    else if (g_editor.cursor_line >= g_editor.scroll_line + g_editor.visible_lines)
    {
        g_editor.scroll_line = g_editor.cursor_line - g_editor.visible_lines + 1;
    }
    clamp_scroll();
}

void clamp_cursor(void)
{
    if (g_editor.cursor_line >= g_editor.line_count)
        g_editor.cursor_line = g_editor.line_count - 1;
    if (g_editor.cursor_line < 0)
        g_editor.cursor_line = 0;

    Line *line = &g_editor.lines[g_editor.cursor_line];
    if (g_editor.cursor_col > line->length)
        g_editor.cursor_col = line->length;
    if (g_editor.cursor_col < 0)
        g_editor.cursor_col = 0;
}

void editor_move_cursor_left(void)
{
    if (g_editor.cursor_col > 0) {
        g_editor.cursor_col--;
    }
    else if (g_editor.cursor_line > 0) {
        g_editor.cursor_line--;
        g_editor.cursor_col = g_editor.lines[g_editor.cursor_line].length;
    }
}

void editor_move_cursor_right(void)
{
    Line *line = &g_editor.lines[g_editor.cursor_line];
    
    if (g_editor.cursor_col < line->length) {
        g_editor.cursor_col++;
    }
    else if (g_editor.cursor_line < g_editor.line_count - 1) {
        g_editor.cursor_line++;
        g_editor.cursor_col = 0;
    }
}
void editor_move_cursor_up(void)
{
    if (g_editor.cursor_line > 0)
    {
        g_editor.cursor_line--;
        Line *line = &g_editor.lines[g_editor.cursor_line];
        if (g_editor.cursor_col > line->length)
            g_editor.cursor_col = line->length;
    }
    update_scroll();   // ← ONLY THIS
}

void editor_move_cursor_down(void)
{
    if (g_editor.cursor_line < g_editor.line_count - 1)
    {
        g_editor.cursor_line++;
        Line *line = &g_editor.lines[g_editor.cursor_line];
        if (g_editor.cursor_col > line->length)
            g_editor.cursor_col = line->length;
    }
    update_scroll();   // ← ONLY THIS
}

static const char* get_time_since_last_save(void)
{
    static char buf[64];
    Uint32 now = SDL_GetTicks();
    Uint32 diff_ms = now - g_editor.last_save_time;
    Uint32 minutes = diff_ms / 60000;

    if (minutes == 0) {
        snprintf(buf, sizeof(buf), "just now");
    } else if (minutes == 1) {
        snprintf(buf, sizeof(buf), "1 minute ago");
    } else {
        snprintf(buf, sizeof(buf), "%u minutes ago", minutes);
    }

    return buf;
}

void editor_init(void)
{
    memset(&g_editor, 0, sizeof(Editor));
    
    g_editor.is_insert_mode = true;           // start in insert mode
    g_editor.last_save_time = SDL_GetTicks(); // pretend we just "opened" file
    g_editor.modified = false;
    g_editor.filename[0] = '\0';
	
	g_editor.active = TRUE;
    g_editor.font_size = _terminal.settings.font_size;
    g_editor.default_fg = _terminal.settings.font_color;
    g_editor.default_bg = _terminal.settings.background;
    g_editor.cursor_color = _terminal.settings.font_color;

    g_editor.line_height = TTF_FontHeight(_terminal.settings.font) + 4;
    
    // Max digits based on capacity (e.g. 9999)
	int max_lines = g_editor.capacity;
	int digits = 1;
	while (max_lines >= 10) {
		max_lines /= 10;
		digits++;
	}

	g_editor.line_number_width =
		LINE_NUMBER_PADDING * 2 +
		digits * g_editor.char_advance;

		
    int minx, maxx, miny, maxy, advance;
    TTF_GlyphMetrics(_sdl.font, 'M', &minx, &maxx, &miny, &maxy, &advance);
    g_editor.char_advance = advance;

    g_editor.capacity = INITIAL_LINES_CAPACITY;
    g_editor.lines = calloc(g_editor.capacity, sizeof(Line));

    // First empty line
    g_editor.line_count = 1;
    g_editor.lines[0].bg_color = COLOR_DEFAULT_BG;

    g_editor.cursor_line = 0;
    g_editor.cursor_col = 0;
    
    // Calculate how many lines fit on screen (excluding status bar)
    int available_height = _sdl.height - (g_editor.line_height * 2 + 12); // 2-row status + padding
    g_editor.visible_lines = available_height / g_editor.line_height;
    if (g_editor.visible_lines < 1) g_editor.visible_lines = 1;

    g_editor.scroll_line = 0;
    g_editor.init = 1;
}

static SDL_Texture* create_char_texture(char c, SDL_Color fg, SDL_Color bg)
{
    if (c == 0 || c == '\n' || c == '\r') return NULL;

    SDL_Surface *surf = TTF_RenderGlyph_Blended(_sdl.font, c, fg);
    if (!surf) return NULL;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(_sdl.renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

static void free_line_textures(Line *line)
{
    for (int i = 0; i < line->length; i++) {
        if (line->chars[i].texture) {
            SDL_DestroyTexture(line->chars[i].texture);
            line->chars[i].texture = NULL;
        }
    }
}

static void line_clear(Line *line)
{
    free_line_textures(line);
    memset(line, 0, sizeof(Line));
    line->bg_color = COLOR_DEFAULT_BG;
}

static void editor_recolor_line(int line_idx)
{
    if (line_idx < 0 || line_idx >= g_editor.line_count) return;

    Line *line = &g_editor.lines[line_idx];
    bool in_comment = false;
    bool after_hash = false;

    for (int i = 0; i < line->length; i++) {
        CharCell *cell = &line->chars[i];
        char c = cell->c;

        cell->fg = COLOR_DEFAULT_FG;

        // Very simple comment detection (//)
        if (i > 0 && line->chars[i-1].c == '/' && c == '/') {
            in_comment = true;
        }

        if (in_comment) {
            cell->fg = COLOR_COMMENT;
            continue;
        }

        // Preprocessor (#...)
        if (c == '#') {
            after_hash = true;
            cell->fg = COLOR_PREPROC;
            continue;
        }

        if (after_hash && (isalpha(c) || c == '_')) {
            cell->fg = COLOR_PREPROC;
            continue;
        }
        if (after_hash && !isalnum(c) && c != '_') {
            after_hash = false;
        }

        // You can add more rules later (keywords, strings, numbers...)
    }
}

static void editor_recreate_textures_line(int line_idx)
{
    if (line_idx < 0 || line_idx >= g_editor.line_count) return;

    Line *line = &g_editor.lines[line_idx];

    // Free old textures
    for (int i = 0; i < line->length; i++) {
        if (line->chars[i].texture) {
            SDL_DestroyTexture(line->chars[i].texture);
            line->chars[i].texture = NULL;
        }
    }

    // Create new ones
    for (int i = 0; i < line->length; i++) {
        CharCell *cell = &line->chars[i];
        cell->texture = create_char_texture(cell->c, cell->fg, cell->bg);
    }
}

void editor_cleanup(void)
{
    if (g_editor.lines) {
        for (int i = 0; i < g_editor.line_count; i++) {
            free_line_textures(&g_editor.lines[i]);
        }
        free(g_editor.lines);
        g_editor.lines = NULL;
    }
    g_editor.line_count = 0;
    g_editor.capacity = 0;
}

void editor_insert_char(char c)
{
    if (c == '\r' || c == '\n') {
        editor_newline();
        return;
    }

    if (c < 32 && c != '\t') return;

    Line *line = &g_editor.lines[g_editor.cursor_line];

    // If line is full → new line **before** inserting the char
    if (line->length >= MAX_CHARS_PER_LINE) {
        editor_newline();
        line = &g_editor.lines[g_editor.cursor_line];  // now the new empty line
    }

    // Make space
    if (g_editor.cursor_col < line->length) {
        memmove(&line->chars[g_editor.cursor_col + 1],
                &line->chars[g_editor.cursor_col],
                (line->length - g_editor.cursor_col) * sizeof(CharCell));
    }

    CharCell *cell = &line->chars[g_editor.cursor_col];
    cell->c = c;
    cell->fg = g_editor.default_fg;
    cell->bg = line->bg_color;
    cell->texture = create_char_texture(c, cell->fg, cell->bg);

    line->length++;
    g_editor.cursor_col++;
	
	g_editor.modified = true;
    editor_update_textures_line(g_editor.cursor_line - 1);
	editor_update_textures_line(g_editor.cursor_line);
	update_scroll();
}

void editor_update_textures_line(int line_idx)
{
    if (line_idx < 0 || line_idx >= g_editor.line_count) return;
    Line *line = &g_editor.lines[line_idx];
    
    // Free all existing
    for (int i = 0; i < line->length; i++) {
        if (line->chars[i].texture) {
            SDL_DestroyTexture(line->chars[i].texture);
            line->chars[i].texture = NULL;
        }
    }
    
    // Recreate
    for (int i = 0; i < line->length; i++) {
        CharCell *cell = &line->chars[i];
        cell->texture = create_char_texture(cell->c, cell->fg, cell->bg);
    }
    
        // Update colors and textures
    editor_recolor_line(g_editor.cursor_line - 1);
    editor_recolor_line(g_editor.cursor_line);
    
}
void editor_backspace(void)
{
    if (g_editor.cursor_col == 0 && g_editor.cursor_line == 0)
        return;

    if (g_editor.cursor_col > 0) {
        Line *line = &g_editor.lines[g_editor.cursor_line];

        // Free the texture we're about to remove
        if (line->chars[g_editor.cursor_col - 1].texture) {
            SDL_DestroyTexture(line->chars[g_editor.cursor_col - 1].texture);
        }

        memmove(&line->chars[g_editor.cursor_col - 1],
                &line->chars[g_editor.cursor_col],
                (line->length - g_editor.cursor_col + 1) * sizeof(CharCell));

        line->length--;
        g_editor.cursor_col--;

		editor_update_textures_line(g_editor.cursor_line - 1);
		editor_update_textures_line(g_editor.cursor_line);
    }
    else {
        // Cursor at start of line → merge with previous line (simple version)
        if (g_editor.cursor_line > 0) {
            Line *prev = &g_editor.lines[g_editor.cursor_line - 1];
            Line *curr = &g_editor.lines[g_editor.cursor_line];

            int prev_len = prev->length;

            if (prev_len + curr->length <= MAX_CHARS_PER_LINE) {
                // Can merge fully
                memcpy(&prev->chars[prev_len],
                       curr->chars,
                       curr->length * sizeof(CharCell));

                prev->length += curr->length;

                // Shift all following lines up
                memmove(&g_editor.lines[g_editor.cursor_line],
                        &g_editor.lines[g_editor.cursor_line + 1],
                        (g_editor.line_count - g_editor.cursor_line - 1) * sizeof(Line));

                g_editor.line_count--;
                g_editor.cursor_line--;
                g_editor.cursor_col = prev_len;

				editor_update_textures_line(g_editor.cursor_line - 1);
				editor_update_textures_line(g_editor.cursor_line);
            }
        }
    }
    update_scroll();
}

void editor_delete(void)
{
    Line *line = &g_editor.lines[g_editor.cursor_line];

    if (g_editor.cursor_col < line->length) {
        // Free texture of deleted char
        if (line->chars[g_editor.cursor_col].texture) {
            SDL_DestroyTexture(line->chars[g_editor.cursor_col].texture);
        }

        memmove(&line->chars[g_editor.cursor_col],
                &line->chars[g_editor.cursor_col + 1],
                (line->length - g_editor.cursor_col - 1) * sizeof(CharCell));

        line->length--;

    editor_update_textures_line(g_editor.cursor_line - 1);
	editor_update_textures_line(g_editor.cursor_line);
    }
    // Note: no merge with next line for now (keep it simple)
    update_scroll();
}


void editor_newline(void)
{
    if (g_editor.line_count >= g_editor.capacity) {
        // TODO: later add resizing of lines array
        return;
    }

    // Current line
    Line *current = &g_editor.lines[g_editor.cursor_line];
    int split_pos = g_editor.cursor_col;
    int chars_to_move = current->length - split_pos;

    // Make space: shift all lines AFTER current one down by 1
    memmove(&g_editor.lines[g_editor.cursor_line + 2],
            &g_editor.lines[g_editor.cursor_line + 1],
            (g_editor.line_count - g_editor.cursor_line - 1) * sizeof(Line));

    // Now create the new line at the right position (cursor_line + 1)
    Line *new_line = &g_editor.lines[g_editor.cursor_line + 1];
    line_clear(new_line);  // prepare fresh line

    // Move text after cursor to new line
    if (chars_to_move > 0) {
        memcpy(new_line->chars,
               &current->chars[split_pos],
               chars_to_move * sizeof(CharCell));
        new_line->length = chars_to_move;
    }

    // Shorten current line
    current->length = split_pos;

    // No need to destroy textures here — new_line owns the moved ones now

    // Increase total line count
    g_editor.line_count++;

    // Move cursor to beginning of the newly inserted line
    g_editor.cursor_line++;   // now points to the new line
    g_editor.cursor_col = 0;

    // Update colors and textures for affected lines
    editor_recolor_line(g_editor.cursor_line - 1);   // old line
    editor_recolor_line(g_editor.cursor_line);       // new line

    editor_update_textures_line(g_editor.cursor_line - 1);
    editor_update_textures_line(g_editor.cursor_line);

    g_editor.modified = true;

    update_scroll();
}
// ... more movement functions can be added later

void editor_handle_event(const SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN)
    {
        SDL_Keycode key = event->key.keysym.sym;
        SDL_Keymod mod  = event->key.keysym.mod;

        // Movement
        if (key == SDLK_LEFT) {
            editor_move_cursor_left();
        }
        else if (key == SDLK_RIGHT) {
            editor_move_cursor_right();
        }
        else if (key == SDLK_UP) {
            editor_move_cursor_up();
        }
        else if (key == SDLK_DOWN) {
            editor_move_cursor_down();
        }
		else if (key == SDLK_ESCAPE)
		{
			g_editor.active = FALSE;
			add_terminal_line("Editor closed", LINE_FLAG_SYSTEM);
			update_max_scroll();
		}

        // Edit
        else if (key == SDLK_BACKSPACE) {
            editor_backspace();
        }
        else if (key == SDLK_DELETE) {
            editor_delete();           // forward delete
        }
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            editor_newline();
        }

        // Optional: Ctrl + something later (copy/paste, etc.)
        // if ((mod & KMOD_CTRL) && key == SDLK_c) { ... }
    }

    else if (event->type == SDL_TEXTINPUT)
    {
        // Normal typing (letters, digits, space, punctuation, etc.)
        for (const char *c = event->text.text; *c; c++)
        {
            editor_insert_char(*c);
        }
    }
    
	else if (event->type == SDL_MOUSEWHEEL)
	{
		if (event->wheel.y > 0) editor_scroll_up();
		else if (event->wheel.y < 0) editor_scroll_down();
		
		clamp_scroll();  // still good to have
	}

    // Optional: you can also handle SDL_TEXTEDITING if you want IME/composition support later
}
// editor_render - now draws static underscore as cursor
void editor_render(void)
{
    // Clear background
    SDL_SetRenderDrawColor(
        _sdl.renderer,
        g_editor.default_bg.r,
        g_editor.default_bg.g,
        g_editor.default_bg.b,
        g_editor.default_bg.a
    );
    SDL_RenderClear(_sdl.renderer);


	int start_line = g_editor.scroll_line;
	int end_line   = start_line + g_editor.visible_lines;
	if (end_line > g_editor.line_count) end_line = g_editor.line_count;

	int y = 8;  // top margin

	for (int i = start_line; i < end_line; i++) {
		Line *line = &g_editor.lines[i];

		// === GUTTER (same as before) ===
		SDL_Rect gutter = {0, y, 12 + g_editor.line_number_width, g_editor.line_height};
		SDL_SetRenderDrawColor(_sdl.renderer, LINE_NUMBER_BG.r, LINE_NUMBER_BG.g, LINE_NUMBER_BG.b, 255);
		SDL_RenderFillRect(_sdl.renderer, &gutter);

		char num_buf[16];
		snprintf(num_buf, sizeof(num_buf), "%d", i + 1);
		SDL_Surface *num_surf = TTF_RenderText_Blended(_sdl.font, num_buf, LINE_NUMBER_FG);
		if (num_surf) {
		    SDL_Texture *num_tex = SDL_CreateTextureFromSurface(_sdl.renderer, num_surf);
		    if (num_tex) {
		        SDL_Rect dst = {
		            .x = 12 + g_editor.line_number_width - LINE_NUMBER_PADDING - num_surf->w,
		            .y = y + (g_editor.line_height - num_surf->h)/2,
		            .w = num_surf->w,
		            .h = num_surf->h
		        };
		        SDL_RenderCopy(_sdl.renderer, num_tex, NULL, &dst);
		        SDL_DestroyTexture(num_tex);
		    }
		    SDL_FreeSurface(num_surf);
		}

		// === TEXT ===
		int x = 12 + g_editor.line_number_width + LINE_NUMBER_MARGIN;
		for (int j = 0; j < line->length; j++) {
		    CharCell *cell = &line->chars[j];
		    if (cell->texture) {
		        SDL_Rect dst = {x, y, g_editor.char_advance, g_editor.line_height};
		        SDL_RenderCopy(_sdl.renderer, cell->texture, NULL, &dst);
		    }
		    x += g_editor.char_advance;
		}

		// === CURSOR (only if this line is visible) ===
		if (i == g_editor.cursor_line) {
		    int cursor_x = 12 + g_editor.line_number_width + LINE_NUMBER_MARGIN +
		                   g_editor.cursor_col * g_editor.char_advance;
		    
		    SDL_Rect cursor_rect = {cursor_x, y + 2, 2, g_editor.line_height - 4};
		    SDL_SetRenderDrawColor(_sdl.renderer,
		        g_editor.cursor_color.r, g_editor.cursor_color.g,
		        g_editor.cursor_color.b, g_editor.cursor_color.a);
		    SDL_RenderFillRect(_sdl.renderer, &cursor_rect);
		}

		y += g_editor.line_height;
	}
		
	// ──────────────────────────────────────────────────────────────
	// STATUS BAR - bottom of screen (2 rows)
	// ──────────────────────────────────────────────────────────────

	int status_height = g_editor.line_height * 2 + 8;
	int status_y = _sdl.height - status_height;

	if (status_y < 0) status_y = 0;  // safety if window too small

	// Background
	SDL_Rect status_bg = {0, status_y, _sdl.width, status_height};
	SDL_SetRenderDrawColor(_sdl.renderer, 38, 38, 48, 255);
	SDL_RenderFillRect(_sdl.renderer, &status_bg);

	// ─── Row 1 ──────────────────────────────────────────────────────
	int row1_y = status_y + 4;

	// Mode (left)
	const char* mode_str = g_editor.is_insert_mode ? "INSERT" : "VIEW";
	SDL_Color mode_col = g_editor.is_insert_mode ? 
		                 (SDL_Color){90, 220, 90, 255} : 
		                 (SDL_Color){255, 170, 80, 255};

	SDL_Surface* mode_surf = TTF_RenderText_Blended(_sdl.font, mode_str, mode_col);
	if (mode_surf) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(_sdl.renderer, mode_surf);
		if (tex) {
		    SDL_Rect dst = {12, row1_y + (g_editor.line_height - mode_surf->h)/2,
		                    mode_surf->w, mode_surf->h};
		    SDL_RenderCopy(_sdl.renderer, tex, NULL, &dst);
		    SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(mode_surf);
	}

	// Filename + modified (right-aligned)
	char file_status[120];
	const char* fname = g_editor.filename[0] ? g_editor.filename : "untitled";
	snprintf(file_status, sizeof(file_status), "%s%s", fname, g_editor.modified ? " *" : "");

	SDL_Surface* file_surf = TTF_RenderText_Blended(
		_sdl.font, 
		file_status,
		(SDL_Color){200, 200, 220, 255}
	);

	if (file_surf) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(_sdl.renderer, file_surf);
		if (tex) {
		    // Right-align: position near the right edge with padding
		    int padding_right = 16;  // ← tune this value (distance from right border)
		    int x = _sdl.width - file_surf->w - padding_right;

		    // Safety: don't let it overlap the mode text on small windows
		    int min_x = 140;  // roughly after mode + some margin
		    if (x < min_x) {
		        x = min_x;
		    }

		    SDL_Rect dst = {
		        x,
		        row1_y + (g_editor.line_height - file_surf->h) / 2,
		        file_surf->w,
		        file_surf->h
		    };

		    SDL_RenderCopy(_sdl.renderer, tex, NULL, &dst);
		    SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(file_surf);
	}

	// ─── Row 2 ──────────────────────────────────────────────────────
	int row2_y = status_y + g_editor.line_height + 4;

	char info[180];
	snprintf(info, sizeof(info),
		     "Last save: %s    Ctrl+S save    Ctrl+E export    Esc: Back to shell",
		     get_time_since_last_save());

	SDL_Surface* info_surf = TTF_RenderText_Blended(_sdl.font, info, 
		                                           (SDL_Color){150,150,170,255});
	if (info_surf) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(_sdl.renderer, info_surf);
		if (tex) {
		    int x = 12;
		    if (info_surf->w > _sdl.width - 24)
		        x = _sdl.width - 12 - info_surf->w;  // right align if too long
		    SDL_Rect dst = {x, row2_y + (g_editor.line_height - info_surf->h)/2,
		                    info_surf->w, info_surf->h};
		    SDL_RenderCopy(_sdl.renderer, tex, NULL, &dst);
		    SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(info_surf);
	}

    SDL_RenderPresent(_sdl.renderer);
}

