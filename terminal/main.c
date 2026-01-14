#include <SDL.h>
#include <SDL_ttf.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif
#include <string.h>
#include <stdio.h>

#include "base64.h"
#include "data_codec.h"
#include "settings.h"

#define WIDTH 800
#define HEIGHT 480

#define MAX_LINES 512
#define MAX_LINE_LENGTH 256

int root_active = 0;          // tracks if root is active
int flag_js_root_activate = 0;
int awaiting_sudo_password = 0; // tracks if sudo is waiting for password
static int translation_pending = 0;
static int weather_forecast_pending = 0;
int cursor_pos = 0; // cursor position within input
int terminal_fullscreen = 0;


#define ROOT_PASSWORD "rekav"

char input[MAX_LINE_LENGTH];
int input_len = 0;

char terminal[MAX_LINES][MAX_LINE_LENGTH];
int line_count = 0;
int scroll_offset = 0;
int running = 1;

typedef struct {
    const char *name;
    double lat;
    double lon;
} City;

static City cities[] = {
    {"Paris", 48.8566, 2.3522},
    {"London", 51.5074, -0.1278},
    {"New_York", 40.7128, -74.0060},
    {"Tokyo", 35.6895, 139.6917},
    {"Sydney", -33.8688, 151.2093},
    {"Moscow", 55.7558, 37.6173},
    {"Berlin", 52.52, 13.4050},
    {"Toronto", 43.6532, -79.3832},
    {"Rio", -22.9068, -43.1729},
    {"Cape_Town", -33.9249, 18.4241}
};
static const int city_count = sizeof(cities) / sizeof(cities[0]);

// Struct to store each cmd
typedef void (*command_func)(const char *args);

typedef struct {
    const char *name;
    const char *desc;
    command_func func;
} Command;

typedef struct {
    const char *cmd;
    const char *description;
} ManEntry;

static ManEntry man_db[] = {
    {
        "translate",
        "--------------------------------------------------\n"
        "                   TRANSLATE                      \n"
        "--------------------------------------------------\n\n"
        "translate <source> <target> <text>\n"
        "  Sends <text> to the translator and prints the result in the terminal.\n\n"
        "Usage:\n"
        "  translate en fr Hello world\n"
        "  translate es en 'Hola mundo'\n\n"
        "Examples:\n"
        "  translate en fr Hello world\n"
        "  translate es en Goodbye!\n"
        "  translate ja en 'How are you?'\n\n"
        "Notes:\n"
        "  - Translations are handled asynchronously.\n"
        "  - Result is automatically printed in the terminal.\n"
        "  - You must specify both <source> and <target> languages.\n"
        "  - MyMemory API is used; short phrases may return unexpected results.\n"
        "  - Machine translation fallback is forced (mt=1) for reliability.\n"
        "  - Supported languages include:\n"
        "      en (English), fr (French), es (Spanish), de (German), it (Italian),\n"
        "      pt (Portuguese), nl (Dutch), ru (Russian), ja (Japanese), zh (Chinese)\n"
        "  - Use quotes for multi-word text.\n\n"
        "--------------------------------------------------\n"
        "        Type 'translate <src> <tgt> <text>'         \n"
        "--------------------------------------------------\n"
    },
};


static const int man_db_size = sizeof(man_db) / sizeof(man_db[0]);


// cmd prototypes 
void add_line(const char *text);
void clear_terminal();
void cmd_help(const char *args);
void cmd_clear(const char *args);
void cmd_echo(const char *args);
void cmd_open(const char *args);
void execute_command(const char *cmd);
void cmd_ls(const char *args);
void cmd_cat(const char *args);
void cmd_sudo(const char *args);
void cmd_exit(const char *args);
void cmd_settings(const char *args);
void submit_input();
void cmd_fullscreen(const char *args);
void cmd_translate(const char *args);
void cmd_man(const char *args);
void cmd_weather(const char *args);
int poll_translate_result();
int poll_forecast_result(void);


Command commands[] = {
    {"help",  "Show help",      cmd_help},
    {"clear", "Clear terminal", cmd_clear},
    {"echo",  "Print text",     cmd_echo},
    {"open",  "Open a page",    cmd_open},
    {"ls",    "List",    		cmd_ls},
	{"cat",    "open file",    	cmd_cat},
    {"sudo",    "root login",   cmd_sudo},
    {"exit",    "exit root",   cmd_exit},
    {"settings", "Modify terminal appearance", cmd_settings},
    {"fullscreen", "Toggle fullscreen mode", cmd_fullscreen},
    {"translate",  "Translate text",          cmd_translate},
    {"weather",  "Weather Info.",          cmd_weather},
    {"man",  "Display documentation",          cmd_man}, 

};

