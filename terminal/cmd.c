#include "cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "data_codec.h"
#include "settings.h"
#include "base64.h"
#include "ascii_converter.h"

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

void cmd_help(const char *args) {

    add_terminal_line("", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("                     HELP                         ", LINE_FLAG_NONE);
    add_terminal_line("--------------------------------------------------", LINE_FLAG_NONE);
    add_terminal_line("", LINE_FLAG_NONE);

    add_terminal_line("Commands:", LINE_FLAG_NONE);
    add_terminal_line("  help                         - Show this help message", LINE_FLAG_NONE);
    add_terminal_line("  clear                        - Clear terminal output", LINE_FLAG_NONE);
    add_terminal_line("  open <page>                  - Open a page in a new tab", LINE_FLAG_NONE);
    add_terminal_line("  ls                           - List available pages and files", LINE_FLAG_NONE);
    add_terminal_line("  sudo                         - Root login", LINE_FLAG_NONE);
    add_terminal_line("  cat <file>                   - Display the content (info.txt...)", LINE_FLAG_NONE);
    add_terminal_line("  exit                         - Exit root access", LINE_FLAG_NONE);
    add_terminal_line("  translate <src> <tgt> <text> - Translate text via MyMemory API", LINE_FLAG_NONE);
    add_terminal_line("  man <command>                - Show command documentation", LINE_FLAG_NONE);
    add_terminal_line("  to_ascii <link>              - Convert image to ASCII", LINE_FLAG_NONE);
    add_terminal_line("  debug             			  - Get App context dump (dev)", LINE_FLAG_NONE);

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
		//add_line("Toggled fullscreen mode.");


	#else
		//add_line("Fullscreen not supported in native mode.");
	#endif
}

void cmd_echo(const char *args) {
    if (args && strlen(args) > 0) return;
        //add_line(args);
}

void cmd_open(const char *args) {
    if (!args || strlen(args) == 0) {
        //add_line("Usage: open <page>");
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
        //add_line("Page not found or unavailable.");
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
    //add_line(buf);
}

void cmd_cat(const char *args) {
    if (!args || strlen(args) == 0) {
        //add_line("Usage: cat <file>");
        return;
    }

    FILE *f = fopen(args, "r");
    if (!f) {
        char buf[256];
        snprintf(buf, sizeof(buf), "cat: %s: No such fileeee", args);
        //add_line(buf);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0; // remove newline
        //add_line(line);
    }
    fclose(f);
}


void cmd_ls(const char *args) {
	return;
	/*
    add_line("gallery.html");
    add_line("random.html");
    add_line("shitpost.html");
    add_line("about.html");
    add_line("info.txt");
    add_line("welcome.txt");
    */
}

void cmd_sudo(const char *args) {
    if (_root_active) {
        //add_line("You are already root.");
    } else {
        _awaiting_sudo_password = 1;
    }
}

void cmd_man(const char *args) {
    if (!args || strlen(args) == 0) {
        //add_line("Usage: man <command>");
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
                //add_line(buf);
                line = (*next) ? next + 1 : next;
            }
            return;
        }
    }

    //add_line("No manual entry for this command.");
}

void cmd_settings(const char *args) {
    char option[64];
    char value[64];

	/*if (!args || strlen(args) == 0) {
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
	*/


}

// Command: to_ascii <image_url>
void cmd_to_ascii(const char *args) {
    _image_processing_pending = 1;  // flag for polling

    if (!args || strlen(args) == 0) {
        //add_line("Usage: to_ascii <image_url> [download=1 wide=300 font_size=6 bg=black color=white name=output]");
        _image_processing_pending = 0;
        return;
    }

    // Split first word (URL) and the rest (options)
    char url[1024];
    char options_str[1024] = {0};
    sscanf(args, "%1023s %[^\n]", url, options_str);

    // Basic URL validation
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        //add_line("Error: Please provide a valid http/https URL");
        _image_processing_pending = 0;
        return;
    }

    // Parse options
    ExportOptions opts;
    opts.chars_wide = 130;    // default
    opts.font_size = 6;       // default
    opts.fg[0]=255; opts.fg[1]=255; opts.fg[2]=255; // default white
    opts.bg[0]=0; opts.bg[1]=0; opts.bg[2]=0;       // default black
    opts.filename = "ascii_highres.png";

    _image_download_pending = 0;

    char key[64], val[64];
    const char *ptr = options_str;
    while (*ptr) {
        if (sscanf(ptr, "%63[^=]=%63s", key, val) == 2) {
            if (strcmp(key, "download") == 0 && atoi(val) != 0) {
                _image_download_pending = 1;
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
            }
        }

        // Skip to next option
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

    //add_line("Fetching and processing image...");
    //add_line("(This may take a few seconds depending on image size)");

    // Save opts globally if download is requested
    if (_image_download_pending) {
        // copy opts to a global variable for poll_image_result
        global_opts = opts;
    }
}


void cmd_weather(const char *args) {
    _weather_forecast_pending = 1;
    /*if (!args || strlen(args) == 0) {
        add_line("Usage: weather <city>");
        add_line("Available cities:");
        for (int i = 0; i < city_count; i++) {
            add_line(cities[i].name);
        }
        return;
    }*/

    // Find city in array (case-insensitive)
    City *selected = NULL;
    for (int i = 0; i < city_count; i++) {
        if (strcasecmp(args, cities[i].name) == 0) { // <-- case-insensitive
            selected = &cities[i];
            break;
        }
    }

    if (!selected) {
        //add_line("City not found. Type 'weather' to see available cities.");
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
    //add_line(buf);
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


