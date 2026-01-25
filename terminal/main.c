#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif

#include <string.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "global.h"
#include "base64.h"
#include "data_codec.h"
#include "settings.h"
#include "cmd.h"
#include "ascii_converter.h"
#include "sdl.h"
#include "translate.h"
#include "forecast.h"
#include "editor.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, disable_canvas_smoothing, (), {
    setTimeout(function () {
        var canvas = Module.canvas;
        if (!canvas) return;
        canvas.style.imageRendering = "pixelated";
        canvas.style.imageRendering = "crisp-edges";
        canvas.style.imageRendering = "-moz-crisp-edges";
    }, 0);
});
#endif

AppContext app = {ZERO_MEMORY};

void app_init(void) {

    memset(&app, ZERO_MEMORY, sizeof(AppContext));
	
    VersionInfo v = {
        .app               = {"1.0", "2026-01-18"},
        .terminal          = {"1.0", "2026-01-17"},
        .weather_forecast  = {"1.0", "2026-01-16"},
        .ascii_converter   = {"1.0", "2026-01-15"},
        .translator        = {"1.0", "2026-01-14"},
        .editor            = {"dev", "2026-01-14"},
    };

    app.version = v; 

	#ifdef __EMSCRIPTEN__
		app.config.viewport_width  = EM_ASM_INT({ return window.innerWidth; });
		app.config.viewport_height = EM_ASM_INT({ return window.innerHeight; });
	#else
		app.config.viewport_width  = DESKTOP_WIDTH;
		app.config.viewport_height = DESKTOP_HEIGHT;
	#endif

    app.is_mobile_view = (app.config.viewport_width < MIN_DESKTOP_WIDTH) ? TRUE : FALSE;
    app.config.font_size = app.is_mobile_view ? XS : M;

    if (!init_sdl(&app.sdl, APP_NAME,
                  app.config.viewport_width,
                  app.config.viewport_height,
                  app.config.font_size)) {
        exit(TRUE);
    }

    apply_theme(&app.terminal.settings, THEME_DEFAULT);

    app.terminal.running = TRUE;
    app.terminal.tick_rate = MEDIUM;
    app.terminal.last_tick = 0;
    app.terminal.width   = app.config.viewport_width;
    app.terminal.height  = app.config.viewport_height;
    app.terminal.scroll_offset_px = ZERO_MEMORY;
    app.terminal.max_scroll = ZERO_MEMORY;
    app.terminal.line_count = ZERO_MEMORY;
    app.terminal.history.count = ZERO_MEMORY;
    app.terminal.history.pos = ZERO_MEMORY;
    
    for (int i = 0; i < MAX_HISTORY_COMMANDS; i++) {
        app.terminal.history.commands[i] = NULL;
    }
    
    _terminal.log_visible = FALSE;
	_terminal.log_count = 0;

	for (int i = 0; i < LOG_MAX_LINES; i++) {
		_terminal.logs[i].type = LOG_INFO;
		_terminal.logs[i].datetime[0] = '\0';
		_terminal.logs[i].text[0] = '\0';
	}

    if (!app.terminal.settings.font) {
        app.terminal.settings.font = TTF_OpenFont(FONT_PATH, app.config.font_size);
        if (!app.terminal.settings.font) {
            printf("Failed to load font: %s\n", TTF_GetError());
        }
    }
    
    set_terminal_font_hinting(TTF_HINTING_MONO);
    
	add_terminal_line(" ╔════════════════════════════════════════╗", LINE_FLAG_SYSTEM);
	add_terminal_line(" ║          Rekav Terminal v1.0           ║", LINE_FLAG_SYSTEM);
	add_terminal_line(" ╚════════════════════════════════════════╝", LINE_FLAG_SYSTEM);
	add_terminal_line("", LINE_FLAG_NONE);
	add_terminal_line("  Minimalist. For Fun. Built in pure C and WASM Emscripten.", LINE_FLAG_HIGHLIGHT);
	add_terminal_line(" ", LINE_FLAG_NONE);
	add_terminal_line(" NOTE (dev status – Jan. 2026)", LINE_FLAG_SYSTEM | LINE_FLAG_BOLD);
	// You can combine flags if your system supports it, or just use LINE_FLAG_SYSTEM
	add_terminal_line(" • The code editor is currently in early development", LINE_FLAG_NONE);
	add_terminal_line("   UI/skeleton, but most editing", LINE_FLAG_NONE);
	add_terminal_line("   features are either missing or only partially working.", LINE_FLAG_NONE);
	add_terminal_line(" • Many promised functions are bare-bones or placeholders", LINE_FLAG_NONE);
	add_terminal_line("   at this stage — this is **not** the final version yet.", LINE_FLAG_NONE);
	add_terminal_line(" ", LINE_FLAG_NONE);

	add_terminal_line("  Type 'help' or just start typing :)", LINE_FLAG_ITALIC);
	add_terminal_line(" ", LINE_FLAG_NONE);
	add_terminal_line("  Have fun!", LINE_FLAG_HIGHLIGHT);
	add_terminal_line(" ", LINE_FLAG_NONE);

    reset_current_input();

    update_max_scroll();
}

