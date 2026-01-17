#ifndef CMD_H
#define CMD_H

#include <stdint.h>

// Forward declarations
void execute_command(const char *cmd);

// Command struct
typedef void (*command_func)(const char *args);

typedef struct {
    const char *name;
    const char *desc;
    command_func func;
} Command;

// Command registry
extern Command commands[];
extern int command_count;

// Command implementations
void cmd_help(const char *args);
void cmd_clear(const char *args);
void cmd_echo(const char *args);
void cmd_open(const char *args);
void cmd_ls(const char *args);
void cmd_cat(const char *args);
void cmd_sudo(const char *args);
void cmd_exit(const char *args);
void cmd_settings(const char *args);
void cmd_fullscreen(const char *args);
void cmd_translate(const char *args);
void cmd_man(const char *args);
void cmd_weather(const char *args);
void cmd_to_ascii(const char *args);

#endif // CMD_H

