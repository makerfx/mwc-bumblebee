
//
//Demovideo see :
//
//https://drive.google.com/drive/folders/0Bx2Jw84lqebkelF3MHg2eWVzb0U
//
// This sketch works with std 180MHz and less, but better with 240MHZ, or 144MHz and overclocked F_BUS to the max.
//
//For Audio, edit the Audio-library and remove all files which #include "SD.h" - sorry, no other way.
//


/*
 * around 10 vids
 * SongSelect = recieve
 * notes = send
 */


#include <Wire.h>
#include <SPI.h>
#include <Adafruit_CAP1188.h>

#define CAP1188_SENSITIVITY 0x1F
Adafruit_CAP1188 cap = Adafruit_CAP1188();

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            6 //right
#define PIN2           5 //left
#define PIN3           4 //middle

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16
#define NUMPIXELS2     24


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2 = Adafruit_NeoPixel(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels3 = Adafruit_NeoPixel(NUMPIXELS2, PIN3, NEO_GRB + NEO_KHZ800);

int delayval =  1500; // delay for half a second
int delayval2 = 1000; // delay for half a second
int delayval3 = 1000; // delay for half a second

bool flag1 = true;
bool flag2 = true;
bool flag3 = true;
bool button1_last = HIGH;
bool button2_last = HIGH;
bool button3_last = HIGH;
int button1 = 31; //right
int button2 = 30; //left
int button3 = 32; //horn
int channel = 3;
bool horn_flag = false;
int min_bright = 15;
int current_bright = 15;
int flash_bright = 25;

int dial_left_min = 4;
int dial_left_max = 9;
int dial_left_bounce = 15;
int dial_middle_min = 4;
int dial_middle_max = 6;
int dial_middle_bounce = 21;
int dial_right_min = 3;
int dial_right_max = 8;
int dial_right_bounce = 16;


#define USE_AUDIO 1
#include <SPI.h>
#include <ILI9341_t3DMA.h>
#include <SdFat.h>


#define TFT_DC      15
#define TFT_CS      10
#define TFT_RST     8  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12


  
uint32_t last_led = 0;
uint32_t last_button_hit = 0;
uint32_t last_bright = 0;
int color = 0;
int i = 0;
int vid_num = 1;

int playvid = 0;
int current_vid = 0;

uint32_t last_led2 = 0;
int color2 = 0;
int i2 = 0;

uint32_t last_led3 = 0;
int color3 = 0;
int i3 = 0;
bool bgflag = true;
bool button_pushed1 = false;
bool button_pushed2 = false;
bool button_pushed3 = false;
bool reset_leds = false;

ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

SdFatSdioEX SD;
File file;

#if USE_AUDIO
const char background_vid1[] = "0.bin";
//const char background_vid2[] = "Bumblebee clips.bin";
//const char background_vid3[] = "background2.bin";
const char button1_vid[] = "1.bin";
const char button2_vid[] = "2.bin";
const char button3_vid[] = "3.bin";
const char button4_vid[] = "4.bin";
const char button5_vid[] = "5.bin";
const char button6_vid[] = "6.bin";
const char button7_vid[] = "7.bin";
const char button8_vid[] = "8.bin";
const char button9_vid[] = "9.bin";
const char button10_vid[] = "10.bin";
const char button11_vid[] = "11.bin";
const char button12_vid[] = "12.bin";
#else
//const char background_vid[] = "backgroundVid.bin";
#endif

#if USE_AUDIO
//#include <Audio.h>
//AudioPlayQueue           queue1;         //xy=183,163
//AudioOutputAnalog        dac1;           //xy=376,154
//AudioConnection          patchCord1(queue1, dac1);
#endif

const float fps = 23.98;
const int rate_us = 1e6 / fps;
const uint32_t framesize = 153600;

#if USE_AUDIO
uint8_t header[512];
uint16_t audiobuf[16 * 320];
uint32_t audioPos = 0;
int16_t *lb = NULL;
//int16_t *rb = NULL;
#endif


void playVideo(const char * filename) {
  int bytesread;
  
  //Serial.print("Begin playing ");
  //Serial.println(filename);

  if (!file.open(filename, O_READ)) {
    SD.errorHalt("open failed");
    return;
  }

#if USE_AUDIO
  file.read(header, 512);  //read header - not used yet
#endif

  while (current_vid == playvid) {
    uint32_t m;
    m = micros();
    uint32_t * p = screen32;
    int rd = 0;

    rd = framesize;
    do {

      bytesread = file.read(p, rd);
      if (bytesread <= 0) {
        break;
      }
      p += bytesread / sizeof(uint32_t);
      rd -= bytesread;
    } while (bytesread > 0 && rd > 0);

    if (bytesread <= 0) {
      break;
    }

#if USE_AUDIO
    bytesread = file.read(audiobuf, sizeof(audiobuf));
    if (!bytesread) {
      break;
    }

    //Audio is 1839 samples*2 channels
    int16_t l = 0;
    for (int i = 0; i < 1839; i++) {
      if (audioPos == 0) {
//        lb = queue1.getBuffer();
      }
      if (lb) {
        *lb++ = audiobuf[i * 2];
        audioPos += 1;
        if (audioPos >= 128) {
          audioPos = 0;
//          queue1.playBuffer();
        }
      }
    }
#endif


    //Sync with framerate
    while (micros() - m < rate_us) {
      
      updateLeds(m);
      checkbuttons(m);
      ;
    }
  }

  file.close();
  if(current_vid == playvid){
    playvid = 0;
  }
  //playvid = 0;
  
  bgflag == true;
  Serial.println("EOF");
}


void playBackgroundVideo(const char * filename) {
  int bytesread;
  
  Serial.print("Begin playing ");
  Serial.println(filename);

  if (!file.open(filename, O_READ)) {
    SD.errorHalt("open failed");
    return;
  }

#if USE_AUDIO
  file.read(header, 512);  //read header - not used yet
#endif

  while (playvid == 0) {
    uint32_t m;
    m = micros();
    uint32_t * p = screen32;
    int rd = 0;

    rd = framesize;
    do {

      bytesread = file.read(p, rd);
      if (bytesread <= 0) {
        break;
      }
      p += bytesread / sizeof(uint32_t);
      rd -= bytesread;
    } while (bytesread > 0 && rd > 0);

    if (bytesread <= 0) {
      break;
    }

#if USE_AUDIO
    bytesread = file.read(audiobuf, sizeof(audiobuf));
    if (!bytesread) {
      break;
    }

    //Audio is 1839 samples*2 channels
    int16_t l = 0;
    for (int i = 0; i < 1839; i++) {
      if (audioPos == 0) {
//        lb = queue1.getBuffer();
      }
      if (lb) {
        *lb++ = audiobuf[i * 2];
        audioPos += 1;
        if (audioPos >= 128) {
          audioPos = 0;
//          queue1.playBuffer();
        }
      }
    }
#endif


    //Sync with framerate
    while (micros() - m < rate_us) {


 

      //read midi for input
      if (usbMIDI.read()) {
        processMIDI();
      }
      //button pressed
      checkbuttons(m);
      updateLeds(m);
    }
  }

  file.close();
  Serial.println("EOF");
}


void checkbuttons(uint32_t current_time){
  //Serial.println(digitalRead(button3));
  
  if(digitalRead(button1) != button1_last && digitalRead(button1) == LOW){
        //bgflag=false;
        if(current_time - last_button_hit > 200000){
        Serial.println("button 1 hit");
        usbMIDI.sendNoteOn(49,100,channel);
        button1_last = digitalRead(button1);
        last_button_hit = current_time;
        /*
        playvid = vid_num;
        Serial.print("playvid: ");
        Serial.println(playvid);
        Serial.print("current_vid: ");
        Serial.println(current_vid);

        vid_num++;
        if(vid_num > 12){
          vid_num = 1;
        }
        */
        //current_vid = playvid;
        button_pushed1 = true;
        flag1 = true;
        button_pushed2 = true;
        flag3 = true;
        button_pushed3 = true;
        flag3 = true;
        current_bright = flash_bright;
        pixels.setBrightness(current_bright);
        pixels2.setBrightness(current_bright);
        pixels3.setBrightness(current_bright);
        }
        return;
      }
      else if(digitalRead(button1) != button1_last && digitalRead(button1) == HIGH){
        button1_last = digitalRead(button1);
      }
      

  if(digitalRead(button2) != button2_last && digitalRead(button2) == LOW){
        //bgflag=false;
        if(current_time - last_button_hit > 200000){
        Serial.println("button 2 hit");
        
        //Serial.println(i);
        //Serial.println(flag1);
        //Serial.println(current_time);
        //Serial.println(last_button_hit);
        //Serial.println(current_time - last_button_hit);
        usbMIDI.sendNoteOn(48,100,channel);
        button2_last = digitalRead(button2);
        last_button_hit = current_time;
        playvid = vid_num;
        Serial.print("playvid: ");
        Serial.println(playvid);
        Serial.print("current_vid: ");
        Serial.println(current_vid);
        //current_vid = playvid;

        vid_num++;
        if(vid_num > 12){
          vid_num = 1;
        }
        
        button_pushed1 = true;
        flag1 = true;
        button_pushed2 = true;
        flag3 = true;
        button_pushed3 = true;
        flag3 = true;
        current_bright = flash_bright;
        pixels.setBrightness(current_bright);
        pixels2.setBrightness(current_bright);
        pixels3.setBrightness(current_bright);
        }
        return;
      }
      else if(digitalRead(button2) != button2_last && digitalRead(button2) == HIGH){
        button2_last = digitalRead(button2);
      }

   if(digitalRead(button3) != button3_last && digitalRead(button3) == LOW){
        //bgflag=false;
        if(current_time - last_button_hit > 1000000){
        Serial.println("button 3 hit");
        //Serial.println(current_time);
        //Serial.println(last_button_hit);
        //Serial.println(current_time - last_button_hit);
        usbMIDI.sendNoteOn(44,100,channel);
        button3_last = digitalRead(button3);
        last_button_hit = current_time;
        button_pushed1 = true;
        flag1 = true;
        button_pushed2 = true;
        flag3 = true;
        button_pushed3 = true;
        flag3 = true;
        current_bright = flash_bright;
        pixels.setBrightness(current_bright);
        pixels2.setBrightness(current_bright);
        pixels3.setBrightness(current_bright);
        
        
        }
        
      }
      else if(digitalRead(button3) != button3_last && digitalRead(button3) == HIGH){
        button3_last = digitalRead(button3);
      }
      
}


void updateLeds(uint32_t m){

if(m - last_led > delayval){
        //Serial.println(last_led);
        
         
         if(flag1 == true){
          pixels.setPixelColor(i, pixels.Color(0,color,150)); // Moderately bright green color.
          pixels.show(); // This sends the updated pixel color to the hardware.
          //Serial.println("on");
          i++;
          
         }
         else{
          pixels.setPixelColor(i, pixels.Color(0,0,0)); // Moderately bright green color.
          pixels.show(); // This sends the updated pixel color to the hardware.
          i--;
          //Serial.println("off");
         }
         
         last_led = m;
        
         //if(i > NUMPIXELS){
         if(i > random(dial_right_max, dial_right_bounce) && button_pushed1 == false){
          pixels2.setPixelColor(i, pixels2.Color(0,0,0)); // Moderately bright green color.
          i--;
          flag1 = false;
          //color = color + 100;
          if(color > 255){
            color = 0;
          }
         }
         else if(i > dial_right_bounce && button_pushed1 == true){
          i--;
          flag1 = false;
          //color = color + 100;
          if(color > 255){
            color = 0;
          }
         }
         else if(i < dial_right_max){
          i = dial_right_max;
          clear_leds(1);
          button_pushed1 = false;
          flag1 = true;
          if(current_bright == min_bright && reset_leds == true){
            for(int i = dial_right_min; i < dial_right_max; i++){
              pixels.setPixelColor(i, pixels.Color(0,color,150)); // Moderately bright green color.
            }
          }
         }
      }

      if(m - last_led2 > delayval2){
        if(flag1 == true){
          pixels2.setPixelColor(i2, pixels2.Color(0,color2,150)); // Moderately bright green color.
          pixels2.show(); // This sends the updated pixel color to the hardware.
          //Serial.println("on");
          i2++;
          
         }
         else{
          pixels2.setPixelColor(i2, pixels2.Color(0,0,0)); // Moderately bright green color.
          pixels2.show(); // This sends the updated pixel color to the hardware.
          i2--;
          //Serial.println("off");
         }
         
         last_led2 = m;
        
         //if(i2 > NUMPIXELS){
          if(i2 > random(dial_left_max, dial_left_bounce) && button_pushed2 == false){
          pixels2.setPixelColor(i2, pixels2.Color(0,0,0)); // Moderately bright green color.
          i2--;
          flag2 = false;
          //color2 = color2 + 100;
          if(color2 > 255){
            color2 = 0;
          }
         }
         if(i2 > dial_left_bounce && button_pushed2 == true){
          //pixels2.setPixelColor(i2, pixels2.Color(0,0,0)); // Moderately bright green color.
          i2--;
          flag2 = false;
          //color2 = color2 + 100;
          if(color2 > 255){
            color2 = 0;
          }
         }
         else if(i2 < dial_left_max){
          i2 = dial_left_max;
          clear_leds(2);
          flag2 = true;
          button_pushed2 = false;
          if(current_bright == min_bright && reset_leds == true){
            for(int i = dial_left_min; i < dial_left_max; i++){
              pixels2.setPixelColor(i, pixels2.Color(0,color2,150)); // Moderately bright green color.
            }
          }
         }
      }

      if(m - last_led3 > delayval3){
        //Serial.println(last_led3);
         if(flag3 == true){
          pixels3.setPixelColor(i3, pixels3.Color(0,color3,150)); // Moderately bright green color.
          pixels3.show(); // This sends the updated pixel color to the hardware.
          //Serial.println("on");
          i3++;
          
         }
         else{
          pixels3.setPixelColor(i3, pixels3.Color(0,0,0)); // Moderately bright green color.
          pixels3.show(); // This sends the updated pixel color to the hardware.
          i3--;
          //Serial.println("off");
         }
         
         last_led3 = m;
        
         //if(i3 > NUMPIXELS2){
         if(i3 > random(dial_middle_max, dial_middle_bounce) && button_pushed3 == false){
          pixels3.setPixelColor(i3, pixels3.Color(0,0,0)); // Moderately bright green color.
          i3--;
          flag3 = false;
          //color3 = color3 + 100;
          if(color3 > 255){
            color = 0;
          }
         }
         if(i3 > dial_middle_bounce && button_pushed3 == true){
          //pixels3.setPixelColor(i3, pixels3.Color(0,0,0)); // Moderately bright green color.
          i3--;
          flag3 = false;
          //color3 = color3 + 100;
          if(color3 > 255){
            color = 0;
          }
         }
         else if(i3 < dial_middle_max){
          i3 = dial_middle_max;
          clear_leds(3);
          flag3 = true;
          button_pushed3 = false;
          if(current_bright == min_bright && reset_leds == true){
            for(int i = dial_middle_min; i < dial_middle_max; i++){
              pixels3.setPixelColor(i, pixels3.Color(0,color3,150)); // Moderately bright green color.
            }
          }
         }
      }
      if(current_bright > min_bright && m - last_bright > 10000){
        current_bright--;
        pixels.setBrightness(current_bright);
        pixels2.setBrightness(current_bright);
        pixels3.setBrightness(current_bright);
        for(int i = dial_middle_min; i < dial_middle_bounce; i++){
          pixels3.setPixelColor(i, pixels3.Color(255,255,255)); // Moderately bright green color.
        }
        for(int i = dial_right_min; i < dial_right_bounce; i++){
          //pixels2.setPixelColor(i, pixels2.Color(255,255,255)); // Moderately bright green color.
          pixels.setPixelColor(i, pixels.Color(255,255,255)); // Moderately bright green color.
        }
        for(int i = dial_left_min; i < dial_left_bounce; i++){
          pixels2.setPixelColor(i, pixels2.Color(255,255,255)); // Moderately bright green color.
          //pixels.setPixelColor(i, pixels.Color(255,255,255)); // Moderately bright green color.
        }
        last_bright = m;
        reset_leds = true;
      }
      
  
}

void clear_leds(int led){
  switch(led){
    case 1: //right
      for(int i = 0; i <= NUMPIXELS; i++){
        if(i >= dial_right_min && i < dial_right_max){ //blue always
          pixels.setPixelColor(i, pixels.Color(0,color,150)); 
        }
        else if(i < dial_right_min or i > dial_right_bounce){ //never used so gold
          pixels.setPixelColor(i, pixels.Color(255, 255, 0));
        }
        else{// its in the bounce range blank it
          pixels.setPixelColor(i, pixels.Color(0,0,0));
        }
        
      }
      return;

      
    case 2: //left

      for(int i = 0; i < NUMPIXELS; i++){
        if(i >= dial_left_min && i < dial_left_max){ //blue always
          pixels2.setPixelColor(i, pixels2.Color(0,color,150)); 
        }
        else if(i < dial_left_min or i > dial_left_bounce){ //never used so gold
          pixels2.setPixelColor(i, pixels2.Color(255, 255, 0));
        }
        else{// its in the bounce range blank it
          pixels2.setPixelColor(i, pixels2.Color(0,0,0));
        }
        
      }
      return;

      
    case 3: //middle

      for(int i = 0; i < NUMPIXELS2; i++){
        if(i >= dial_middle_min && i < dial_middle_max){ //blue always
          pixels3.setPixelColor(i, pixels3.Color(0,color,150)); 
        }
        else if(i < dial_middle_min or i > dial_middle_bounce){ //never used so gold
          pixels3.setPixelColor(i, pixels3.Color(255, 255, 0));
        }
        else{// its in the bounce range blank it
          pixels3.setPixelColor(i, pixels3.Color(0,0,0));
        }
        
      }
      return;

      
    default:
      return;
  }
}


void processMIDI(void) {
  byte type, channel, data1, data2, cable;

  // fetch the MIDI message, defined by these 5 numbers (except SysEX)
  //
  type = usbMIDI.getType();       // which MIDI message, 128-255
  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
  cable = usbMIDI.getCable();     // which virtual cable with MIDIx8, 0-7

  // uncomment if using multiple virtual cables
  //Serial.print("cable ");
  //Serial.print(cable, DEC);
  //Serial.print(": ");

  // print info about the message
  //
  switch (type) {
    case usbMIDI.NoteOff: // 0x80
      Serial.print("Note Off, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      break;

    case usbMIDI.NoteOn: // 0x90
      Serial.print("Note On, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      break;

    case usbMIDI.AfterTouchPoly: // 0xA0
      Serial.print("AfterTouch Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      break;

    case usbMIDI.ControlChange: // 0xB0
      Serial.print("Control Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", control=");
      Serial.print(data1, DEC);
      Serial.print(", value=");
      Serial.println(data2, DEC);
      break;

    case usbMIDI.ProgramChange: // 0xC0
      Serial.print("Program Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", program=");
      Serial.println(data1, DEC);
      
      break;

    case usbMIDI.AfterTouchChannel: // 0xD0
      Serial.print("After Touch, ch=");
      Serial.print(channel, DEC);
      Serial.print(", pressure=");
      Serial.println(data1, DEC);
      break;

    case usbMIDI.PitchBend: // 0xE0
      Serial.print("Pitch Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", pitch=");
      Serial.println(data1 + data2 * 128, DEC);
      break;

    case usbMIDI.SystemExclusive: // 0xF0
      // Messages larger than usbMIDI's internal buffer are truncated.
      // To receive large messages, you *must* use the 3-input function
      // handler.  See InputFunctionsComplete for details.
      Serial.print("SysEx Message: ");
      printBytes(usbMIDI.getSysExArray(), data1 + data2 * 256);
      Serial.println();
      break;

    case usbMIDI.TimeCodeQuarterFrame: // 0xF1
      Serial.print("TimeCode, index=");
      Serial.print(data1 >> 4, DEC);
      Serial.print(", digit=");
      Serial.println(data1 & 15, DEC);
      break;

    case usbMIDI.SongPosition: // 0xF2
      Serial.print("Song Position, beat=");
      Serial.println(data1 + data2 * 128);
      break;

    case usbMIDI.SongSelect: // 0xF3
      Serial.print("Sond Select, song=");
      Serial.println(data1, DEC);
      digitalWrite(13, !digitalRead(13));

      playvid = (int)data1;
      Serial.print("playvid Midi: ");
      Serial.println(playvid);
      Serial.print("current_vid Midi: ");
      Serial.println(current_vid);
        //current_vid = playvid;

      
      break;

    case usbMIDI.TuneRequest: // 0xF6
      Serial.println("Tune Request");
      break;

    case usbMIDI.Clock: // 0xF8
      Serial.println("Clock");
      break;

    case usbMIDI.Start: // 0xFA
      Serial.println("Start");
      break;

    case usbMIDI.Continue: // 0xFB
      Serial.println("Continue");
      break;

    case usbMIDI.Stop: // 0xFC
      Serial.println("Stop");
      break;

    case usbMIDI.ActiveSensing: // 0xFE
      Serial.println("Actvice Sensing");
      break;

    case usbMIDI.SystemReset: // 0xFF
      Serial.println("System Reset");
      break;

    default:
      Serial.println("Opps, an unknown MIDI message type!");
  }
}


void printBytes(const byte *data, unsigned int size) {
  while (size > 0) {
    byte b = *data++;
    if (b < 16) Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1) Serial.print(' ');
    size = size - 1;
  }
}


void setup() {

#if USE_AUDIO
//  AudioMemory(10);
#endif

  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code
  pixels.begin(); // This initializes the NeoPixel library.
  pixels2.begin(); // This initializes the NeoPixel library.
  pixels3.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(min_bright);
  pixels2.setBrightness(min_bright);
  pixels3.setBrightness(min_bright);
  Serial.begin(9600);
  delay(1000);
  Serial.println("started");
 // cap.begin();
  if (!cap.begin()) {
    Serial.println("CAP1188 not found");
    while (1);
  }
  Serial.println("CAP1188 found!");

  //Decrease sensitivity a little - default is 0x2F (32x) per datasheet
  //cap.writeRegister(CAP1188_SENSITIVITY, 0x3F);  // 16x sensitivity
  //cap.writeRegister(CAP1188_SENSITIVITY, 0x4F);  // 8x  sensitivity
  cap.writeRegister(CAP1188_SENSITIVITY, 0x5F);  // 4x  sensitivity
  //cap.writeRegister(CAP1188_SENSITIVITY, 0x6F);  // 2x  sensitivity THIS SEEMS TO WORK THE BEST FOR 3.5" plate sensors
  //cap.writeRegister(CAP1188_SENSITIVITY, 0x7F);  // 1x  sensitivity
  Serial.print("Sensitivity: 0x");
  Serial.println(cap.readRegister(CAP1188_SENSITIVITY), HEX);
  
  SD.begin();
  
  SPI.begin();

 
  tft.begin();
  tft.fillScreen(ILI9341_BLUE);

  tft.refresh();
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT);
for(int i = dial_middle_min; i < dial_middle_max; i++){
  pixels3.setPixelColor(i, pixels3.Color(0,color3,150)); // Moderately bright green color.
}
for(int i = dial_right_min; i < dial_right_max; i++){
  pixels.setPixelColor(i, pixels.Color(0,color,150)); // Moderately bright green color.
}

for(int i = dial_left_min; i < dial_left_max; i++){
  pixels2.setPixelColor(i, pixels2.Color(0,color2,150)); // Moderately bright green color.
}  
  delay(500);

}

void loop() {
//  uint8_t touched = cap.touched();
  switch(playvid){
    case 0:
      playBackgroundVideo(background_vid1);
      break;
    case 1:
      current_vid = 1;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button1_vid);
      break;
    case 2:
      current_vid = 2;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button2_vid);
      break;
    case 3:
      current_vid = 3;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button3_vid);
      break;
    case 4:
      current_vid = 4;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button4_vid);
      break;
    case 5:
      current_vid = 5;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button5_vid);
      break;
    case 6:
      current_vid = 6;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button6_vid);
      break;
    case 7:
      current_vid = 7;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button7_vid);
      break;
    case 8:
      current_vid = 8;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button8_vid);
      break;
    case 9:
      current_vid = 9;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button9_vid);
      break;
    case 10:
      current_vid = 10;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button10_vid);
      break;
    case 11:
      current_vid =11;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button11_vid);
      break;
    case 12:
      current_vid = 12;
      Serial.print("current_vid: ");
      Serial.println(current_vid);
        //Serial.println(playvid);
      playVideo(button12_vid);
      break;
    default:
      break;
  }
}