void app_cleanup(void) {
    cleanup_sdl(&app.sdl);

    if (app.terminal.settings.font) {
        TTF_CloseFont(app.terminal.settings.font);
        app.terminal.settings.font = NULL;
    }

    for (int i = 0; i < app.terminal.line_count; i++) {
        if (app.terminal.lines[i].texture) {
            SDL_DestroyTexture(app.terminal.lines[i].texture);
            app.terminal.lines[i].texture = NULL;
        }
    }
}

static inline BOOL terminal_is_busy(void) {
    return _translation_pending
        || _weather_forecast_pending
        || _image_processing_pending
        || _image_download_pending;
}

const char* get_current_prompt(void) {

    if (terminal_is_busy())
        return "[working...] ";

    if (_awaiting_sudo_password)
        return "password: ";

    if (_root_active)
        return "root@rekav:~$ ";

    return DEFAULT_PROMPT;
}

void update_max_scroll() {
    int content_height = terminal_content_height();
    int viewport_height = _terminal.height - TERMINAL_PADDING_TOP - TERMINAL_PADDING_BOTTOM;

    if (content_height <= viewport_height) {
        _terminal.max_scroll = 0;
    } else {
        _terminal.max_scroll = content_height - viewport_height;
    }

    if (_terminal.scroll_offset_px < 0)
        _terminal.scroll_offset_px = 0;

    if (_terminal.scroll_offset_px > _terminal.max_scroll)
        _terminal.scroll_offset_px = _terminal.max_scroll;
}

int terminal_content_height() {
    int h = 0;
    for (int i = 0; i < _terminal.line_count; i++) {
        h += _terminal.lines[i].line_height;
    }
    update_input_texture();
    h += _terminal.input.texture_h;
    return h;
}

void submit_input(void)
{
    const char *full_input = _terminal.input.buffer;
    int prompt_len = _terminal.input.prompt_len;
    const char *clean_cmd = full_input + prompt_len;
    while (*clean_cmd == ' ') clean_cmd++;
    size_t cmd_len = strlen(clean_cmd);

    if (_awaiting_sudo_password)
    {
        _awaiting_sudo_password = FALSE;

        if (strcmp(clean_cmd, ROOT_PASSWORD) == 0)
        {
            _root_active = 1;

			#ifdef __EMSCRIPTEN__
			emscripten_run_script("sessionStorage.setItem('rekav_root_session','true'); console.log('Root session set via emscripten_run_script');");
			#endif
			
			push_base64_to_storage(data_encoded, "rekav_data");
			const char *articles[] = { article_0, article_1, article_2 };
			const char *keys[]     = { "rekav_article_0", "rekav_article_1", "rekav_article_2" };
			int article_count = 3;

			for (int i = 0; i < article_count; i++) {
				push_base64_to_storage(articles[i], keys[i]);
			}

            add_terminal_line("Root access granted.", LINE_FLAG_SYSTEM);
        }
        else
        {
            add_terminal_line("sudo: Access denied.", LINE_FLAG_ERROR);
        }

        reset_current_input();
        _terminal.dirty = TRUE;
        return;
    }

    if (cmd_len == 0)
    {
        reset_current_input();
        _terminal.dirty = TRUE;
        return;
    }

    const int MAX_DISPLAY_LEN = 120; // TODO: Compute this according to font size

    if (cmd_len <= MAX_DISPLAY_LEN)
    {
        add_terminal_line(full_input, LINE_FLAG_PROMPT);
    }
    else
    {
        char first_part[128];
        snprintf(first_part, sizeof(first_part), "%.*s", prompt_len + MAX_DISPLAY_LEN, full_input);
        add_terminal_line(first_part, LINE_FLAG_PROMPT);

        // Then add continuation lines
        const char *ptr = clean_cmd + MAX_DISPLAY_LEN;
        while (*ptr)
        {
            char chunk[128];
            int chunk_len = strnlen(ptr, MAX_DISPLAY_LEN);
            strncpy(chunk, ptr, chunk_len);
            chunk[chunk_len] = '\0';
            add_terminal_line(chunk, LINE_FLAG_NONE);
            ptr += chunk_len;
        }
    }

    execute_command(clean_cmd);
    
	if (!_awaiting_sudo_password && cmd_len > 0) {
		char *cmd_copy = strdup(clean_cmd);
		if (cmd_copy) {
			if (_terminal.history.count == MAX_HISTORY_COMMANDS) {
			    free(_terminal.history.commands[0]);
			    memmove(_terminal.history.commands,
			            _terminal.history.commands + 1,
			            sizeof(char*) * (MAX_HISTORY_COMMANDS - 1));
			    _terminal.history.commands[MAX_HISTORY_COMMANDS - 1] = cmd_copy;
			} else {
			    _terminal.history.commands[_terminal.history.count++] = cmd_copy;
			}
			_terminal.history.pos = -1;
		}

		#ifdef __EMSCRIPTEN__
			EM_ASM({
				let hist = Module.history_array || [];
				let cmd = UTF8ToString($0);
				hist.push(cmd);
				if (hist.length > 64) hist.shift();
				Module.history_array = hist;
				sessionStorage.setItem('rekav_history', JSON.stringify(hist));
			}, cmd_copy);
		#endif
	}
	
    reset_current_input();
    update_max_scroll();

    if (is_at_bottom()) {
        _terminal.scroll_offset_px = _terminal.max_scroll;
    }    
    _terminal.dirty = TRUE;
}

