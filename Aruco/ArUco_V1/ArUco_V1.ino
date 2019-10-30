/*****************************************************************************************
  This sketch is aimed at experimenting with Computer Vision and JeVois ArUco module. It 
  receives serial data from the JeVois camera and displays shapes corresponding to detected
  ArUco markers and displays name accordingly on the ILI9341 TFT 240x320 display. 
  
  The serial read and scrolling code is based on 2015 work by Alan Senior at
  https://www.instructables.com/id/Arduino-serial-UART-scrolling-display-terminal-usi/
 ****************************************************************************************/

const char EOPmarker = '\r'; //This is the end of packet marker
char serialbuf[64]; //This gives the incoming serial some room. Change it if you want a longer incoming.
uint16_t back_color=0x0000, font_color=0x0000;
char* ID;

#include <string.h> // we'll need this for subString
#define MAX_STRING_LEN 20 // like 3 lines above, change as needed.
 
#include <Adafruit_GFX_AS.h>     // Core graphics library
#include <Adafruit_ILI9341_AS.h> // Hardware-specific library
#include <SPI.h>

// These are the pins used for the UNO, we must use hardware SPI
#define _sclk 13
#define _miso 12 // Not used
#define _mosi 11
#define _cs 10
#define _rst 7  // While not used with adafruit original lib, the reset pin must be used with this custom lib otherwise it won't work at startup
#define _dc  9

// Must use hardware SPI for speed
Adafruit_ILI9341_AS tft = Adafruit_ILI9341_AS(_cs, _dc, _rst);

// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
//#define TOP_FIXED_AREA 16 // Number of lines in top fixed area (lines counted from top of screen)
#define TOP_FIXED_AREA 208 // Number of lines in top fixed area (lines counted from top of screen)

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = TOP_FIXED_AREA;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = 320-TOP_FIXED_AREA-BOT_FIXED_AREA;
// The initial y coordinate of the top of the bottom text line
uint16_t yDraw = 320 - BOT_FIXED_AREA - TEXT_HEIGHT;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;
// For the byte we read from the serial port
byte data = 0;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the serial buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

int received_coord[8]={0,0,0,0,0,0,0,0}; // Table to hold 8 coordinates of object box in camera -1000/+1000;-1000/+1000 coordinates
int converted_coord[8]={0,0,0,0,0,0,0,0};  // Table to hold 8 coordinates of object box in screen 0/240;0/180 coordinates
int previous_converted_coord[8]={0,0,0,0,0,0,0,0};  // Table to hold 8 coordinates of object box in screen 0/240;0/180 coordinates, hold previous iteration
bool loading=true;

long elapsed=0, time_ref=0;

void setup() {
  // Setup the TFT display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);

  yield();
  
  int           x1, y1, x2, y2, x3, y3,
                w = tft.width(),
                h = tft.height();
  x1 = w-1;
  y1 = 0;
  y2 = 140 - 1;
  x3 = 0;
  y3 = h-1;
  for(x2=0; x2<w; x2+=6) {tft.drawLine(x1, y1, x2, y2, ILI9341_CYAN);tft.drawLine(x3, y3, x1-x2, 180, ILI9341_CYAN);}
    
  tft.setTextColor(ILI9341_RED, ILI9341_YELLOW);
  tft.fillRect(0,140,240,40, ILI9341_YELLOW);
  tft.drawCentreString("Papas Inventeurs",120,150,4);
  delay(3000);
  for(x2=0; x2<w; x2+=6) {tft.drawLine(x1, y1, x2, y2, ILI9341_BLACK);tft.drawLine(x3, y3, x1-x2, 180, ILI9341_BLACK);}
  tft.fillScreen(ILI9341_BLACK);
  
  // Setup scroll area
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  
  // Setup baud rate and draw top banner
  Serial.begin(9600);
  //Serial.begin(115200);
  tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
  tft.fillRect(0,0,240,16, ILI9341_RED);
  tft.drawCentreString("Looking for ArUco markers....",120,0,2);

  //tft.fillRect(0, 17, 240, 180, ILI9341_WHITE);
  tft.drawLine(0,200,240,200,ILI9341_WHITE);
    
  // Change colour for scrolling zone
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // Zero the array
  for (byte i = 0; i<18; i++) blank[i]=0;
}


