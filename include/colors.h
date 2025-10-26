# pragma once
# include <Arduino.h>
# include "commands.h"
# include "esp-client.h"

typedef struct color{
  String name;
  uint8_t rgb[3];
} _color;

extern _color    textColors[8];

void  applyColor(void);
void  readNewTextColor(char c);