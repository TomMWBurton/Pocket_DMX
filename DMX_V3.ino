// https://forum.arduino.cc/index.php?topic=633532.0
// https://cdn-learn.adafruit.com/downloads/pdf/adafruit-gfx-graphics-library.pdf
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SPI.h>
#include <Wire.h>           // this is needed even tho we aren't using it
#include <TouchScreen.h>
#include <EEPROM.h>
//#include <ArduinoRS485.h> // the ArduinoDMX library depends on ArduinoRS485
//#include <ArduinoDMX.h>
#include <DmxSimple.h>
#include <TimerOne.h>

#define MINPRESSURE 200     //Min prssure to activate touch screen
#define MAXPRESSURE 1000    //Max prssure to activate touch screen

#define BLACK   0x0000      //translates colour codes to english text
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// ALL Touch panels and wiring is DIFFERENT
// copy-paste results from TouchScreen_Calibr_native.ino
//const int XP=7,XM=A1,YP=A2,YM=6; //240x320 ID=0x9340
//const int TS_LEFT=193,TS_RT=907,TS_TOP=194,TS_BOT=859;
const int XP=7,XM=A1,YP=A2,YM=6; //240x320 ID=0x9340
const int TS_LEFT=155,TS_RT=900,TS_TOP=140,TS_BOT=870;


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Adafruit_GFX_Button on_btn;
MCUFRIEND_kbv tft;

int pixel_x, pixel_y;     //Touch_getXY() updates global vars

int sliderYmax = 265;
int sliderYmin = 85;

int slider1x = 20;
int slider2x = 60;
int slider3x = 100;
int slider4x = 140;
int slider5x = 180;
int slider6x = 220;

int sliderXpositions[] = {20,60,100,140,180,220};

int channelValueBoxX = 15;
int channelValueBoxY = 45;
int channelValueBoxHeight = 30;

int channelMapBoxX = 15;
int channelMapBoxY = 10;
int channelMapBoxHeight = 30;

int currentPage = 1;
int scrollPosition = 1;
int scrollPositionOld = 1;

int channelMapArray[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
int sliderArray[] = {0,0,0,0,0,0,     0,0,0,0,0,0,      0,0,0,0,0,0,      0,0,0,0,0,0};
int sliderPosArray[] = {0,0,0,0,0,0,     0,0,0,0,0,0,      0,0,0,0,0,0,      0,0,0,0,0,0};
int sliderOldArray[] = {0,0,0,0,0,0,     0,0,0,0,0,0,      0,0,0,0,0,0,      0,0,0,0,0,0};

bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT); // restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH); // because TFT control pins are digital
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE); // looks for detected pressure on screen
    if (pressed) {
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, 240); //return pressure point data
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, 320);
    }
    return pressed;
}

void setup() {
  //Serial.begin(9600);
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0); //PORTRAIT
  tft.fillScreen(BLACK);
  draw_screen();
  delay(2000);
  
  digitalWrite(10, HIGH);
  DmxSimple.maxChannel(24);
  DmxSimple.usePin(1);
  Timer1.initialize(300);
  Timer1.attachInterrupt(sendDMX); // sendDMX to run every 0.03 seconds
}

void sendDMX() {
  for (int channel = 0; channel < 24 ; channel++) {
    int channelAddress = channelMapArray[channel];
    int channelValue = sliderArray[channel];
    DmxSimple.write(channelAddress, channelValue);
  }
}

