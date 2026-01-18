#include "cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

void cmd_version(const char *args) {

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("                    VERSION                       ", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("", LINE_FLAG_NONE);

    add_terminal_line("Components:", LINE_FLAG_NONE);

    char buf[128];

    snprintf(buf, sizeof(buf), "  App               : %s", VERSION_INFO.app);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  Terminal          : %s", VERSION_INFO.terminal);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  Weather Forecast  : %s", VERSION_INFO.weather_forecast);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  ASCII Converter   : %s", VERSION_INFO.ascii_converter);
    add_terminal_line(buf, LINE_FLAG_NONE);

    snprintf(buf, sizeof(buf), "  Translator        : %s", VERSION_INFO.translator);
    add_terminal_line(buf, LINE_FLAG_NONE);

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("            Type 'help' to see commands           ", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("", LINE_FLAG_NONE);
}


void cmd_help(const char *args) {

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("                     HELP                         ", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
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

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("        Type 'man <command>' for more info       ", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
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

void cmd_settings(const char *args) {
    char option[64];
    char value[64];

    if (!args || strlen(args) == 0) {
        add_terminal_line("", LINE_FLAG_NONE);
        add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
        add_terminal_line("                   SETTINGS                       ", LINE_FLAG_NONE);
        add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
        add_terminal_line("", LINE_FLAG_NONE);

        add_terminal_line("Usage: settings <option> <value>", LINE_FLAG_SYSTEM);
        add_terminal_line("", LINE_FLAG_NONE);
        add_terminal_line("Options:", LINE_FLAG_NONE);
        add_terminal_line("  bg           - Background color (charcoal, dos_blue, pink, green, red)", LINE_FLAG_NONE);
        add_terminal_line("  font_color   - Font color (white, green, pink, red, orange)", LINE_FLAG_NONE);
        add_terminal_line("  font_size    - Font size (10-20)", LINE_FLAG_NONE);
        add_terminal_line("  line_height  - Line height (15-25)", LINE_FLAG_NONE);
        add_terminal_line("  theme        - Predefined themes (default, msdos, barbie, jurassic, inferno)", LINE_FLAG_NONE);

        add_terminal_line("", LINE_FLAG_NONE);
        add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
        add_terminal_line("       Example: settings bg charcoal            ", LINE_FLAG_NONE);
        add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
        add_terminal_line("", LINE_FLAG_NONE);

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

    ExportOptions opts;
    opts.chars_wide = 130;
    opts.font_size = 6;
    opts.fg[0] = 255; opts.fg[1] = 255; opts.fg[2] = 255;
    opts.bg[0] = 0;   opts.bg[1] = 0;   opts.bg[2] = 0;
    opts.filename = "ascii_highres.png";

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

    if (_image_download_pending) {
        global_opts = opts;
    }
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
    snprintf(buf, sizeof(buf), "Fetching weather for %s…", selected->name);
    add_terminal_line(buf, LINE_FLAG_SYSTEM);
}


void cmd_translate(const char *args) {
    _translation_pending = 1;

    if (!args || strlen(args) == 0) {
        //add_line("Usage: translate <source> <target> <text>");
        return;
    }

    char source[16];
    char target[16];
    char text[256];

    // Parse: source, target, rest of line
    if (sscanf(args, "%15s %15s %[^\n]", source, target, text) != 3) {
        //add_line("Invalid usage! Example: translate en fr Hello world");
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

    //add_line("Translating…");
}


