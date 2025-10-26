#pragma once
#include <Arduino.h>
#include "commands.h"

# ifndef e_command_state
typedef enum COMMAND_STATE{
  EMPTY,
  READING,
  SPECIFIED,
} e_command_state;
# endif


# ifndef e_read_destanies
typedef enum READ_DESTANIES{
  DEV_NAME,
  SCREEN,
  COLOR,
  NONE,
} e_read_destanies;
# endif

typedef struct dev {
  String  deviceName;
  String  deviceIp;
  String  buffer;
  String  command;
  uint8_t currentColor;
  bool    isIdentified;
  e_command_state state;
  e_read_destanies read_into;
} _dev;