void loop() {
  
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  
  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
  
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) return; //INVALID
  //Serial.print("X = "); Serial.print(p.x);
  //Serial.print("\tY = "); Serial.print(p.y);
  //Serial.print("\tPressure = "); Serial.println(p.z);

  //Serial.println("XY point retrived"); //diagnosis notes
  
  // Scale from ~0->4000 to tft.width using the calibration #'s
  pixel_x = map(p.x, TS_LEFT, TS_RT, 0, 240); //.kbv makes sense to me
  pixel_y = map(p.y, TS_TOP, TS_BOT, 0, 320);

  //Serial.print("pixel_x = "); Serial.print(pixel_x);
  //Serial.print("\tpixel_y = "); Serial.println(pixel_y);
  
  if(currentPage == 1) {

    // Channel map update
    if (pixel_y > channelMapBoxY && pixel_y < (channelMapBoxY + channelMapBoxHeight)) {
      int i = 0;
      bool updated = false;
      while(updated == false) {
      //for(int i = 0 ; i < 6 ; i++) {
        if (pixel_x > (sliderXpositions[i]-channelMapBoxX) && pixel_x < ((sliderXpositions[i]-channelMapBoxX) + 30) && updated == false) {
          currentPage = 2;
          int result = update_channel(channelMapArray[(scrollPosition-1) * 6 + i]);
          if(result == -1) { 
            currentPage = 1;
            updated = true;
          } else {
            channelMapArray[(scrollPosition-1) * 6 + i] = result;
            currentPage = 1;  
            updated = true;        
          }
        }
        i++;
      }
      
      draw_screen_1_sliders(scrollPosition, scrollPosition);
      draw_screen_1_buttons();
      draw_screen_1_channel_map();
      return;
    }

    // Change channels
    if (pixel_y > 280 && pixel_y < 310) {
      if (pixel_x > 15 && pixel_x < 65) { 
        scrollPositionOld = scrollPosition;
        if(scrollPosition > 1) {  
          scrollPositionOld = scrollPosition; 
          scrollPosition = scrollPosition - 1;
        }
        //Serial.print("Scroll position decrease to: ");
        //Serial.println(scrollPosition);
      }
      if (pixel_x > 185 && pixel_x < 225) {
        if(scrollPosition < 4) {
          scrollPositionOld = scrollPosition;
          scrollPosition = scrollPosition + 1;
        }
        //Serial.print("Scroll position increase to: ");
        //Serial.println(scrollPosition);
      }

      if (pixel_x > 75 && pixel_x < 175) {
       //Serial.println("SAVE!");
       save();
      }
       
      draw_screen_1_sliders(scrollPosition, scrollPositionOld);

      draw_screen_1_buttons();

      draw_screen_1_channel_map();
      
      for(int i = 0 ; i < 24 ; i++) {
        sliderOldArray[i] = sliderPosArray[i];
      }

      delay(200); // Debounce
      
      return;
      
    }

    // Check sliders
    if (pixel_y > (sliderYmin - 5) && pixel_y < (sliderYmax + 5)) {
      // Slider 1
      if (pixel_x > (slider1x - 15) && pixel_x < (slider1x + 15)) {
        // Map slider y value to DMX intensity
        sliderArray[(scrollPosition-1) * 6 + 0] = map(pixel_y, sliderYmax, sliderYmin, 0, 255);
        if(sliderArray[(scrollPosition-1) * 6 + 0] > 255) sliderArray[(scrollPosition-1) * 6 + 0] = 255;
        if(sliderArray[(scrollPosition-1) * 6 + 0] < 0) sliderArray[(scrollPosition-1) * 6 + 0] = 0;
        //Serial.println("check slider 1"); // diagnosis notes
        // Grab new slider position after storing the old one
        sliderOldArray[(scrollPosition-1) * 6 + 0] = sliderPosArray[(scrollPosition-1) * 6 + 0];
        sliderPosArray[(scrollPosition-1) * 6 + 0] = pixel_y;
      }
      // Slider 2
      if (pixel_x > (slider2x - 15) && pixel_x < (slider2x + 15)) {
        // Map slider y value to DMX intensity
        sliderArray[(scrollPosition-1) * 6 + 1] = map(pixel_y, sliderYmax, sliderYmin, 0, 255);
        if(sliderArray[(scrollPosition-1) * 6 + 1] > 255) sliderArray[(scrollPosition-1) * 6 + 1] = 255;
        if(sliderArray[(scrollPosition-1) * 6 + 1] < 0) sliderArray[(scrollPosition-1) * 6 + 1] = 0;
        //Serial.println("check slider 2"); // diagnosis notes
        // Grab new slider position after storing the old one
        sliderOldArray[(scrollPosition-1) * 6 + 1] = sliderPosArray[(scrollPosition-1) * 6 + 1];
        sliderPosArray[(scrollPosition-1) * 6 + 1] = pixel_y;
      }
      // Slider 3
      if (pixel_x > (slider3x - 15) && pixel_x < (slider3x + 15)) {
        // Map slider y value to DMX intensity
        sliderArray[(scrollPosition-1) * 6 + 2] = map(pixel_y, sliderYmax, sliderYmin, 0, 255);
        if(sliderArray[(scrollPosition-1) * 6 + 2] > 255) sliderArray[(scrollPosition-1) * 6 + 2] = 255;
        if(sliderArray[(scrollPosition-1) * 6 + 2] < 0) sliderArray[(scrollPosition-1) * 6 + 2] = 0;
        //Serial.println("check slider 3"); // diagnosis notes
        // Grab new slider position after storing the old one
        sliderOldArray[(scrollPosition-1) * 6 + 2] = sliderPosArray[(scrollPosition-1) * 6 + 2];
        sliderPosArray[(scrollPosition-1) * 6 + 2] = pixel_y;
      }
      // Slider 4
      if (pixel_x > (slider4x - 15) && pixel_x < (slider4x + 15)) {
        // Map slider y value to DMX intensity
        sliderArray[(scrollPosition-1) * 6 + 3] = map(pixel_y, sliderYmax, sliderYmin, 0, 255);
        if(sliderArray[(scrollPosition-1) * 6 + 3] > 255) sliderArray[(scrollPosition-1) * 6 + 3] = 255;
        if(sliderArray[(scrollPosition-1) * 6 + 3] < 0) sliderArray[(scrollPosition-1) * 6 + 3] = 0;
        //Serial.println("check slider 4"); // diagnosis notes
        // Grab new slider position after storing the old one
        sliderOldArray[(scrollPosition-1) * 6 + 3] = sliderPosArray[(scrollPosition-1) * 6 + 3];
        sliderPosArray[(scrollPosition-1) * 6 + 3] = pixel_y;
  
      }
      // Slider 5
      if (pixel_x > (slider5x - 15) && pixel_x < (slider5x + 15)) {
        // Map slider y value to DMX intensity
        sliderArray[(scrollPosition-1) * 6 + 4] = map(pixel_y, sliderYmax, sliderYmin, 0, 255);
        if(sliderArray[(scrollPosition-1) * 6 + 4] > 255) sliderArray[(scrollPosition-1) * 6 + 4] = 255;
        if(sliderArray[(scrollPosition-1) * 6 + 4] < 0) sliderArray[(scrollPosition-1) * 6 + 4] = 0;
        //Serial.println("check slider 5"); // diagnosis notes
        // Grab new slider position after storing the old one
        sliderOldArray[(scrollPosition-1) * 6 + 4] = sliderPosArray[(scrollPosition-1) * 6 + 4];
        sliderPosArray[(scrollPosition-1) * 6 + 4] = pixel_y;
      }
      // Slider 6
      if (pixel_x > (slider6x - 15) && pixel_x < (slider6x + 15)) {
        // Map slider y value to DMX intensity
        sliderArray[(scrollPosition-1) * 6 + 5] = map(pixel_y, sliderYmax, sliderYmin, 0, 255);
        if(sliderArray[(scrollPosition-1) * 6 + 5] > 255) sliderArray[(scrollPosition-1) * 6 + 5] = 255;
        if(sliderArray[(scrollPosition-1) * 6 + 5] < 0) sliderArray[(scrollPosition-1) * 6 + 5] = 0;
        //Serial.println("check slider 6"); // diagnosis notes
        // Grab new slider position after storing the old one
        sliderOldArray[(scrollPosition-1) * 6 + 5] = sliderPosArray[(scrollPosition-1) * 6 + 5];
        sliderPosArray[(scrollPosition-1) * 6 + 5] = pixel_y;
      }
  
      draw_screen_1_sliders(scrollPosition, scrollPosition);
  
      draw_screen_1_buttons();
  
      draw_screen_1_channel_map();
  
      for(int i = 0 ; i < 24 ; i++) {
        //Serial.println("Slider:");
        //Serial.println(i+1);
        //Serial.println(sliderOldArray[i]);
        //Serial.println(sliderPosArray[i]);
        sliderOldArray[i] = sliderPosArray[i];
        //Serial.println(sliderOldArray[i]);
      }      
    }
  }
} 