void clear_terminal() {
    _terminal.line_count = 0;
    _terminal.scroll_offset_px = 0;
    _terminal.dirty = TRUE;
}

void reset_current_input() {
    const char *prompt = get_current_prompt();
    int plen = strlen(prompt);

    strncpy(_terminal.input.buffer, prompt, INPUT_MAX_CHARS - 1);
    _terminal.input.buffer[INPUT_MAX_CHARS - 1] = '\0';

    _terminal.input.prompt_len = plen;
    _terminal.input.cursor_pos = plen;
    _terminal.input.dirty = TRUE;

    if (_terminal.input.texture) {
        SDL_DestroyTexture(_terminal.input.texture);
        _terminal.input.texture = NULL;
    }
    _terminal.input.texture_w = 0;
    _terminal.input.texture_h = 0;
}

void add_terminal_line(const char *text, TerminalLineFlags flags)
{
    // FIFO: buffer full, drop the oldest line
    if (_terminal.line_count == MAX_LINES) {

        // Destroy texture of the oldest line
        if (_terminal.lines[0].texture) {
            SDL_DestroyTexture(_terminal.lines[0].texture);
            _terminal.lines[0].texture = NULL;
        }

        // Shift everything up by one (FIFO)
        memmove(
            &_terminal.lines[0],
            &_terminal.lines[1],
            sizeof(TerminalLine) * (MAX_LINES - 1)
        );

        _terminal.line_count = MAX_LINES - 1;
    }

    // Append new line at tail
    int idx = _terminal.line_count;
    TerminalLine *line = &_terminal.lines[idx];
    memset(line, 0, sizeof(*line));

    // UTF-8 safe copy
    size_t len = strnlen(text, MAX_LINE_LENGTH - 1);
    while (len > 0 && (text[len] & 0xC0) == 0x80) len--;
    memcpy(line->text, text, len);
    line->text[len] = '\0';

    // Style & flags
    SDL_Color font_color = _terminal.settings.font_color;
    SDL_Color bg_color   = _terminal.settings.line_background_color;
    
	int font_style = TTF_STYLE_NORMAL;

	if (flags & LINE_FLAG_ERROR) {
        font_color = COLOR_RED_RGB;
        font_style |= TTF_STYLE_BOLD;
    }
    else if (flags & LINE_FLAG_WARNING) {
        font_color = COLOR_ORANGE_RGB;
        font_style |= TTF_STYLE_BOLD;
    }
    else if (flags & LINE_FLAG_SYSTEM) {
        font_color = COLOR_CYAN_RGB;
    }
    else if (flags & LINE_FLAG_HIGHLIGHT) {
        font_color = COLOR_YELLOW_RGB;
    }

    if (flags & LINE_FLAG_BOLD) {
        font_style |= TTF_STYLE_BOLD;
    }
    if (flags & LINE_FLAG_ITALIC) {
        font_style |= TTF_STYLE_ITALIC;
    }

    line->font             = _terminal.settings.font;
    line->font_color       = font_color;
    line->background_color = bg_color;
    line->letter_spacing   = _terminal.settings.letter_spacing;
    line->flags            = flags;
    line->dirty            = TRUE;
    line->cursor_index     = 0;

    TTF_SetFontStyle(line->font, font_style);

    // Render text
    int max_width = _terminal.width
                  - TERMINAL_PADDING_LEFT
                  - TERMINAL_PADDING_RIGHT
                  - 4;

    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(
        line->font,
        line->text,
        line->font_color,
        max_width
    );
    
	if (surface) {
		line->texture = SDL_CreateTextureFromSurface(_sdl.renderer, surface);
		line->width = surface->w;
		line->height = surface->h;
		line->line_height = SDL_max(surface->h, TTF_FontHeight(line->font) + 2);
		SDL_FreeSurface(surface);
	} else {
		line->line_height = TTF_FontLineSkip(line->font);
	}

    _terminal.line_count++;
    _terminal.dirty = TRUE;

    update_max_scroll();
    _terminal.scroll_offset_px = _terminal.max_scroll;
}


