#include "colors.h"
#include <ESP32Video.h>

extern _dev  connected_dev;
extern VGA3BitI videodisplay;

_color    textColors[8] = {
  {"black", {0, 0, 0}},
  {"blue", {0, 0, 255}},
  {"green", {0, 255, 0}},
  {"cyan", {0, 255, 255}},
  {"red", {255, 0, 0}},
  {"magenta", {255, 0, 255}},
  {"yellow", {255, 255, 0}},
  {"white", {255, 255, 255}},
};


void  applyColor(void){
  static uint8_t lastColor = 7;

  for (uint8_t i = 0; i < 8; i++) {
    if (textColors[i].name == connected_dev.buffer) {
      uint8_t *rgb = textColors[i].rgb;
      videodisplay.setTextColor(videodisplay.RGB(rgb[0], rgb[1], rgb[2]));
      lastColor = i;
      break ;
    }
  }
  connected_dev.currentColor = lastColor;
}

void  readNewTextColor(char c){
  if (c == '\n'){
    connected_dev.read_into = SCREEN;
    connected_dev.state = EMPTY;
    applyColor();
    connected_dev.buffer = "";
    return;
  }
  connected_dev.buffer += c;
  return ;
}