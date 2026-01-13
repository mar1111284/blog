#include <SDL.h>
#include <SDL_ttf.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <string.h>
#include <stdio.h>

#define WIDTH 600
#define HEIGHT 360

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

#include <emscripten/emscripten.h>


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
void cmd_about(const char *args);
void cmd_echo(const char *args);
void cmd_open(const char *args);
void execute_command(const char *cmd);
void cmd_ls(const char *args);
void cmd_cat(const char *args);
void cmd_sudo(const char *args);
void submit_input();

Command commands[] = {
    {"help",  "Show help",      cmd_help},
    {"clear", "Clear terminal", cmd_clear},
    {"about", "About terminal", cmd_about},
    {"echo",  "Print text",     cmd_echo},
    {"open",  "Open a page",    cmd_open},
    {"ls",    "List",    		cmd_ls},
	{"cat",    "open file",    	cmd_cat},
    {"sudo",    "root login",   cmd_sudo},
    
};

int command_count = sizeof(commands) / sizeof(commands[0]);

// cmd implementation
void cmd_help(const char *args) {
    add_line("Commands:");
    add_line("  help           - You find it.");
    add_line("  clear          - Clear terminal output");
    add_line("  open <page>    - Open a page in a new tab");
	add_line("  ls             - List available pages and files");
    add_line("  sudo           - Root login");
    add_line("  cat <file>     - Display the content of a file (info.txt, manifesto.txt, welcome.txt)");

}

void cmd_clear(const char *args) {
    clear_terminal();
}

void cmd_about(const char *args) {
    add_line("Fake WebAssembly Terminal");
    add_line("Built with C, SDL2 and Emscripten");
    add_line("Part of rekav-blog");
}

void cmd_echo(const char *args) {
    if (args && strlen(args) > 0)
        add_line(args);
}


void cmd_open(const char *args) {
    if (!args || strlen(args) == 0) {
        add_line("Usage: open <page>");
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
	}, args);
	#endif
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
    int line_height = 19;
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
    SDL_Color color = {255, 255, 255, 255}; // White text
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render() {
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255); // Blue background
    SDL_RenderClear(renderer);

    int y = 10;
    int input_height = 40;
    int line_height = 19;
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