void update_input_texture(void)
{
    if (!_terminal.input.dirty) return;

    if (_terminal.input.texture) {
        SDL_DestroyTexture(_terminal.input.texture);
        _terminal.input.texture = NULL;
        _terminal.input.texture_w = 0;
        _terminal.input.texture_h = 0;
    }

    int max_width = _terminal.width - TERMINAL_PADDING_LEFT - TERMINAL_PADDING_RIGHT - 10;

    char render_text[INPUT_MAX_CHARS + 2];
    render_text[0] = '\0';

    if (_awaiting_sudo_password) {
        size_t real_len = strlen(_terminal.input.buffer);
        size_t prompt_len = _terminal.input.prompt_len;
        strncat(render_text, _terminal.input.buffer, prompt_len);
        size_t hidden_len = real_len - prompt_len;
        for (size_t i = 0; i < hidden_len; i++) {
            strncat(render_text, PASSWORD_CHAR, 1);
        }

        int visual_cursor_pos = prompt_len + (int)(hidden_len > 0 ? hidden_len : 0);
        if (_terminal.input.cursor_pos >= (int)prompt_len) {
            char temp[INPUT_MAX_CHARS + 2];
            strncpy(temp, render_text, visual_cursor_pos);
            temp[visual_cursor_pos] = '\0';
            strcat(temp, CURSOR_CHAR);
            strcat(temp, render_text + visual_cursor_pos);
            strcpy(render_text, temp);
        }
    }
    else {
        size_t len = strlen(_terminal.input.buffer);
        int cursor = _terminal.input.cursor_pos;

        if (cursor >= 0 && cursor <= (int)len) {
            strncat(render_text, _terminal.input.buffer, cursor);
            strncat(render_text, CURSOR_CHAR, 1);
            strncat(render_text, _terminal.input.buffer + cursor, len - cursor);
        }
        else {
            strcpy(render_text, _terminal.input.buffer);
        }
    }

    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(
        _terminal.settings.font,
        render_text,
        _terminal.settings.font_color,
        max_width
    );

    if (surface) {
        _terminal.input.texture = SDL_CreateTextureFromSurface(_sdl.renderer, surface);
        if (_terminal.input.texture) {
            SDL_QueryTexture(_terminal.input.texture, NULL, NULL,
                             &_terminal.input.texture_w,
                             &_terminal.input.texture_h);
        }
        SDL_FreeSurface(surface);
    }

    _terminal.input.dirty = false;
}

static void render_terminal_background(void)
{
    // Background clear
    SDL_SetRenderDrawColor(_sdl.renderer,
                           _terminal.settings.background.r,
                           _terminal.settings.background.g,
                           _terminal.settings.background.b,
                           _terminal.settings.background.a);
    SDL_RenderClear(_sdl.renderer);

    // Background texture if any
    if (_terminal.settings.background_texture) {
        SDL_RenderCopy(_sdl.renderer, _terminal.settings.background_texture, NULL, NULL);
    }
}

