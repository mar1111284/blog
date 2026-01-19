#include "cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "editor.h"

#include "global.h"
#include "data_codec.h"
#include "settings.h"
#include "base64.h"
#include "ascii_converter.h"
#include <emscripten/emscripten.h>


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
    {"to_ascii", "Convert image from URL to ASCII art", cmd_to_ascii},
    {"debug", "App debug dump",app_debug_dump },
    {"version", "Version",cmd_version },
    {"log", "Log message",cmd_log },
    {"editor", "Open editor",cmd_editor },
};

int command_count = sizeof(commands) / sizeof(commands[0]);

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

    add_terminal_line("Unknown command. Type 'help'", LINE_FLAG_SYSTEM);
}

void cmd_log(const char *args) {
    if (!args || strlen(args) == 0) {
        add_terminal_line("Usage: log [visible=0|1] [list]", LINE_FLAG_SYSTEM);
        return;
    }

    char key[64], val[64];
    const char *ptr = args;
    BOOL show_list = FALSE;

    while (*ptr) {
        if (sscanf(ptr, "%63[^= ]=%63s", key, val) == 2) {
            if (strcmp(key, "visible") == 0) {
                _terminal.log_visible = (atoi(val) != 0) ? TRUE : FALSE;
                char buf[128];
                snprintf(buf, sizeof(buf), "Terminal log visibility set to %s", _terminal.log_visible ? "ON" : "OFF");
                add_terminal_line(buf, LINE_FLAG_SYSTEM);
            }
        } else {
            // handle flags without "=" like "list"
            char word[64];
            if (sscanf(ptr, "%63s", word) == 1) {
                if (strcmp(word, "list") == 0) {
                    show_list = TRUE;
                }
            }
        }

        const char *next = strchr(ptr, ' ');
        if (!next) break;
        ptr = next + 1;
    }

    if (show_list) {
        add_terminal_line("\n--- Terminal Logs ---", LINE_FLAG_SYSTEM);

        for (int i = 0; i < _terminal.log_count; i++) {
            TerminalLog *log = &_terminal.logs[i];
            char line[512];
            const char *type_str = "INFO";
            switch (log->type) {
                case LOG_INFO:    type_str = "INFO"; break;
                case LOG_WARNING: type_str = "WARN"; break;
                case LOG_ERROR:   type_str = "ERROR"; break;
                case LOG_FATAL:   type_str = "FATAL"; break;
            }

            snprintf(line, sizeof(line), "[%s] (%s) #%d: %s",
                     log->datetime, type_str, log->id, log->text);

            add_terminal_line(line, LINE_FLAG_NONE);
        }

        add_terminal_line("--- End of Logs ---\n", LINE_FLAG_SYSTEM);
    }
}



void cmd_version(const char *args) {

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("                    VERSION                       ", LINE_FLAG_SYSTEM);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("", LINE_FLAG_NONE);

    add_terminal_line("Components:", LINE_FLAG_NONE);

    char buf[128];

    snprintf(buf, sizeof(buf), "  App               : %s (%s)", 
             app.version.app.version, app.version.app.release_date);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  Terminal          : %s (%s)", 
             app.version.terminal.version, app.version.terminal.release_date);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  Weather Forecast  : %s (%s)", 
             app.version.weather_forecast.version, app.version.weather_forecast.release_date);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  ASCII Converter   : %s (%s)", 
             app.version.ascii_converter.version, app.version.ascii_converter.release_date);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  Translator        : %s (%s)", 
             app.version.translator.version, app.version.translator.release_date);
    add_terminal_line(buf, LINE_FLAG_NONE);

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("            Type 'help' to see commands           ", LINE_FLAG_SYSTEM);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("", LINE_FLAG_NONE);
}