int command_count = sizeof(commands) / sizeof(commands[0]);

void cmd_weather(const char *args) {
    weather_forecast_pending = 1;
    if (!args || strlen(args) == 0) {
        add_line("Usage: weather <city>");
        add_line("Available cities:");
        for (int i = 0; i < city_count; i++) {
            add_line(cities[i].name);
        }
        return;
    }

    // Find city in array
    City *selected = NULL;
    for (int i = 0; i < city_count; i++) {
        if (strcmp(args, cities[i].name) == 0) {
            selected = &cities[i];
            break;
        }
    }

    if (!selected) {
        add_line("City not found. Type 'weather' to see available cities.");
        return;
    }

#ifdef __EMSCRIPTEN__
    EM_ASM({
        const lat = $0;
        const lon = $1;
        const city = UTF8ToString($2);

        // Store request in sessionStorage for JS to process
        sessionStorage.setItem(
            "rekav_weather_request",
            JSON.stringify({latitude: lat, longitude: lon, city: city})
        );

        console.log("C -> JS weather request queued:", city, lat, lon);
    }, selected->lat, selected->lon, selected->name);
#endif

    char buf[128];
    snprintf(buf, sizeof(buf), "Fetching weather for %s…", selected->name);
    add_line(buf);
}

void cmd_man(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: man <command>");
        return;
    }

    // Search in man_db
    for (int i = 0; i < man_db_size; i++) {
        if (strcmp(args, man_db[i].cmd) == 0) {
            // Print each line of description
            const char *desc = man_db[i].description;
            const char *line = desc;
            while (*line) {
                const char *next = strchr(line, '\n');
                if (!next) next = line + strlen(line);
                char buf[512];
                int len = next - line;
                if (len >= sizeof(buf)) len = sizeof(buf) - 1;
                memcpy(buf, line, len);
                buf[len] = '\0';
                add_line(buf);
                line = (*next) ? next + 1 : next;
            }
            return;
        }
    }

    add_line("No manual entry for this command.");
}


void cmd_settings(const char *args) {
    char option[64];
    char value[64];

	if (!args || strlen(args) == 0) {
		add_line("");
		add_line("--------------------------------------------------");
		add_line("                   SETTINGS                       ");
		add_line("--------------------------------------------------");
		add_line("");

		add_line("Usage: settings <option> <value>");
		add_line("");
		add_line("Options:");
		add_line("  bg           - Background color (charcoal, dos_blue, pink, green, red)");
		add_line("  font_color   - Font color (white, green, pink, red, orange)");
		add_line("  font_size    - Font size (10-20)");
		add_line("  line_height  - Line height (15-25)");
		add_line("  theme        - Predefined themes (Default, MS-DOS, Barbie, Jurassic, Inferno)");
		
		add_line("");
		add_line("--------------------------------------------------");
		add_line("       Example: settings bg charcoal            ");
		add_line("--------------------------------------------------");
		add_line("");
		
		return;
	}

    // Parse option and value from args
    int matched = sscanf(args, "%63s %63s", option, value);
    if (matched != 2) {
        add_line("Invalid usage! Example: settings font_size 18");
        return;
    }

	if (strcmp(option, "bg") == 0) {
		if (set_background_color(value)) {
		    add_line("Background color applied.");
		} else {
		    add_line("Unknown background color.");
		}
	} 
	else if (strcmp(option, "font_color") == 0) {
		if (set_font_color(value)) {
		    add_line("Font color applied.");
		} else {
		    add_line("Unknown font color.");
		}
	} 
	else if (strcmp(option, "font_size") == 0) {
		int size = atoi(value);
		if (set_font_size(size)) {
		    add_line("Font size applied.");
		} else {
		    add_line("Font size must be between 10 and 20.");
		}
	} 
	else if (strcmp(option, "line_height") == 0) {
		int height = atoi(value);
		if (set_line_height(height)) {
		    add_line("Line height applied.");
		} else {
		    add_line("Line height must be between 15 and 25.");
		}
	} 
	else if (strcmp(option, "theme") == 0) {
		if (apply_theme(value)) {
		    add_line("Theme applied.");
		} else {
		    add_line("Unknown theme.");
		}
	} 
	else {
		add_line("Unknown setting option.");
	}


}