void draw_screen_1_sliders(int scrollPosition, int scrollPositionOld) {
  // Slider 1 black out the old slider, re-draw the verticle bar, then draw the new slider
  tft.fillCircle(slider1x, sliderOldArray[(scrollPositionOld-1) * 6 + 0], 25, BLACK);
  //tft.drawFastHLine(slider1x-10, sliderOldArray[(scrollPositionOld-1) * 6 + 0], 20, BLACK);
  tft.drawFastHLine(slider1x-10, sliderPosArray[(scrollPosition-1) * 6 + 0], 20, WHITE);      
  tft.fillCircle(slider1x, sliderPosArray[(scrollPosition-1) * 6 + 0], 5, WHITE);
  tft.drawFastVLine(slider1x, sliderYmin, (sliderYmax-sliderYmin), WHITE);
  tft.drawRoundRect(slider1x-channelValueBoxX,channelValueBoxY,30,channelValueBoxHeight,3,WHITE);
  tft.fillRoundRect(slider1x-14,46,28,28,3,BLACK);
  tft.setCursor(slider1x-8, 56);
  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  if (sliderArray[(scrollPosition-1) * 6 + 0]<100) tft.print('0');
  if (sliderArray[(scrollPosition-1) * 6 + 0]<10) tft.print('0');
  tft.print(sliderArray[(scrollPosition-1) * 6 + 0]);
  // Slider 2 black out the old slider, re-draw the verticle bar, then draw the new slider
  tft.fillCircle(slider2x, sliderOldArray[(scrollPositionOld-1) * 6 + 1], 25, BLACK);
  //tft.drawFastHLine(slider2x-10, sliderOldArray[(scrollPositionOld-1) * 6 + 1], 20, BLACK);
  tft.drawFastHLine(slider2x-10, sliderPosArray[(scrollPosition-1) * 6 + 1], 20, WHITE);      
  tft.fillCircle(slider2x, sliderPosArray[(scrollPosition-1) * 6 + 1], 5, WHITE);
  tft.drawFastVLine(slider2x, sliderYmin, (sliderYmax-sliderYmin), WHITE);
  tft.drawRoundRect(slider2x-channelValueBoxX,channelValueBoxY,30,channelValueBoxHeight,3,WHITE);
  tft.fillRoundRect(slider2x-14,46,28,28,3,BLACK);
  tft.setCursor(slider2x-8, 56);
  //tft.setTextColor(WHITE);  
  //tft.setTextSize(1);
  if (sliderArray[(scrollPosition-1) * 6 + 1]<100) tft.print('0');
  if (sliderArray[(scrollPosition-1) * 6 + 1]<10) tft.print('0');
  tft.print(sliderArray[(scrollPosition-1) * 6 + 1]);
  // Slider 3 black out the old slider, re-draw the verticle bar, then draw the new slider
  tft.fillCircle(slider3x, sliderOldArray[(scrollPositionOld-1) * 6 + 2], 25, BLACK);
  //tft.drawFastHLine(slider3x-10, sliderOldArray[(scrollPositionOld-1) * 6 + 2], 20, BLACK);
  tft.drawFastHLine(slider3x-10, sliderPosArray[(scrollPosition-1) * 6 + 2], 20, WHITE);      
  tft.fillCircle(slider3x, sliderPosArray[(scrollPosition-1) * 6 + 2], 5, WHITE);
  tft.drawFastVLine(slider3x, sliderYmin, (sliderYmax-sliderYmin), WHITE);
  tft.drawRoundRect(slider3x-channelValueBoxX,channelValueBoxY,30,channelValueBoxHeight,3,WHITE);
  tft.fillRoundRect(slider3x-14,46,28,28,3,BLACK);
  tft.setCursor(slider3x-8, 56);
  //tft.setTextColor(WHITE);  
  //tft.setTextSize(1);
  if (sliderArray[(scrollPosition-1) * 6 + 2]<100) tft.print('0');
  if (sliderArray[(scrollPosition-1) * 6 + 2]<10) tft.print('0');
  tft.print(sliderArray[(scrollPosition-1) * 6 + 2]);
  // Slider 4 black out the old slider, re-draw the verticle bar, then draw the new slider
  tft.fillCircle(slider4x, sliderOldArray[(scrollPositionOld-1) * 6 + 3], 25, BLACK);
  //tft.drawFastHLine(slider4x-10, sliderOldArray[(scrollPositionOld-1) * 6 + 3], 20, BLACK);
  tft.drawFastHLine(slider4x-10, sliderPosArray[(scrollPosition-1) * 6 + 3], 20, WHITE);      
  tft.fillCircle(slider4x, sliderPosArray[(scrollPosition-1) * 6 + 3], 5, WHITE);
  tft.drawFastVLine(slider4x, sliderYmin, (sliderYmax-sliderYmin), WHITE);
  tft.drawRoundRect(slider4x-channelValueBoxX,channelValueBoxY,30,channelValueBoxHeight,3,WHITE);
  tft.fillRoundRect(slider4x-14,46,28,28,3,BLACK);
  tft.setCursor(slider4x-8, 56);
  //tft.setTextColor(WHITE);  
  //tft.setTextSize(1);
  if (sliderArray[(scrollPosition-1) * 6 + 3]<100) tft.print('0');
  if (sliderArray[(scrollPosition-1) * 6 + 3]<10) tft.print('0');
  tft.print(sliderArray[(scrollPosition-1) * 6 + 3]);
  // Slider 5 black out the old slider, re-draw the verticle bar, then draw the new slider
  tft.fillCircle(slider5x, sliderOldArray[(scrollPositionOld-1) * 6 + 4], 25, BLACK);
  //tft.drawFastHLine(slider5x-10, sliderOldArray[(scrollPositionOld-1) * 6 + 4], 20, BLACK);
  tft.drawFastHLine(slider5x-10, sliderPosArray[(scrollPosition-1) * 6 + 4], 20, WHITE);      
  tft.fillCircle(slider5x, sliderPosArray[(scrollPosition-1) * 6 + 4], 5, WHITE);
  tft.drawFastVLine(slider5x, sliderYmin, (sliderYmax-sliderYmin), WHITE);
  tft.drawRoundRect(slider5x-channelValueBoxX,channelValueBoxY,30,channelValueBoxHeight,3,WHITE);
  tft.fillRoundRect(slider5x-14,46,28,28,3,BLACK);
  tft.setCursor(slider5x-8, 56);
  //tft.setTextColor(WHITE);  
  //tft.setTextSize(1);
  if (sliderArray[(scrollPosition-1) * 6 + 4]<100) tft.print('0');
  if (sliderArray[(scrollPosition-1) * 6 + 4]<10) tft.print('0');
  tft.print(sliderArray[(scrollPosition-1) * 6 + 4]);
  // Slider 6 black out the old slider, re-draw the verticle bar, then draw the new slider
  tft.fillCircle(slider6x, sliderOldArray[(scrollPositionOld-1) * 6 + 5], 25, BLACK);
  //tft.drawFastHLine(slider6x-10, sliderOldArray[(scrollPositionOld-1) * 6 + 5], 20, BLACK);
  tft.drawFastHLine(slider6x-10, sliderPosArray[(scrollPosition-1) * 6 + 5], 20, WHITE);      
  tft.fillCircle(slider6x, sliderPosArray[(scrollPosition-1) * 6 + 5], 5, WHITE);
  tft.drawFastVLine(slider6x, sliderYmin, (sliderYmax-sliderYmin), WHITE);
  tft.drawRoundRect(slider6x-channelValueBoxX,channelValueBoxY,30,channelValueBoxHeight,3,WHITE);
  tft.fillRoundRect(slider6x-14,46,28,28,3,BLACK);
  tft.setCursor(slider6x-8, 56);
  //tft.setTextColor(WHITE);  
  //tft.setTextSize(1);
  if (sliderArray[(scrollPosition-1) * 6 + 5]<100) tft.print('0');
  if (sliderArray[(scrollPosition-1) * 6 + 5]<10) tft.print('0');
  tft.print(sliderArray[(scrollPosition-1) * 6 + 5]);
}