void cmd_help(const char *args) {

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("                     HELP                         ", LINE_FLAG_SYSTEM);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("", LINE_FLAG_NONE);

    add_terminal_line("Commands:", LINE_FLAG_NONE);
    add_terminal_line("  help                         - Show this help message", LINE_FLAG_NONE);
    add_terminal_line("  version                      - Show application version info", LINE_FLAG_NONE);
    add_terminal_line("  clear                        - Clear terminal output", LINE_FLAG_NONE);
    add_terminal_line("  open <page>                  - Open a page in a new tab", LINE_FLAG_NONE);
    add_terminal_line("  ls                           - List available pages and files", LINE_FLAG_NONE);
    add_terminal_line("  sudo                         - Root login", LINE_FLAG_NONE);
    add_terminal_line("  cat <file>                   - Display the content (info.txt...)", LINE_FLAG_NONE);
    add_terminal_line("  exit                         - Exit root access", LINE_FLAG_NONE);
    add_terminal_line("  translate <src> <tgt> <text> - Translate text via MyMemory API", LINE_FLAG_NONE);
    add_terminal_line("  man <command>                - Show command documentation", LINE_FLAG_NONE);
    add_terminal_line("  to_ascii <link>              - Convert image to ASCII", LINE_FLAG_NONE);
    add_terminal_line("  debug                        - Get App context dump (dev)", LINE_FLAG_NONE);
    add_terminal_line("  log                          - Get log messages", LINE_FLAG_NONE);
    add_terminal_line("  editor                       - Write, Code, C compilator", LINE_FLAG_NONE);
    
    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("        Type 'man <command>' for more info       ", LINE_FLAG_SYSTEM);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("", LINE_FLAG_NONE);
}


void cmd_clear(const char *args) {
    clear_terminal();
}

void cmd_exit(const char *args) {
    _root_active = FALSE;
    #ifdef __EMSCRIPTEN__
    	emscripten_run_script("sessionStorage.clear();");
    #endif
    add_terminal_line("Access: guess", LINE_FLAG_SYSTEM);
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
		add_terminal_line("Fulscreen mode activated", LINE_FLAG_SYSTEM);

	#else
		add_terminal_line("Fullscreen not supported in native mode.", LINE_FLAG_ERROR);
	#endif
}

void cmd_echo(const char *args) {
    if (args && strlen(args) > 0) return;
        add_terminal_line(args, LINE_FLAG_NONE);
}