void cmd_translate(const char *args) {
    translation_pending = 1;

    if (!args || strlen(args) == 0) {
        add_line("Usage: translate <source> <target> <text>");
        return;
    }

    char source[16];
    char target[16];
    char text[256];

    // Parse: source, target, rest of line
    if (sscanf(args, "%15s %15s %[^\n]", source, target, text) != 3) {
        add_line("Invalid usage! Example: translate en fr Hello world");
        return;
    }

#ifdef __EMSCRIPTEN__
    // Store translate request in sessionStorage for JS to process
    EM_ASM({
        const source = UTF8ToString($0);
        const target = UTF8ToString($1);
        const text   = UTF8ToString($2);

        sessionStorage.setItem(
            "rekav_translate_request",
            JSON.stringify({
                source: source,
                target: target,
                text: text
            })
        );

        console.log("C -> JS translate request queued:", source, target, text);
    }, source, target, text);
#endif

    add_line("Translating…");
}



#ifdef __EMSCRIPTEN__
int poll_translate_result(void) {
    if (!translation_pending) return 0;

    // small internal buffer
    char buf[512];
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_translate_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) return;

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_translate_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {
        translation_pending = 0;
        add_line(buf);  // prints automatically
        return 1;
    }

    return 0;
}
#endif


#ifdef __EMSCRIPTEN__
int poll_forecast_result(void) {
    if (!weather_forecast_pending) return 0;

    // Internal buffer for JS → C string
    char buf[8192];  // big enough for multi-line text
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_weather_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) return;

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_weather_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {
        weather_forecast_pending = 0;

        // Split by '\n' and print line by line
        char *line = buf;
        while (*line) {
            char *next = strchr(line, '\n');
            if (!next) next = line + strlen(line);

            char tmp[512];
            int len = next - line;
            if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
            memcpy(tmp, line, len);
            tmp[len] = '\0';

            // Print each line to terminal
            add_line(tmp);

            line = (*next) ? next + 1 : next;
        }

        add_line(""); // extra space after forecast
        return 1;
    }

    return 0;
}
#endif








void cmd_fullscreen(const char *args) {

#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            let canvas = Module['canvas'];
            if (!document.fullscreenElement) {
                canvas.requestFullscreen();
            } else {
                document.exitFullscreen();
            }
        } catch(e) {
            console.error('Fullscreen toggle error:', e);
        }
    });
    add_line("Toggled fullscreen mode.");

    // --- Set font size for fullscreen ---
    int new_size = 14;

    if (settings.font) {
        TTF_CloseFont(settings.font); // free old font
    }

    settings.font = TTF_OpenFont("font.ttf", new_size);
    if (!settings.font) {
        add_line("Failed to set font for fullscreen.");
    } else {
        add_line("Font size set to 14 for fullscreen.");
        settings.font_size = new_size; // update settings
    }

#else
    add_line("Fullscreen not supported in native mode.");
#endif
}


// cmd implementation
void cmd_help(const char *args) {
    add_line("");
    add_line("--------------------------------------------------");
    add_line("                     HELP                         ");
    add_line("--------------------------------------------------");
    add_line("");

    add_line("Commands:");
    add_line("  help                         - Show this help message");
    add_line("  clear                        - Clear terminal output");
    add_line("  open <page>                  - Open a page in a new tab");
    add_line("  ls                           - List available pages and files");
    add_line("  sudo                         - Root login");
    add_line("  cat <file>                   - Display the content (info.txt...)");
    add_line("  exit                         - Exit root access");
    add_line("  translate <src> <tgt> <text> - Translate text via MyMemory API");
    add_line("  man <command>                - Show command documentation");
    
    add_line("");
    add_line("--------------------------------------------------");
    add_line("        Type 'man <command>' for more info       ");
    add_line("--------------------------------------------------");
    add_line("");
}


void cmd_clear(const char *args) {
    clear_terminal();
}

void cmd_exit(const char *args) {
    root_active = 0;  // exit root

    #ifdef __EMSCRIPTEN__
    // Clear all session storage (root + articles)
    emscripten_run_script("sessionStorage.clear();");
    #endif

    add_line("Access: guess");
}

void cmd_echo(const char *args) {
    if (args && strlen(args) > 0)
        add_line(args);
}


// List of allowed pages
const char *available_pages[] = {
    "gallery",
    "random",
    "shitpost",
    "about"
};
const int page_count = sizeof(available_pages) / sizeof(available_pages[0]);