static void render_terminal_lines(void)
{
    int y = TERMINAL_PADDING_TOP - _terminal.scroll_offset_px;

    for (int i = 0; i < _terminal.line_count; i++) {
        TerminalLine *ln = &_terminal.lines[i];

        if (y + ln->height < 0) {
            y += ln->line_height;
            continue;
        }

        if (ln->texture) {
            SDL_Rect dst = {TERMINAL_PADDING_LEFT, y, ln->width, ln->height};
            SDL_RenderCopy(_sdl.renderer, ln->texture, NULL, &dst);
        }

        y += ln->line_height;
    }
}

static void render_terminal_input(void)
{
    update_input_texture();

    if (_terminal.input.texture) {
        int input_y = TERMINAL_PADDING_TOP - _terminal.scroll_offset_px;
        // Find y after last history line
        for (int i = 0; i < _terminal.line_count; i++) {
            input_y += _terminal.lines[i].line_height;
        }

        SDL_Rect dst = {
            TERMINAL_PADDING_LEFT,
            input_y,
            _terminal.input.texture_w,
            _terminal.input.texture_h
        };
        SDL_RenderCopy(_sdl.renderer, _terminal.input.texture, NULL, &dst);
    }
}

void render_terminal(void)
{
    render_terminal_background();
    render_terminal_lines();
    render_terminal_input();

    SDL_RenderPresent(_sdl.renderer);
    _terminal.dirty = FALSE;
}
int is_at_bottom() {
    return _terminal.scroll_offset_px >= _terminal.max_scroll - 20; // small tolerance
}

static bool handle_history_navigation(SDL_Keycode key)
{


    if (_awaiting_sudo_password) {
        return false;
    }

    if (_terminal.history.count == 0) {
        return false;
    }

    if (key == SDLK_UP) {
        if (_terminal.history.pos == -1) {
            _terminal.history.pos = _terminal.history.count - 1;
        } else if (_terminal.history.pos > 0) {
            _terminal.history.pos--;
        } else {
            return true;
        }
        update_max_scroll();
    }
    else if (key == SDLK_DOWN) {
        if (_terminal.history.pos == -1) {
            return true;
        }

        if (_terminal.history.pos < _terminal.history.count - 1) {
            _terminal.history.pos++;
        } else {
            _terminal.history.pos = -1;
            reset_current_input();
            _terminal.dirty = TRUE;
            return true;
        }
        update_max_scroll();
    }
    
    else {
        return false;
    }

    const char *cmd = _terminal.history.commands[_terminal.history.pos];
    if (cmd) {
        size_t prompt_len = _terminal.input.prompt_len;
        size_t max_after_prompt = INPUT_MAX_CHARS - prompt_len - 1;
        strncpy(_terminal.input.buffer + prompt_len, cmd, max_after_prompt);
        _terminal.input.buffer[INPUT_MAX_CHARS - 1] = '\0'; // safety
        _terminal.input.cursor_pos = strlen(_terminal.input.buffer);
        _terminal.input.dirty = TRUE;
    }
	_terminal.dirty = TRUE;
    return true;
}

void insert_text_at_cursor(const char *text) {
    int pos = _terminal.input.cursor_pos; // byte offset
    size_t text_len = strlen(text);       // UTF-8 bytes
    size_t buf_len = strlen(_terminal.input.buffer);

    if (buf_len + text_len >= INPUT_MAX_CHARS) return;

    memmove(_terminal.input.buffer + pos + text_len,
            _terminal.input.buffer + pos,
            buf_len - pos + 1); // +1 for '\0'

    memcpy(_terminal.input.buffer + pos, text, text_len);

    _terminal.input.cursor_pos += text_len;
    _terminal.input.dirty = TRUE;
}