void loop(void) {
  while (Serial.available()) {
    data = Serial.read();

    if (loading==true){
      if (data == '\r' || xPos>231) {
        xPos = 0;
        yDraw = scroll_line(); // It takes about 13ms to scroll 16 pixel lines
      }
      if (data > 31 && data < 128) {
        xPos += tft.drawChar(data,xPos,yDraw,2);
        blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
      }
    }
    
    static int bufpos = 0; //starts the buffer back at the first position in the incoming serial.read
      if ((data != EOPmarker)&&(bufpos<64)) { //if the incoming character is not the byte that is the incoming package ender
        serialbuf[bufpos] = data; //the buffer position in the array get assigned to the current read
        bufpos++; //once that has happend the buffer advances, doing this over and over again until the end of package marker is read.
      }
      else { //once the end of package marker has been read
        serialbuf[bufpos] = 0; //restart the buff
        bufpos = 0; //restart the position of the buff
        elapsed=millis()-time_ref;
 
        if ((atoi(subStr(serialbuf, " ", 3))==4)&&(elapsed>300)){
          if (loading==true){
            loading=false;
            tft.fillRect(0,210,240,110, ILI9341_BLACK);
          }
          elapsed=0;
          time_ref=millis();
          ID = subStr(serialbuf, " ", 2);
          //int extra = atoi(subStr(serialbuf, " ", 3));
          received_coord[0] = atoi(subStr(serialbuf, " ", 4));
          received_coord[1] = atoi(subStr(serialbuf, " ", 5));
          received_coord[2] = atoi(subStr(serialbuf, " ", 6));        
          received_coord[3] = atoi(subStr(serialbuf, " ", 7));
          received_coord[4] = atoi(subStr(serialbuf, " ", 8));
          received_coord[5] = atoi(subStr(serialbuf, " ", 9)); 
          received_coord[6] = atoi(subStr(serialbuf, " ", 10));
          received_coord[7] = atoi(subStr(serialbuf, " ", 11));

          int data_check=0;
          for (int i=0;i<8;i++){
            if (received_coord[i]>-1001 && received_coord[i]<1001){data_check++;}
          }

          if (data_check==8){
            for (int i=0;i<8;i++){
              previous_converted_coord[i]=converted_coord[i];
              if (i%2!=2){converted_coord[i]=map(received_coord[i],-1000,1000,0,240);}  // Convert x-axis coordinates from camera to screen coordinates
              else {converted_coord[i]=map(received_coord[i],-1000,1000,0,180);}    // Convert y-axis coordinates from camera to screen coordinates
              back_color=0xFC18;  font_color=0x0000; // Pink & Black
              if (strcmp(ID,"U27")==0){back_color=0xF800; font_color=0xFFFF;}  // Red & White
              if (strcmp(ID,"U43")==0){back_color=0x07E0; font_color=0x0000;}  // Green & Black
              if (strcmp(ID,"U18")==0){back_color=0x001F; font_color=0xFFFF;}  // Blue & White
            }
          
            // Plot object box
            for (int i=0;i<4;i++){
              tft.drawLine(previous_converted_coord[2*i%8], previous_converted_coord[(2*i+1)%8],previous_converted_coord[(2*i+2)%8], previous_converted_coord[(2*i+3)%8],ILI9341_BLACK);
              tft.drawLine(converted_coord[2*i%8], converted_coord[(2*i+1)%8],converted_coord[(2*i+2)%8], converted_coord[(2*i+3)%8],back_color);
              }

            // Show ArUco ID number in bottom box
            tft.fillRect(0, 280, 240, 40, back_color);
            tft.setTextColor(font_color, back_color);
            tft.drawCentreString(ID,120,290,2);        
          }
        }
      }
    
  }
}

// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, ILI9341_BLACK);

  // Change the top of the scroll area
  yStart+=TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  if (yStart >= 320 - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - 320 + BOT_FIXED_AREA);
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t TFA, uint16_t BFA) {
  tft.writecommand(ILI9341_VSCRDEF); // Vertical scroll definition
  tft.writedata(TFA >> 8);
  tft.writedata(TFA);
  tft.writedata((320-TFA-BFA)>>8);
  tft.writedata(320-TFA-BFA);
  tft.writedata(BFA >> 8);
  tft.writedata(BFA);
}

// ##############################################################################################
// Setup the vertical scrolling start address
// ##############################################################################################
void scrollAddress(uint16_t VSP) {
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling start address
  tft.writedata(VSP>>8);
  tft.writedata(VSP);
}

char* subStr (char* input_string, char *separator, int segment_number) {
  char *act, *sub, *ptr;
  static char copy[MAX_STRING_LEN];
  int i;
  strcpy(copy, input_string);
  for (i = 1, act = copy; i <= segment_number; i++, act = NULL) {
    sub = strtok_r(act, separator, &ptr);
    if (sub == NULL) break;
  }
 return sub;
}