void cmd_open(const char *args) {
    if (!args || strlen(args) == 0) {
        add_terminal_line("Usage: open <page>", LINE_FLAG_SYSTEM);
        return;
    }

    char page[128];
    strncpy(page, args, sizeof(page) - 1);
    page[sizeof(page) - 1] = '\0';

    size_t len = strlen(page);
    if (len > PAGE_EXT_LEN && strcmp(page + len - PAGE_EXT_LEN, PAGE_EXT) == 0) {
        page[len - PAGE_EXT_LEN] = '\0';
    }

    int found = 0;
    for (int i = 0; i < page_count; i++) {
        if (strcmp(page, available_pages[i]) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        add_terminal_line("Page not found or unavailable.", LINE_FLAG_SYSTEM);
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
    add_terminal_line(buf, LINE_FLAG_SYSTEM);
}

void cmd_cat(const char *args) {
    if (!args || strlen(args) == 0) {
        add_terminal_line("Usage: cat <file>", LINE_FLAG_SYSTEM);
        return;
    }

    FILE *f = fopen(args, "r");
    if (!f) {
        char buf[256];
        snprintf(buf, sizeof(buf), "cat: %s: No such file", args);
        add_terminal_line(buf, LINE_FLAG_SYSTEM);
        return;
    }

    char line[CAT_LINE_MAX];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        add_terminal_line(line, LINE_FLAG_NONE);
    }

    fclose(f);
}

void cmd_ls(const char *args) {
    for (int i = 0; i < page_count; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s.html", available_pages[i]);
        add_terminal_line(buf, LINE_FLAG_NONE);
    }

    add_terminal_line("info.txt", LINE_FLAG_NONE);
    add_terminal_line("welcome.txt", LINE_FLAG_NONE);
}


void cmd_sudo(const char *args) {
    if (_root_active) {
        add_terminal_line("You are already root.", LINE_FLAG_SYSTEM);
    } else {
        _awaiting_sudo_password = TRUE;
    }
}

void cmd_man(const char *args) {
    if (!args || strlen(args) == 0) {
        add_terminal_line("Usage: man <command>", LINE_FLAG_SYSTEM);
        return;
    }

    for (int i = 0; i < man_db_size; i++) {
        if (strcmp(args, man_db[i].cmd) == 0) {
            const char *desc = man_db[i].description;
            const char *line = desc;
            while (*line) {
                const char *next = strchr(line, '\n');
                if (!next) next = line + strlen(line);
                char buf[MAN_LINE_BUF];
                int len = next - line;
                if (len >= sizeof(buf)) len = sizeof(buf) - 1;
                memcpy(buf, line, len);
                buf[len] = '\0';
                add_terminal_line(buf, LINE_FLAG_NONE);
                line = (*next) ? next + 1 : next;
            }
            return;
        }
    }

    add_terminal_line("No manual entry for this command.", LINE_FLAG_SYSTEM);
}

void cmd_editor(const char *args) {
    // If user just typed "editor" without args
    if (!args || strlen(args) == 0) {
        add_terminal_line("Opening editor...", LINE_FLAG_SYSTEM);
        if(g_editor.init != 1) editor_init();
        g_editor.active = 1;
        return;
    }

    // Optional: future flags parsing, e.g., "editor newfile.txt"
    char buf[128];
    snprintf(buf, sizeof(buf), "Unknown argument: '%s'", args);
    add_terminal_line(buf, LINE_FLAG_WARNING);

    add_terminal_line("Usage: editor", LINE_FLAG_SYSTEM);
    add_terminal_line("Opens the text editor overlay. Escape to exit.", LINE_FLAG_SYSTEM);
}

void cmd_settings(const char *args) {
    char option[64];
    char value[64];

	if (!args || strlen(args) == 0) {
		print_settings_help();
		return;
	}

    if (args && strcmp(args, "--list-themes") == 0) {
        list_themes();
        return;
    }

    if (args && strcmp(args, "--list-colors") == 0) {
        list_colors();
        return;
    }

    int matched = sscanf(args, "%63s %63s", option, value);
    if (matched != 2) {
        add_terminal_line("Invalid usage! Example: settings font_size 18", LINE_FLAG_SYSTEM);
        return;
    }

    if (strcmp(option, "bg") == 0) {
        if (set_background_color(&_terminal.settings, value)) {
            add_terminal_line("Background color applied.", LINE_FLAG_SYSTEM);
        } else {
            add_terminal_line("Unknown background color.", LINE_FLAG_SYSTEM);
        }
    } 
    else if (strcmp(option, "font_color") == 0) {
        if (set_font_color(&_terminal.settings, value)) {
            add_terminal_line("Font color applied.", LINE_FLAG_SYSTEM);
        } else {
            add_terminal_line("Unknown font color.", LINE_FLAG_SYSTEM);
        }
    } 
    else if (strcmp(option, "font_size") == 0) {
        int size = atoi(value);
        if (set_font_size(&_terminal.settings, size)) {
            add_terminal_line("Font size applied.", LINE_FLAG_SYSTEM);
        } else {
            add_terminal_line("Font size must be between 10 and 20.", LINE_FLAG_SYSTEM);
        }
    } 
    else if (strcmp(option, "line_height") == 0) {
        int height = atoi(value);
        if (set_line_height(&_terminal.settings, height)) {
            add_terminal_line("Line height applied.", LINE_FLAG_SYSTEM);
        } else {
            add_terminal_line("Line height must be between 15 and 25.", LINE_FLAG_SYSTEM);
        }
    } 
    else if (strcmp(option, "theme") == 0) {
        if (apply_theme(&_terminal.settings, value)) {
            add_terminal_line("Theme applied.", LINE_FLAG_SYSTEM);
        } else {
            add_terminal_line("Unknown theme.", LINE_FLAG_SYSTEM);
        }
    } 
    else {
        add_terminal_line("Unknown setting option.", LINE_FLAG_SYSTEM);
    }
}

void cmd_to_ascii(const char *args) {
    _image_processing_pending = 1;

    if (!args || strlen(args) == 0) {
        add_terminal_line("Usage: to_ascii <image_url> [download=1 wide=300 font_size=6 bg=black color=white name=output ramp=1]", LINE_FLAG_SYSTEM);
        _image_processing_pending = 0;
        return;
    }

    char url[1024];
    char options_str[1024] = {0};
    sscanf(args, "%1023s %[^\n]", url, options_str);

    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        add_terminal_line("Error: Please provide a valid http/https URL", LINE_FLAG_SYSTEM);
        _image_processing_pending = 0;
        return;
    }

	// Default
    ExportOptions opts;
    opts.chars_wide = 130;
    opts.font_size = 7;
    opts.ramp = RAMP_1;
    opts.fg[0] = 255; opts.fg[1] = 255; opts.fg[2] = 255;
    opts.bg[0] = 0;   opts.bg[1] = 0;   opts.bg[2] = 0;
    opts.filename = "ascii_art.png";

    _image_download_pending = 0;

    char key[64], val[64];
    const char *ptr = options_str;
    while (*ptr) {
        if (sscanf(ptr, "%63[^=]=%63s", key, val) == 2) {
            if (strcmp(key, "download") == 0 && atoi(val) != 0) {
                _image_download_pending = TRUE;
            } else if (strcmp(key, "wide") == 0) {
                opts.chars_wide = atoi(val);
            } else if (strcmp(key, "font_size") == 0) {
                opts.font_size = atoi(val);
            } else if (strcmp(key, "bg") == 0) {
                parse_color(val, &opts.bg[0], &opts.bg[1], &opts.bg[2]);
            } else if (strcmp(key, "color") == 0) {
                parse_color(val, &opts.fg[0], &opts.fg[1], &opts.fg[2]);
            } else if (strcmp(key, "name") == 0) {
                opts.filename = strdup(val);
            } else if (strcmp(key, "ramp") == 0) {
                int r = atoi(val);
                if      (r == 1) opts.ramp = RAMP_1;
                else if (r == 2) opts.ramp = RAMP_2;
                else if (r == 3) opts.ramp = RAMP_3;
                else if (r == 4) opts.ramp = RAMP_4;
				else if (r == 5) opts.ramp = RAMP_5;
				else if (r == 6) opts.ramp = RAMP_6;
                else             opts.ramp = RAMP_1;
            }
        }

        const char *next = strchr(ptr, ' ');
        if (!next) break;
        ptr = next + 1;
    }
	
	#ifdef __EMSCRIPTEN__
		// Store the request in sessionStorage
		EM_ASM({
		    const url = UTF8ToString($0);
		    sessionStorage.setItem("rekav_image_request", url);
		    console.log("C -> JS image request queued:", url);
		}, url);
	#endif

    add_terminal_line("Fetching and processing image...", LINE_FLAG_SYSTEM);
    add_terminal_line("(This may take a few seconds depending on image size)", LINE_FLAG_SYSTEM);
    global_opts = opts;
}


