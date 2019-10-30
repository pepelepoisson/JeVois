
/*****************************************************************************************
  This sketch is part of the Nunchi project aimed at experimenting with Computer 
  Vision and JeVois PyEmotions module. It receives serial data from the JeVois camera and 
  displays radar plots representing face emotions on the ILI9341 TFT 240x320 display. 
  
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
#define PushB1  2 // Pin for push button 1  
#define PushB2  3 // Pin for push button 1 
#define Button_1_On  (!digitalRead(PushB1))
#define Button_2_On  (!digitalRead(PushB2))

// Must use hardware SPI for speed
Adafruit_ILI9341_AS tft = Adafruit_ILI9341_AS(_cs, _dc, _rst);

// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
//#define TOP_FIXED_AREA 16 // Number of lines in top fixed area (lines counted from top of screen)
#define TOP_FIXED_AREA 208 // Number of lines in top fixed area (lines counted from top of screen)

// The initial y moodinate of the top of the scrolling area
uint16_t yStart = TOP_FIXED_AREA;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = 320-TOP_FIXED_AREA-BOT_FIXED_AREA;
// The initial y moodinate of the top of the bottom text line
uint16_t yDraw = 320 - BOT_FIXED_AREA - TEXT_HEIGHT;

// Keep track of the drawing x moodinate
uint16_t xPos = 0;
// For the byte we read from the serial port
byte data = 0;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the serial buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

#define AXIS_LENGTH 60  // Length of each axis, from center, in pixels
#define RADAR_CENTER_X 120  // X-position of radar center
#define RADAR_CENTER_Y 110  // Y-position of radar center
int received_mood[8]={0,0,0,0,0,0,0,0}; // Table to 8 mood values
int x[8]={0,0,0,0,0,0,0,0};  // Table to hold 8 x-axis coordinates for 8 mood spikes
int y[8]={0,0,0,0,0,0,0,0};  // Table to hold 8 y-axis coordinates for 8 mood spikes
int previous_x[8]={0,0,0,0,0,0,0,0};  // Table to hold 8 x-values from previous iteration
int previous_y[8]={0,0,0,0,0,0,0,0};  // Table to hold 8 y-values from previous iteration
float x_factor[8]={0,0.707107,1,0.707107,0,-0.707007,-1,-0.707107};  // Table of cos values to calculate x-coordinates
float y_factor[8]={-1,-0.707107,0,+0.707107,+1,+0.707107,0,-0.707107};  // Table of sin values to calculate y-coordinates
float mood_length=0;
bool loading=true;
int max_mood_index=-1, previous_max_mood_index=-1, second_max_mood_index=-1, max_mood=-1000, second_max_mood=-1000;
enum moodEnum { HAPPINESS, SURPRISE, NEUTRAL,CONTEMPT,ANGER,DISGUST,SADNESS,FEAR, MOOD_COUNT };
const char *mood[MOOD_COUNT] = { "Happiness", "Surprise", "Neutral","Contempt","Anger","Disgust","Sadness","Fear"};

long elapsed=0, time_ref=0;

void Splash_Screen() {
  int x1, y1, x2, y2, x3, y3, w = tft.width(), h = tft.height();
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
}

void Nunchi_Intro() {

    // Drawing top title
    //tft.fillRect(0,0,240,320, ILI9341_WHITE);
    //tft.drawRoundRect(0,0,240,200,20,ILI9341_RED);
    tft.fillRoundRect(40,50,160,60,20,ILI9341_WHITE);
    tft.setTextColor(ILI9341_RED, ILI9341_WHITE);
    tft.drawCentreString("Nunchi",120,70,4);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.drawCentreString("The (Korean) subtle art",120,130,2);
    tft.drawCentreString("to gauge others' moods",120,150,2);
    // Drawing bottom separator line
    tft.drawLine(0,200,240,200,ILI9341_WHITE);
}

void Radar_background() {
    // Clearing screen
    tft.fillScreen(ILI9341_BLACK);
    
    // Drawing top title
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.fillRect(0,0,240,16, ILI9341_RED);
    tft.drawCentreString("How do you feel today?",120,0,2);

    // Drawing axes
    tft.drawLine(120, 50, 120, 170, ILI9341_WHITE);
    tft.drawLine(60, 110, 180, 110, ILI9341_WHITE);
    tft.drawLine(78,68,163,153, ILI9341_WHITE);
    tft.drawLine(78,153,163,68, ILI9341_WHITE);
    
    // Drawing axes titles
    tft.setTextColor(ILI9341_WHITE);tft.setTextSize(1);
    tft.setCursor(100, 30); tft.println("Happiness");  //1
    tft.setCursor(175, 58); tft.println("Surprise");  //2
    tft.setCursor(190, 105); tft.println("Neutral");  //3
    tft.setCursor(173, 155);tft.println("Contempt");  //4
    tft.setCursor(105, 180);tft.println("Anger");  //5
    tft.setCursor(25, 155); tft.println("Disgust"); //6
    tft.setCursor(5, 105);tft.println("Sadness");  //7
    tft.setCursor(45, 58);tft.println("Fear"); //8

    // Drawing bottom separator line
    tft.drawLine(0,200,240,200,ILI9341_WHITE);
}

void setup() {
  pinMode(PushB1,INPUT);
  digitalWrite(PushB1,HIGH);  // Configure built-in pullup resitor for push button 1
  
  pinMode(PushB2,INPUT);
  digitalWrite(PushB2,HIGH);  // Configure built-in pullup resitor for push button 1 
  // Setup the TFT display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);

  yield();
  
  Splash_Screen();
  
  // Setup scroll area
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  
  // Setup baud rate
  Serial.begin(9600);

  Nunchi_Intro();
  
  // Change colour for scrolling zone
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // Zero the array
  for (byte i = 0; i<18; i++) blank[i]=0;
}


void loop(void) {

  if (Button_1_On){Radar_background();}
  if (Button_2_On){tft.fillScreen(ILI9341_BLACK);}
  
  while (Serial.available()) {
    data = Serial.read();

    if (loading==true){
    //if (true){
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
         
        if (strcmp(subStr(serialbuf, " ", 2),"mood:")==0){
          if (loading==true){
            loading=false;
            // Setup scroll area
            setupScrollArea(320, 0);
            tft.fillScreen(ILI9341_BLACK);
            Radar_background();
          }
                    
          received_mood[2] = atoi(subStr(serialbuf, " ", 3));  // Neutral
          received_mood[0] = atoi(subStr(serialbuf, " ", 4));  // Happiness
          received_mood[1] = atoi(subStr(serialbuf, " ", 5));  // Surprise
          received_mood[6] = atoi(subStr(serialbuf, " ", 6));  // Sadness
          received_mood[4] = atoi(subStr(serialbuf, " ", 7));  // Anger
          received_mood[5] = atoi(subStr(serialbuf, " ", 8));  // Disgust
          received_mood[7] = atoi(subStr(serialbuf, " ", 9));  // Fear
          received_mood[3] = atoi(subStr(serialbuf, " ", 10));  // Contempt
         
          int data_check=0;
          for (int i=0;i<8;i++){
            if (received_mood[i]>-1001 && received_mood[i]<1001){data_check++;}
          }

          if (data_check==8){
            previous_max_mood_index=max_mood_index;
            max_mood_index=0;
            max_mood=-1000;
            for (int i=0;i<8;i++){
              previous_x[i]=x[i];
              previous_y[i]=y[i];
              if (received_mood[i]>max_mood){max_mood_index=i;max_mood=received_mood[i];}
              mood_length=map(received_mood[i],-1000,1000,0,AXIS_LENGTH);
              x[i]=x_factor[i]*mood_length+RADAR_CENTER_X;
              y[i]=y_factor[i]*mood_length+RADAR_CENTER_Y;
              back_color=0xFC18;  font_color=0x0000; // Pink & Black
            }

            // Find-out which mood has second max value
            second_max_mood_index=0;
            second_max_mood=-1000;
            for (int i=0;i<8;i++){
              if (received_mood[i]>second_max_mood && i!=max_mood_index){second_max_mood_index=i;second_max_mood=received_mood[i];}
            }

            // If max mood changed then write results below the radar plot
            if (previous_max_mood_index!=max_mood_index){
              tft.fillRect(0,210,240,110, ILI9341_BLACK);
              tft.setCursor(0, 220);
              if(max_mood_index==2){tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);tft.println("Hum... pretty neutral so far.");}
              else{
                tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);tft.println("Oh! Now it looks like the mood is mostly ");
                tft.setTextColor(ILI9341_RED); tft.setTextSize(2);tft.println(mood[max_mood_index]);
                if(second_max_mood_index!=2){
                  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);tft.println("and ");
                  tft.setTextColor(ILI9341_GREEN); tft.setTextSize(2);tft.print(mood[second_max_mood_index]);
                }
              }
            }
            
            // Delete previous main mood triangles
            if (previous_max_mood_index!=-1){
              tft.fillTriangle(RADAR_CENTER_X, RADAR_CENTER_Y, previous_x[previous_max_mood_index], previous_y[previous_max_mood_index], (previous_x[previous_max_mood_index]+previous_x[(previous_max_mood_index+1)%8])/2, (previous_y[previous_max_mood_index]+previous_y[(previous_max_mood_index+1)%8])/2,ILI9341_BLACK);
              tft.fillTriangle(RADAR_CENTER_X, RADAR_CENTER_Y, previous_x[previous_max_mood_index], previous_y[previous_max_mood_index], (previous_x[previous_max_mood_index]+previous_x[(previous_max_mood_index+7)%8])/2, (previous_y[previous_max_mood_index]+previous_y[(previous_max_mood_index+7)%8])/2,ILI9341_BLACK);
            }//yield();
            
            // Plot radar
            for (int i=0;i<8;i++){
              tft.drawLine(previous_x[i%8], previous_y[i%8],previous_x[(i+1)%8], previous_y[(i+1)%8],ILI9341_BLACK);
              tft.drawLine(x[i%8], y[i%8],x[(i+1)%8], y[(i+1)%8],back_color);
              }

            // Plot main mood triangles
            tft.fillTriangle(RADAR_CENTER_X, RADAR_CENTER_Y, x[max_mood_index], y[max_mood_index], (x[max_mood_index]+x[(max_mood_index+1)%8])/2, (y[max_mood_index]+y[(max_mood_index+1)%8])/2,ILI9341_RED);
            tft.fillTriangle(RADAR_CENTER_X, RADAR_CENTER_Y, x[max_mood_index], y[max_mood_index], (x[max_mood_index]+x[(max_mood_index-1)%8])/2, (y[max_mood_index]+y[(max_mood_index-1)%8])/2,ILI9341_RED);

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

