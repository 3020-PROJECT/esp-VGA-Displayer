#include <WiFi.h>
#include <ESP32Video.h>
#include <Ressources/CodePage437_8x8.h>
#include "images.h"
#include "colors.h"
#include "esp-client.h"

// ========== WiFi Config ==========
const char* ssid     = "MAKERS_IOT";
const char* password = "1337@IOT";
const uint16_t serverPort = 1337;

// ========== VGA pins ==========
#define RED   14
#define GREEN 19
#define BLUE  27
#define HSYNC 32
#define VSYNC 33

// ========== Screen params ==========
#define NATIVE_TEXT_MODE 0
#define FONT_HIGHT 8
#define FONT_WIDTH 8
#define COLS 80   // 640 / 8
#define ROWS 49   // (400 / 8) - 1

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 400


//VGA Device
VGA3BitI videodisplay;
int16_t   cursor_x, cursor_y;

// ServerConfigs
WiFiServer server(serverPort);
WiFiClient client;
_dev  connected_dev;

void printCentered(const char* s, int16_t row);

// ========== Helper functions ==========
void setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
  # if NATIVE_TEXT_MODE
    videodisplay.setCursor(cursor_x, cursor_y);
  # else
    videodisplay.setCursor(cursor_x * FONT_WIDTH, cursor_y * FONT_HIGHT + FONT_HIGHT);
  # endif
}

void  displayConnectedDeviceName(void){
  uint8_t *rgb = textColors[connected_dev.currentColor].rgb;
  videodisplay.setTextColor(videodisplay.RGB(0, 255, 255));
  printCentered(connected_dev.deviceName.c_str(), ROWS);
  videodisplay.setTextColor(videodisplay.RGB(rgb[0], rgb[1], rgb[2]));
}

void  clearScreen(bool showIdentified){
  connected_dev.buffer = "";
  setCursor(0, 0);
  videodisplay.clear();
  if (showIdentified && connected_dev.isIdentified)
        displayConnectedDeviceName();
}


void drawRLEImage(const unsigned char* rle, int width, int height) {
  int x = 0, y = 0;
  clearScreen(false);
  uint8_t logo_color = 7;
  for (int i = 0; i < 4078; i += 2) {
    uint8_t len = rle[i];
    uint8_t color = rle[i + 1] ? logo_color : 0;
    for (int j = 0; j < len; j++) {
      videodisplay.dotFast(x, y, color);
      x++;
      if (x >= width) {
        x = 0;
        y++;
        if (y >= height)
          return;
      }
    }
  }
}

void moveCursorNext() {
  cursor_x++;
  if (cursor_x >= COLS) {
    cursor_x = 0;
    cursor_y++;
  }
  if (cursor_y >= ROWS) {
    videodisplay.clear(true);
    cursor_x = 0;
    cursor_y = 0;
  }
  setCursor(cursor_x, cursor_y);
}


void putChar(char c){
  videodisplay.print(c);
  moveCursorNext();
}

void putStr(const char *s){
  while (*s){
    putChar(*s);
    s++;
  }
}

void printCentered(const char* s, int16_t row) {
  videodisplay.setCursor(((COLS / 2) - (strlen(s) / 2)) * 8, row * 8);
  videodisplay.print(s);
  setCursor(cursor_x, cursor_y);
}

void  putPrompt(void){
  uint8_t *rgb = textColors[connected_dev.currentColor].rgb;
  videodisplay.setTextColor(videodisplay.RGB(255, 255, 255));
  putChar('>');
  videodisplay.setTextColor(videodisplay.RGB(rgb[0], rgb[1], rgb[2]));
}

void  readDeviceName(char c){
if (c == '\n'){
      connected_dev.read_into = SCREEN;
      connected_dev.state = EMPTY;
      connected_dev.isIdentified = true;
      displayConnectedDeviceName();
      return;
    }
    connected_dev.deviceName += c;
    return ;
}

void  echoToScreen(char c){
  switch (c) {
    case '\n':
      cursor_x = 0;
      cursor_y++;
      if (cursor_y == ROWS){
        setCursor(0, 0);
        clearScreen(true);
      }
      else
        setCursor(cursor_x, cursor_y);
      putPrompt();
      connected_dev.buffer = "";
      connected_dev.read_into = SCREEN;
      connected_dev.state = EMPTY;
      break;
    default:
      putChar(c);
      break;
  }
}

void verifyInput(char c) {
  // still waiting for a valid command
  if (connected_dev.state != SPECIFIED)
    return handleCommandState(c);
  // if we need to read into the device name
  if (connected_dev.read_into == DEV_NAME)
    return readDeviceName(c);
  else if (connected_dev.read_into == COLOR)
    return readNewTextColor(c);
  // else then we just need to echo into the screen
  echoToScreen(c);
}

void showWelcomeScreen() {
  drawRLEImage(makersLogo, 640, 400);
}

// ========== Setup VGA ==========
void initVGA(void) {
  videodisplay.setFont(CodePage437_8x8);
  videodisplay.init(VGAMode::MODE640x400, RED, GREEN, BLUE, HSYNC, VSYNC);
  videodisplay.setTextColor(videodisplay.RGB(255, 255, 255), videodisplay.RGB(0, 0, 0));
  clearScreen(false);
}

void resetClientData(void){
  connected_dev.buffer.clear();
  connected_dev.deviceIp.clear();
  connected_dev.deviceName.clear();
  connected_dev.isIdentified = false;
  connected_dev.state = EMPTY;
  connected_dev.currentColor = 7;
}

// ========== Setup ==========
void setup() {
  // WiFi connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  server.begin();
  initVGA();
  initCommands();
  resetClientData();
  showWelcomeScreen();
}

// ========== Loop ==========
bool WELCOME_STATE = true;
void loop() {
  // if client disconnected
  if (client && !client.connected()) {
    client.stop();
    resetClientData();
    showWelcomeScreen();
    WELCOME_STATE = true;
  }
  
  // Accept new client if none is connected
  if (!client || !client.connected()) {
    client = server.accept();
    if (!client) {
      if (!WELCOME_STATE){
        showWelcomeScreen();
        WELCOME_STATE = true;
        resetClientData();
      }
      return;
    }
    connected_dev.deviceIp = client.remoteIP().toString();
    clearScreen(false);
    setCursor(0, 0);
    putPrompt();
    WELCOME_STATE = false;
  }
  
  // Handle client input
  while (client.available()) {
    char c = client.read();
    verifyInput(c);
  }
}