void draw_screen_1_buttons() {
  tft.fillRoundRect(15,280,50,30,3,WHITE);
  tft.fillTriangle(25,295,55,285,55,305,BLACK);

  tft.fillRoundRect(240-65,280,50,30,3,WHITE);
  tft.fillTriangle(240-25,295,240-55,285,240-55,305,BLACK);

  tft.fillRoundRect(75,280,90,30,3,WHITE);
  tft.setCursor(97, 287);
  tft.setTextColor(BLACK);  
  tft.setTextSize(2);
  tft.print("Save");
}

void draw_screen_1_channel_map() {
  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  // Slider 1
  tft.drawRoundRect(slider1x-channelMapBoxX,channelMapBoxY,30,channelMapBoxHeight,3,WHITE);
  tft.fillRoundRect(slider1x-(channelMapBoxX-1),channelMapBoxY+1,channelMapBoxHeight-2,channelMapBoxHeight-2,3,BLACK);
  tft.setCursor(slider1x-8, 21);
  if (channelMapArray[(scrollPosition-1) * 6 + 0]<100) tft.print('0');
  if (channelMapArray[(scrollPosition-1) * 6 + 0]<10) tft.print('0');
  tft.print(channelMapArray[(scrollPosition-1) * 6 + 0]);
  // Slider 2
  tft.drawRoundRect(slider2x-channelMapBoxX,channelMapBoxY,30,channelMapBoxHeight,3,WHITE);
  tft.fillRoundRect(slider2x-(channelMapBoxX-1),channelMapBoxY+1,channelMapBoxHeight-2,channelMapBoxHeight-2,3,BLACK);
  tft.setCursor(slider2x-8, 21);
  if (channelMapArray[(scrollPosition-1) * 6 + 1]<100) tft.print('0');
  if (channelMapArray[(scrollPosition-1) * 6 + 1]<10) tft.print('0');
  tft.print(channelMapArray[(scrollPosition-1) * 6 + 1]);
  // Slider 3
  tft.drawRoundRect(slider3x-channelMapBoxX,channelMapBoxY,30,channelMapBoxHeight,3,WHITE);
  tft.fillRoundRect(slider3x-(channelMapBoxX-1),channelMapBoxY+1,channelMapBoxHeight-2,channelMapBoxHeight-2,3,BLACK);
  tft.setCursor(slider3x-8, 21);
  if (channelMapArray[(scrollPosition-1) * 6 + 2]<100) tft.print('0');
  if (channelMapArray[(scrollPosition-1) * 6 + 2]<10) tft.print('0');
  tft.print(channelMapArray[(scrollPosition-1) * 6 + 2]);
  // Slider 4
  tft.drawRoundRect(slider4x-channelMapBoxX,channelMapBoxY,30,channelMapBoxHeight,3,WHITE);
  tft.fillRoundRect(slider4x-(channelMapBoxX-1),channelMapBoxY+1,channelMapBoxHeight-2,channelMapBoxHeight-2,3,BLACK);
  tft.setCursor(slider4x-8, 21);
  if (channelMapArray[(scrollPosition-1) * 6 + 3]<100) tft.print('0');
  if (channelMapArray[(scrollPosition-1) * 6 + 3]<10) tft.print('0');
  tft.print(channelMapArray[(scrollPosition-1) * 6 + 3]);
  // Slider 5
  tft.drawRoundRect(slider5x-channelMapBoxX,channelMapBoxY,30,channelMapBoxHeight,3,WHITE);
  tft.fillRoundRect(slider5x-(channelMapBoxX-1),channelMapBoxY+1,channelMapBoxHeight-2,channelMapBoxHeight-2,3,BLACK);
  tft.setCursor(slider5x-8, 21);
  if (channelMapArray[(scrollPosition-1) * 6 + 4]<100) tft.print('0');
  if (channelMapArray[(scrollPosition-1) * 6 + 4]<10) tft.print('0');
  tft.print(channelMapArray[(scrollPosition-1) * 6 + 4]);
  // Slider 6
  tft.drawRoundRect(slider6x-channelMapBoxX,channelMapBoxY,30,channelMapBoxHeight,3,WHITE);
  tft.fillRoundRect(slider6x-(channelMapBoxX-1),channelMapBoxY+1,channelMapBoxHeight-2,channelMapBoxHeight-2,3,BLACK);
  tft.setCursor(slider6x-8, 21);
  if (channelMapArray[(scrollPosition-1) * 6 + 5]<100) tft.print('0');
  if (channelMapArray[(scrollPosition-1) * 6 + 5]<10) tft.print('0');
  tft.print(channelMapArray[(scrollPosition-1) * 6 + 5]);
}