void cmd_open(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: open <page>");
        return;
    }

    // Copy input to mutable buffer
    char page[128];
    strncpy(page, args, sizeof(page) - 1);
    page[sizeof(page) - 1] = '\0';

    // Remove ".html" if user typed it
    size_t len = strlen(page);
    if (len > 5 && strcmp(page + len - 5, ".html") == 0) {
        page[len - 5] = '\0';
    }

    // Check if page is in allowed list
    int found = 0;
    for (int i = 0; i < page_count; i++) {
        if (strcmp(page, available_pages[i]) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        add_line("Page not found or unavailable.");
        return;
    }

#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            var page = UTF8ToString($0);
            window.open('/' + page + '.html', '_blank');
        } catch(e) {
            console.error('cmd_open JS error:', e);
        }
    }, page);
#endif

    char buf[128];
    snprintf(buf, sizeof(buf), "Opening %s...", page);
    add_line(buf);
}



void cmd_cat(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: cat <file>");
        return;
    }

    FILE *f = fopen(args, "r");
    if (!f) {
        char buf[256];
        snprintf(buf, sizeof(buf), "cat: %s: No such fileeee", args);
        add_line(buf);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0; // remove newline
        add_line(line);
    }
    fclose(f);
}


void cmd_ls(const char *args) {
    add_line("gallery.html");
    add_line("random.html");
    add_line("shitpost.html");
    add_line("about.html");
    add_line("info.txt");
    add_line("manifesto.txt");
    add_line("welcome.txt");
}

void cmd_sudo(const char *args) {
    if (root_active) {
        add_line("You are already root.");
    } else {
        awaiting_sudo_password = 1;
        input_len = 0;
        input[0] = '\0';
    }
}

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

/* ---------- Terminal helpers ---------- */

void add_line(const char *text) {
    if (line_count >= MAX_LINES) {
        memmove(terminal, terminal + 1, sizeof(terminal) - sizeof(terminal[0]));
        line_count--;
    }
    strncpy(terminal[line_count++], text, MAX_LINE_LENGTH - 1);

    // Auto-scroll if already at bottom
    int input_height = 40;
	int line_height = settings.line_height;
    int max_visible_lines = (HEIGHT - input_height) / line_height;

    if (scroll_offset + max_visible_lines >= line_count - 1) {
        scroll_offset = line_count - max_visible_lines;
        if (scroll_offset < 0) scroll_offset = 0;
    }
}

void clear_terminal() {
    line_count = 0;
    scroll_offset = 0;
}

void execute_command(const char *cmd) {
    if (!cmd || strlen(cmd) == 0)
        return;

    char command[MAX_LINE_LENGTH];
    char *args = NULL;

    strncpy(command, cmd, MAX_LINE_LENGTH - 1);
    command[MAX_LINE_LENGTH - 1] = '\0';

    args = strchr(command, ' ');
    if (args) {
        *args = '\0';
        args++;
    }

    for (int i = 0; i < command_count; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            commands[i].func(args);
            return;
        }
    }

    add_line("Unknown command. Type 'help'");
}


void submit_input() {
	if (awaiting_sudo_password) {
		printf("DEBUG: Checking password: '%s'\n", input); // C-side debug

	if (strcmp(input, ROOT_PASSWORD) == 0) {
		root_active = 1;
		
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

		add_line("Root access granted.");

	} else {
		add_line("sudo: Access denied.");
	}
	
		// Reset password state
		awaiting_sudo_password = 0;
		input_len = 0;
		input[0] = '\0';
		return;
	}


    // Normal command mode
    char line[MAX_LINE_LENGTH + 3];
    snprintf(line, sizeof(line), "> %s", input);
    add_line(line);

    execute_command(input);

    input_len = 0;
    input[0] = '\0';
}


/* ---------- Rendering ---------- */

