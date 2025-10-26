#include <Arduino.h>
#include "commands.h"
#include "display.h"

_command  commands[MAX_COMMANDS];

extern _dev  connected_dev;

e_command_id  getCommandID(void){
  uint8_t id;
  
  for (id = 0; id < MAX_COMMANDS; id++){
    if (connected_dev.command == commands[id].name)
      return (e_command_id)id;
  }

  return UNKNOWN;
}

void  execCommands(void) {
  uint8_t id;
  
  for (id = 0; id < MAX_COMMANDS; id++){
    if (connected_dev.command == commands[id].name){
      commands[id].handler();
      break;
    }
  }
  if (id == UNKNOWN)
    connected_dev.state = EMPTY;
  connected_dev.command = "";
}

void handleCommandState(char c){
  e_command_id id;

  if ((c != ' ') && (c != '\n')){
    connected_dev.command += c;
    return;
  }
  connected_dev.state = SPECIFIED;
  execCommands();
}
void execClearScreen(void){
  clearScreen(true);
  putPrompt();
  connected_dev.state = EMPTY;
}

void execIdentify(void){
  if (connected_dev.isIdentified){
    connected_dev.state = EMPTY;
    return;
  }
  connected_dev.read_into = DEV_NAME;
  connected_dev.deviceName = "";
}

void execPrint(void){
  connected_dev.read_into = SCREEN;
}

void execColorChange(void){
  connected_dev.read_into = COLOR;
  connected_dev.buffer = "";
}

void initCommands(void){
  commands[CLEAR].name = "/clear";
  commands[CLEAR].handler = execClearScreen;
  commands[IDENTIFY].name = "/identify";
  commands[IDENTIFY].handler = execIdentify;
  commands[PRINT].name = "/print";
  commands[PRINT].handler = execPrint;
  commands[CHANGE_COLOR].name = "/color";
  commands[CHANGE_COLOR].handler = execColorChange;
}