int update_channel(int previousChannel) {
  bool goBack = false;
  bool save = false;
  int newChannel = 0;
  
  // 240 x 320
  tft.fillScreen(BLACK);
  
  tft.setTextColor(WHITE);  
  tft.setTextSize(5);
  tft.setCursor(80, 25);  
  if (previousChannel<100) tft.print('0');
  if (previousChannel<10) tft.print('0');
  tft.print(previousChannel);
    
  tft.setTextColor(WHITE);  
  tft.setTextSize(4);
  tft.drawRoundRect(35,80,50,50,3,WHITE);
  tft.setCursor(50, 90);
  tft.print("1");
  tft.drawRoundRect(95,80,50,50,3,WHITE);
  tft.setCursor(110, 90);
  tft.print("2");
  tft.drawRoundRect(155,80,50,50,3,WHITE);
  tft.setCursor(170, 90);
  tft.print("3");
  
  tft.drawRoundRect(35,140,50,50,3,WHITE);
  tft.setCursor(50, 150);
  tft.print("4");
  tft.drawRoundRect(95,140,50,50,3,WHITE);
  tft.setCursor(110, 150);
  tft.print("5");
  tft.drawRoundRect(155,140,50,50,3,WHITE);
  tft.setCursor(170, 150);
  tft.print("6");

  tft.drawRoundRect(35,200,50,50,3,WHITE);
  tft.setCursor(50, 210);
  tft.print("7");
  tft.drawRoundRect(95,200,50,50,3,WHITE);
  tft.setCursor(110, 210);
  tft.print("8");
  tft.drawRoundRect(155,200,50,50,3,WHITE);
  tft.setCursor(170, 210);
  tft.print("9");

  tft.drawRoundRect(95,260,50,50,3,WHITE);
  tft.setCursor(110, 270);
  tft.print("0");

  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  tft.drawRoundRect(35,260,50,50,3,WHITE);
  tft.setCursor(42, 280);
  tft.print("Return");
  tft.drawRoundRect(155,260,50,50,3,WHITE);
  tft.setCursor(168, 280);
  tft.print("Save");

  while(!goBack and !save) {

    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    
    // if sharing pins, you'll need to fix the directions of the touchscreen pins
    pinMode(XP, OUTPUT);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    pinMode(YM, OUTPUT);
    
    if (p.z < MINPRESSURE || p.z > MAXPRESSURE) continue; //INVALID
    
    // Scale from ~0->4000 to tft.width using the calibration #'s
    pixel_x = map(p.x, TS_LEFT, TS_RT, 0, 240); //.kbv makes sense to me
    pixel_y = map(p.y, TS_TOP, TS_BOT, 0, 320);
  
    //Serial.print("pixel_x = "); Serial.print(pixel_x);
    //Serial.print("\tpixel_y = "); Serial.println(pixel_y);
    
    if (pixel_y > (80) && pixel_y < (130)) {
      if (pixel_x > (35) && pixel_x < (85)) { 
        newChannel = newChannel * 10 + 1;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (95) && pixel_x < (145)) { 
        newChannel = newChannel * 10 + 2;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (155) && pixel_x < (205)) { 
        newChannel = newChannel * 10 + 3;
        if (newChannel > 255) newChannel = 0;        
      }
    }else if (pixel_y > (140) && pixel_y < (190)) {
      if (pixel_x > (35) && pixel_x < (85)) { 
        newChannel = newChannel * 10 + 4;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (95) && pixel_x < (145)) { 
        newChannel = newChannel * 10 + 5;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (155) && pixel_x < (205)) { 
        newChannel = newChannel * 10 + 6;
        if (newChannel > 255) newChannel = 0;        
      }
    } else if (pixel_y > (200) && pixel_y < (250)) {
      if (pixel_x > (35) && pixel_x < (85)) { 
        newChannel = newChannel * 10 + 7;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (95) && pixel_x < (145)) { 
        newChannel = newChannel * 10 + 8;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (155) && pixel_x < (205)) { 
        newChannel = newChannel * 10 + 9;
        if (newChannel > 255) newChannel = 0;        
      }
    }  else if (pixel_y > (260) && pixel_y < (310)) {
      if (pixel_x > (35) && pixel_x < (85)) { 
        goBack = true;
        continue;
      } else if (pixel_x > (95) && pixel_x < (145)) {
        newChannel = newChannel * 10;
        if (newChannel > 255) newChannel = 0;
      } else if (pixel_x > (155) && pixel_x < (205)) {
        save = true;
        continue;
      }
    }

    tft.fillRoundRect(0,0,240,75,3,BLACK);
    tft.setTextColor(WHITE);  
    tft.setTextSize(5);
    tft.setCursor(80, 25);
    
    if (newChannel<100) tft.print('0');
    if (newChannel<10) tft.print('0');
    tft.print(newChannel);

    delay(200);
  }

  tft.fillScreen(BLACK);
  if(newChannel == 0) newChannel = previousChannel;
  if(save) return newChannel;
  if(goBack) return -1;
  return -1;
}