void draw_text(int x, int y, const char *text) {
    if (!settings.font) return;  // Safety check

    SDL_Color color = settings.font_color;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(settings.font, text, color);
    if (!surface) return; // Fail gracefully

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


void render() {
    // Draw background
	if (settings.background_texture) {
		SDL_RenderCopy(renderer, settings.background_texture, NULL, NULL);
	} else {
		SDL_SetRenderDrawColor(
		    renderer,
		    settings.background.r,
		    settings.background.g,
		    settings.background.b,
		    255
		);
		SDL_RenderClear(renderer);
	}

    int y = 10;
    int input_height = 40;
	int line_height = settings.line_height;
    int max_visible_lines = (HEIGHT - input_height) / line_height;

    // Clamp scroll_offset
    if (scroll_offset < 0) scroll_offset = 0;
    if (scroll_offset > line_count - max_visible_lines)
        scroll_offset = line_count - max_visible_lines;
    if (scroll_offset < 0) scroll_offset = 0;

    int start_line = scroll_offset;
    int end_line = start_line + max_visible_lines;
    if (end_line > line_count) end_line = line_count;

    // Draw terminal lines
    for (int i = start_line; i < end_line; i++) {
        draw_text(10, y, terminal[i]);
        y += line_height;
    }

    // ---------- Draw input prompt ----------
    char display[MAX_LINE_LENGTH + 2]; // +1 for cursor, +1 for '\0'

    if (awaiting_sudo_password) {
        // Mask input for password
        for (int i = 0; i < input_len; i++) {
            display[i] = '*';
        }
        display[input_len] = '\0';
    } else {
        // Normal input
        strcpy(display, input);
    }

    int len = strlen(display);

    // Clamp cursor_pos
    if (cursor_pos < 0) cursor_pos = 0;
    if (cursor_pos > len) cursor_pos = len;

    // Insert cursor underscore '_'
    memmove(display + cursor_pos + 1, display + cursor_pos, len - cursor_pos + 1); // shift tail including '\0'
    display[cursor_pos] = '_';

    char prompt_line[MAX_LINE_LENGTH + 32];
    snprintf(prompt_line, sizeof(prompt_line), "> %s", display);

    draw_text(10, HEIGHT - input_height + 10, prompt_line);

    SDL_RenderPresent(renderer);
}


/* ---------- Main loop ---------- */
void main_loop() {

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            running = 0;

        // Text input for normal typing
		if (e.type == SDL_TEXTINPUT) {
			int len = strlen(e.text.text);
			if (input_len + len < MAX_LINE_LENGTH) {
				// Move tail to make room
				memmove(input + cursor_pos + len, input + cursor_pos, input_len - cursor_pos + 1); // +1 for '\0'
				memcpy(input + cursor_pos, e.text.text, len);
				input_len += len;
				cursor_pos += len;
			}
		}


        if (e.type == SDL_KEYDOWN) {
            // Backspace handling
			if (e.key.keysym.sym == SDLK_BACKSPACE && cursor_pos > 0) {
				memmove(input + cursor_pos - 1, input + cursor_pos, input_len - cursor_pos + 1); // +1 for '\0'
				cursor_pos--;
				input_len--;
			}
			else if (e.key.keysym.sym == SDLK_DELETE && cursor_pos < input_len) {
				memmove(input + cursor_pos, input + cursor_pos + 1, input_len - cursor_pos);
				input_len--;
			}
            // Enter submits input
            else if (e.key.keysym.sym == SDLK_RETURN) {
                submit_input();  // <-- submit_input() now handles sudo/password logic
            }
            // Scroll terminal lines
            else if (e.key.keysym.sym == SDLK_UP) {
                scroll_offset--;
            }
            else if (e.key.keysym.sym == SDLK_DOWN) {
                scroll_offset++;
            }
            else if (e.key.keysym.sym == SDLK_LEFT && cursor_pos > 0) {
				cursor_pos--;
			}
			else if (e.key.keysym.sym == SDLK_RIGHT && cursor_pos < input_len) {
				cursor_pos++;
			}

        }

        // Mouse wheel scroll
        if (e.type == SDL_MOUSEWHEEL) {
            int max_visible_lines = (HEIGHT - 40) / 28;
            scroll_offset -= e.wheel.y * 3; // scroll 3 lines per tick
            if (scroll_offset < 0) scroll_offset = 0;
            if (scroll_offset > line_count - max_visible_lines)
                scroll_offset = line_count - max_visible_lines;
        }
    }
    

	poll_translate_result(); // prints automatically if translation arrived
	poll_forecast_result();


    render();
}


/* ---------- Init ---------- */

int main() {

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    init_settings();

    window = SDL_CreateWindow(
        "Rekav - 2026",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    font = TTF_OpenFont("font.ttf", 15); // Bigger font
    if (!font) {
        printf("Failed to load font\n");
        return 1;
    }
    TTF_SetFontStyle(font, TTF_STYLE_BOLD); // Bold font

    SDL_StartTextInput();
    cmd_cat("welcome.txt"); // prints banner at startup

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}