void cmd_weather(const char *args) {
    _weather_forecast_pending = 1;

    if (!args || strlen(args) == 0) {
        add_terminal_line("Usage: weather <city>", LINE_FLAG_SYSTEM);
        add_terminal_line("Available cities:", LINE_FLAG_SYSTEM);
        for (int i = 0; i < city_count; i++) {
            add_terminal_line(cities[i].name, LINE_FLAG_NONE);
        }
        return;
    }

    City *selected = NULL;
    for (int i = 0; i < city_count; i++) {
        if (strcasecmp(args, cities[i].name) == 0) {
            selected = &cities[i];
            break;
        }
    }

    if (!selected) {
        return;
    }

	#ifdef __EMSCRIPTEN__
		EM_ASM({
		    const lat = $0;
		    const lon = $1;
		    const city = UTF8ToString($2);
		    sessionStorage.setItem(
		        "rekav_weather_request",
		        JSON.stringify({ latitude: lat, longitude: lon, city: city })
		    );
		    console.log("C -> JS weather request queued:", city, lat, lon);
		}, selected->lat, selected->lon, selected->name);
	#endif

    char buf[128];
    add_terminal_line(buf, LINE_FLAG_SYSTEM);
}


void cmd_translate(const char *args) {
    _translation_pending = 1;

    if (!args || strlen(args) == 0) {
        add_log("translate: no arguments provided", LOG_WARNING);
        return;
    }

    char source[16];
    char target[16];
    char text[256];

    if (sscanf(args, "%15s %15s %[^\n]", source, target, text) != 3) {
        add_log("translate: invalid arguments format", LOG_ERROR);
        return;
    }

    char buf[512];
    add_log(buf, LOG_INFO);

    #ifdef __EMSCRIPTEN__
        EM_ASM({
            const source = UTF8ToString($0);
            const target = UTF8ToString($1);
            const text   = UTF8ToString($2);

            sessionStorage.setItem(
                "rekav_translate_request",
                JSON.stringify({source: source, target: target, text: text})
            );

            console.log("C -> JS translate request queued:", source, target, text);
        }, source, target, text);
    #endif
}

