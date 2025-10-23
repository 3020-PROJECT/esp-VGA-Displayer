#include <WiFi.h>
#include <ESP32Video.h>
#include <Ressources/CodePage437_8x8.h>
#include "images.h"

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


// 

#define MAX_COMMANDS 3

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

// connected Device details:
typedef enum COMMAND_STATE{
  EMPTY,
  READING,
  SPECIFIED,
} e_command_state;

typedef enum READ_DESTANIES{
  DEV_NAME,
  SCREEN,
  NONE,
} e_read_destanies;

typedef struct dev {
  String  deviceName;
  String  deviceIp;
  String  buffer;
  String  command;
  bool    isIdentified;
  e_command_state state;
  e_read_destanies read_into;
} _dev;

_dev  connected_dev;

String commands[MAX_COMMANDS];

typedef enum COMMAND_ID{
  IDENTIFY,
  CLEAR,
  PRINT,
  UNKNOWN
} e_command_id;

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

void  clearScreen(void){
  connected_dev.buffer = "";
  setCursor(0, 0);
  videodisplay.clear();
}


void drawRLEImage(const unsigned char* rle, int width, int height) {
  int x = 0, y = 0;
  clearScreen();
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
    videodisplay.clear();
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

e_command_id  getCommandID(void){
  uint8_t id;
  
  for (id = 0; id < MAX_COMMANDS; id++){
    if (connected_dev.command == commands[id])
      return (e_command_id)id;
  }

  return UNKNOWN;
}

void handleCommandState(char c){
  e_command_id id;

  if ((c != ' ') && (c != '\n')){
    connected_dev.command += c;
    return;
  }
  connected_dev.state = SPECIFIED;
  id = getCommandID();
  switch (id){
    case  CLEAR:
      clearScreen();
      putChar('>');
      connected_dev.state = EMPTY;
      if (connected_dev.deviceName)
        printCentered(connected_dev.deviceName.c_str(), ROWS);
      break;
    case  IDENTIFY:
      if (connected_dev.isIdentified){
        connected_dev.state = EMPTY;
        break;
      }
      connected_dev.read_into = DEV_NAME;
      connected_dev.deviceName = "";
      break;
    case  PRINT:
      connected_dev.read_into = SCREEN;
      break;
    default:
      connected_dev.state = EMPTY;
      break;
  }
  connected_dev.command = "";
}

void handleChar(char c) {
  if (connected_dev.state != SPECIFIED){
    handleCommandState(c);
    return ;
  }
  if (connected_dev.read_into == DEV_NAME){
    if (c == '\n'){
      connected_dev.read_into = SCREEN;
      connected_dev.state = EMPTY;
      connected_dev.isIdentified = true;
      printCentered(connected_dev.deviceName.c_str(), ROWS);
      return;
    }
    connected_dev.deviceName += c;
    return ;
  }

  switch (c) {
    case '\n':
      cursor_x = 0;
      cursor_y++;
      if (cursor_y >= ROWS) {
        videodisplay.clear();
        cursor_x = cursor_y = 0;
      }
      setCursor(cursor_x, cursor_y);
      connected_dev.buffer = "";
      connected_dev.read_into = SCREEN;
      connected_dev.state = EMPTY;
      putChar('>');
      break;
    default:
      putChar(c);
      break;
  }
}

void showWelcomeScreen() {
  clearScreen();
  drawRLEImage(makersLogo, 640, 400);
}

// ========== Setup VGA ==========
void initVGA(void) {
  videodisplay.setFont(CodePage437_8x8);
  videodisplay.init(VGAMode::MODE640x400, RED, GREEN, BLUE, HSYNC, VSYNC);
  videodisplay.setTextColor(videodisplay.RGB(255, 255, 255), videodisplay.RGB(0, 0, 0));
  clearScreen();
}

void initCommands(){
  commands[CLEAR] = "/clear";
  commands[IDENTIFY] = "/identify";
  commands[PRINT] = "/print";
}

// ========== Setup ==========
void setup() {
  // WiFi connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  // init server
  server.begin();
  // Initial screen
  initVGA();
  initCommands();
  connected_dev.isIdentified = false;
  connected_dev.read_into = NONE;
  showWelcomeScreen();
}

// ========== Loop ==========
bool WELCOME_STATE = true;
void loop() {
  // if client disconnected
  if (client && !client.connected()) {
    client.stop();
    connected_dev.buffer.clear();
    connected_dev.deviceIp.clear();
    connected_dev.deviceName.clear();
    connected_dev.isIdentified = false;
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
      }
      return;
    }
    connected_dev.deviceIp = client.remoteIP().toString();
    clearScreen();
    setCursor(0, 0);
    putChar('>');
    WELCOME_STATE = false;
  }
  
  // Handle client input
  while (client.available()) {
    char c = client.read();
    handleChar(c);
  }
}