void draw_screen() {
  // Create a blank, black screen 240 x 320
  tft.fillScreen(BLACK);
  
  // x y
  tft.fillRect(20, 20, 200, 280, WHITE);
  tft.fillRect(30, 30, 180, 260, BLACK);
  tft.setCursor(35, 130);

  //tft.setFont(&FreeMonoBold24pt7b);

  tft.setTextColor(WHITE);  
  tft.setTextSize(2);
  tft.print("D");
  delay(200);
  tft.print("M");
  delay(200);
  tft.print("X");
  delay(200);
  tft.setFont();
  tft.setTextSize(1);
  tft.setCursor(55, 210);
  tft.print("24 Channel Controller");
  delay(400);
  
  tft.fillScreen(BLACK);

  load();
  
  draw_screen_1_buttons();

  draw_screen_1_channel_map();

  draw_screen_1_sliders(1,1);
}

void save() {
  int eeAddress = 0;   //Location we want the data to be put
  //One simple call, with the address first and the object second
  for(int i = 0 ; i < 24 ; i++) {
    //EEPROM.put(eeAddress, channelMapArray[i]);
    //eeAddress += sizeof(int); //Move address to the next byte after int
    EEPROM.put(eeAddress, sliderArray[i]);
    eeAddress += sizeof(int); //Move address to the next byte after int
    EEPROM.put(eeAddress, sliderPosArray[i]);
    eeAddress += sizeof(int); //Move address to the next byte after int
    EEPROM.put(eeAddress, sliderOldArray[i]);
    eeAddress += sizeof(int); //Move address to the next byte after int
  }
}

void load() {
  int eeAddress = 0;   //Location we want the data to be put
  //One simple call, with the address first and the object second
  for(int i = 0 ; i < 24 ; i++) {
    //EEPROM.get(eeAddress, channelMapArray[i]);
    //eeAddress += sizeof(int); //Move address to the next byte after int
    EEPROM.get(eeAddress, sliderArray[i]);
    eeAddress += sizeof(int); //Move address to the next byte after int
    EEPROM.get(eeAddress, sliderPosArray[i]);
    eeAddress += sizeof(int); //Move address to the next byte after int
    EEPROM.get(eeAddress, sliderOldArray[i]);
    eeAddress += sizeof(int); //Move address to the next byte after int
  }
}