void app_debug_dump(const char *arg)
{
	
	add_terminal_line("\n", LINE_FLAG_NONE);
    add_terminal_line("----------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("          FULL AppContext DEBUG DUMP          ", LINE_FLAG_SYSTEM | LINE_FLAG_HIGHLIGHT);
    add_terminal_line("----------------------------------------------", LINE_FLAG_SYSTEM);

    // ── Config ──────────────────────────────────────────────────────────────
    add_terminal_line("Config:", LINE_FLAG_SYSTEM);
    char buf[256];
    snprintf(buf, sizeof(buf), "  viewport_width:  %d", app.config.viewport_width);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  viewport_height: %d", app.config.viewport_height);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  is_mobile_view:  %s", app.is_mobile_view ? "YES" : "NO");
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  font_size:       %d", app.config.font_size);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  fullscreen:      %d", app.fullscreen);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // ── SDL Context ─────────────────────────────────────────────────────────
    add_terminal_line("SDL Context:", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  window:          %p", (void*)app.sdl.window);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  renderer:        %p", (void*)app.sdl.renderer);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // ── Terminal - Core ─────────────────────────────────────────────────────
    add_terminal_line("Terminal - Core:", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  running:              %d", app.terminal.running);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  width / height:       %d x %d", app.terminal.width, app.terminal.height);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  line_count:           %d / %d", app.terminal.line_count, MAX_LINES);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  scroll_offset_px:     %d", app.terminal.scroll_offset_px);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  max_scroll:           %d", app.terminal.max_scroll);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  dirty:                %d", app.terminal.dirty);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // ── Terminal - Settings ─────────────────────────────────────────────────
    add_terminal_line("Terminal Settings:", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  theme_name:           %s", app.terminal.settings.theme_name ? app.terminal.settings.theme_name : "(null)");
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  font:                 %p", (void*)app.terminal.settings.font);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  font_size:            %d", app.terminal.settings.font_size);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  line_height:          %d", app.terminal.settings.line_height);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  letter_spacing:       %d", app.terminal.settings.letter_spacing);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // Colors (RGB values)
    snprintf(buf, sizeof(buf), "  background:           (%d,%d,%d,%d)",
             app.terminal.settings.background.r, app.terminal.settings.background.g,
             app.terminal.settings.background.b, app.terminal.settings.background.a);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  font_color:           (%d,%d,%d,%d)",
             app.terminal.settings.font_color.r, app.terminal.settings.font_color.g,
             app.terminal.settings.font_color.b, app.terminal.settings.font_color.a);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  line_bg_color:        (%d,%d,%d,%d)",
             app.terminal.settings.line_background_color.r,
             app.terminal.settings.line_background_color.g,
             app.terminal.settings.line_background_color.b,
             app.terminal.settings.line_background_color.a);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // ── Terminal - Current Input ────────────────────────────────────────────
    add_terminal_line("Current Input:", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  cursor_pos:           %d", app.terminal.input.cursor_pos);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  prompt_len:           %d", app.terminal.input.prompt_len);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  buffer length:        %zu", strlen(app.terminal.input.buffer));
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // ── Terminal - History ──────────────────────────────────────────────────
    add_terminal_line("Command History:", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  count:                %d / %d", app.terminal.history.count, MAX_HISTORY_COMMANDS);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  current pos:          %d", app.terminal.history.pos);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    // ── Global Flags ────────────────────────────────────────────────────────
    add_terminal_line("Global Flags:", LINE_FLAG_SYSTEM);
    snprintf(buf, sizeof(buf), "  root_active:          %d", app.root_active);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  awaiting_sudo_password: %d", app.awaiting_sudo_password);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  translation_pending:  %d", app.translation_pending);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  weather_forecast_pending: %d", app.weather_forecast_pending);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  image_processing_pending: %d", app.image_processing_pending);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  image_download_pending: %d", app.image_download_pending);
    add_terminal_line(buf, LINE_FLAG_NONE);
    snprintf(buf, sizeof(buf), "  last_activity:        %u ms ago", SDL_GetTicks() - app.last_activity);
    add_terminal_line(buf, LINE_FLAG_NONE);
    add_terminal_line("\n", LINE_FLAG_NONE);

    add_terminal_line("----------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("               End of debug dump              ", LINE_FLAG_SYSTEM);
    add_terminal_line("----------------------------------------------", LINE_FLAG_SYSTEM);
    add_terminal_line("\n", LINE_FLAG_NONE);
    
}

