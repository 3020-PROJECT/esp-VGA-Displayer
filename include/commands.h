# pragma once
# include "esp-client.h"

# define MAX_COMMANDS 4

typedef void (*CommandHandler)(void);

typedef struct command{
  String name;
  CommandHandler handler;
} _command;


typedef enum COMMAND_ID{
  IDENTIFY,
  CLEAR,
  PRINT,
  CHANGE_COLOR,
  UNKNOWN
} e_command_id;

void initCommands(void);
void handleCommandState(char c);