void handle_keyboard_event(SDL_Event *e) {
	
    if (e->type == SDL_KEYDOWN) {
    
		if (is_at_bottom()) {
			update_max_scroll();
			_terminal.scroll_offset_px = _terminal.max_scroll;
		}
        SDL_Keycode key = e->key.keysym.sym;
        
        if (handle_history_navigation(key)) {
            return;
        }
        
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
		    submit_input();
		    return;
		}

        if (key == SDLK_LEFT) {
            if (_terminal.input.cursor_pos > _terminal.input.prompt_len) {
                _terminal.input.cursor_pos--;
                _terminal.input.dirty = TRUE;
            }
            return;
        }
        
        if (key == SDLK_RIGHT) {
            if (_terminal.input.cursor_pos < (int)strlen(_terminal.input.buffer)) {
                _terminal.input.cursor_pos++;
                _terminal.input.dirty = TRUE;
            }
            return;
        }

        if (key == SDLK_BACKSPACE) {
            int pos = _terminal.input.cursor_pos;
            if (pos <= _terminal.input.prompt_len) {
                return;
            }

            memmove(
                _terminal.input.buffer + pos - 1,
                _terminal.input.buffer + pos,
                strlen(_terminal.input.buffer + pos) + 1
            );

            _terminal.input.cursor_pos--;
            _terminal.input.dirty = TRUE;
            return;
        }

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            return;
        }
    }

	if (e->type == SDL_TEXTINPUT) {
		const char *text = e->text.text; // UTF-8 input
		size_t text_len = strlen(text);

		int pos = _terminal.input.cursor_pos;
		size_t current_len = strlen(_terminal.input.buffer);

		if (current_len + text_len >= INPUT_MAX_CHARS - 1) {
		    return;
		}

		memmove(
		    _terminal.input.buffer + pos + text_len,
		    _terminal.input.buffer + pos,
		    current_len - pos + 1
		);
		memcpy(_terminal.input.buffer + pos, text, text_len);
		_terminal.input.cursor_pos += (int)text_len;
		_terminal.input.dirty = TRUE;
	}
}

void main_loop() {
	
	// Tick bound
    Uint32 now = SDL_GetTicks();
    if (now - _terminal.last_tick < _terminal.tick_rate)
        return;
    _terminal.last_tick = now;

    SDL_Event e;
    
    while (SDL_PollEvent(&e)) {
    
		if (g_editor.active)
	    {
	        editor_handle_event(&e);
	        continue;
	    }
        switch (e.type) {
            case SDL_QUIT:
                _terminal.running = FALSE;
                break;

            case SDL_KEYDOWN:
            case SDL_TEXTINPUT:
		            handle_keyboard_event(&e);
		        break;

            case SDL_MOUSEWHEEL:
                _terminal.scroll_offset_px -= e.wheel.y * SCROLL_DELTA_MULTIPLIER;
                if (_terminal.scroll_offset_px < 0)
                    _terminal.scroll_offset_px = 0;
                if (_terminal.scroll_offset_px > _terminal.max_scroll)
                    _terminal.scroll_offset_px = _terminal.max_scroll;
                _terminal.dirty = TRUE;
                break;

            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    _terminal.width  = e.window.data1;
                    _terminal.height = e.window.data2;
                    update_max_scroll();
                    _terminal.dirty = TRUE;
                }
                break;
        }
    }
    
	if (g_editor.active)
    {
        editor_render();   // ← Render the editor (full screen + status bar)
        return;            // ← Skip terminal render
    }
    if (_terminal.dirty || _terminal.input.dirty) {
        render_terminal();
    }
	
	if (_translation_pending) {
	    poll_translate_result();
    }
    if (_weather_forecast_pending) {
	    poll_forecast_result();
    }
    if (_image_processing_pending) {
	    poll_image_result();
    }
}

int main() {
	app_init();
	disable_canvas_smoothing();
    emscripten_set_main_loop(main_loop, FALSE, TRUE);
    cleanup_sdl(&_sdl);
    return 0;
}

void add_log(const char *text, LogType type) {
    if (!_terminal.log_count) _terminal.log_count = 0;

    int idx = (_terminal.log_count < LOG_MAX_LINES) ? _terminal.log_count : 0;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) {
        snprintf(_terminal.logs[idx].datetime, sizeof(_terminal.logs[idx].datetime),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        strncpy(_terminal.logs[idx].datetime, "0000-00-00 00:00:00", sizeof(_terminal.logs[idx].datetime));
    }

    _terminal.logs[idx].type = type;
    strncpy(_terminal.logs[idx].text, text, LOG_MAX_TEXT - 1);
    _terminal.logs[idx].text[LOG_MAX_TEXT - 1] = '\0';

    if (_terminal.log_count < LOG_MAX_LINES) _terminal.log_count++;

    char buf[600];
    const char *type_str = "";
    switch (type) {
        case LOG_INFO:    type_str = "[INFO]"; break;
        case LOG_WARNING: type_str = "[WARN]"; break;
        case LOG_ERROR:   type_str = "[ERROR]"; break;
        case LOG_FATAL:   type_str = "[FATAL]"; break;
    }
    snprintf(buf, sizeof(buf), "%s %s %s", _terminal.logs[idx].datetime, type_str, _terminal.logs[idx].text);

    if (_terminal.log_visible) {
        add_terminal_line(buf, LINE_FLAG_SYSTEM);
    }
}




