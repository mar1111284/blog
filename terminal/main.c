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

#define ROOT_PASSWORD "rekav"

char input[MAX_LINE_LENGTH];
int input_len = 0;

char terminal[MAX_LINES][MAX_LINE_LENGTH];
int line_count = 0;
int scroll_offset = 0;
int running = 1;

// Struct to store each cmd
typedef void (*command_func)(const char *args);

typedef struct {
    const char *name;
    const char *desc;
    command_func func;
} Command;

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

    
};

int command_count = sizeof(commands) / sizeof(commands[0]);

void cmd_settings(const char *args) {
    char option[64];
    char value[64];

    // Display help if no args
    if (!args || strlen(args) == 0) {
        add_line("Usage: settings <option> <value>");
        add_line("Options:");
        add_line("  bg           - Background color (charcoal, dos_blue, pink, green, red)");
        add_line("  font_color   - Font color (white, green, pink, red, orange)");
        add_line("  font_size    - Font size (10-20)");
        add_line("  line_height  - Line height (15-25)");
        add_line("  theme        - Predefined themes (Default, MS-DOS, Barbie, Jurassic, Inferno)");
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
#else
    add_line("Fullscreen not supported in native mode.");
#endif
}


// cmd implementation
void cmd_help(const char *args) {
    add_line("Commands:");
    add_line("  help           - You find it.");
    add_line("  clear          - Clear terminal output");
    add_line("  open <page>    - Open a page in a new tab");
	add_line("  ls             - List available pages and files");
    add_line("  sudo           - Root login");
    add_line("  cat <file>     - Display the content of a file (info.txt, welcome.txt)");
    add_line("  exit           - Exit root access");

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

    for (int i = start_line; i < end_line; i++) {
        draw_text(10, y, terminal[i]);
        y += line_height;
    }
    
    if (awaiting_sudo_password) {
    // Show password masked
    char masked[MAX_LINE_LENGTH + 1];
    for (int i = 0; i < input_len; i++) masked[i] = '*';
		masked[input_len] = '\0';

		char prompt_line[MAX_LINE_LENGTH + 32];
		snprintf(prompt_line, sizeof(prompt_line), "Password: %s", masked);
		draw_text(10, HEIGHT - input_height + 10, prompt_line);
	} else {
		char prompt_line[MAX_LINE_LENGTH + 32];
		snprintf(prompt_line, sizeof(prompt_line), "> %s_", input);
		draw_text(10, HEIGHT - input_height + 10, prompt_line);
	}

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
            if (input_len < MAX_LINE_LENGTH - 1) {
                strcat(input, e.text.text);
                input_len += strlen(e.text.text);
            }
        }

        if (e.type == SDL_KEYDOWN) {
            // Backspace handling
            if (e.key.keysym.sym == SDLK_BACKSPACE && input_len > 0) {
                input[--input_len] = '\0';
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



