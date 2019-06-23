// Magic Wheelchair - Bumblebee 
//
// reference: Using USB MIDI - https://www.pjrc.com/teensy/td_midi.html
// IMPORTANT NOTE: 8.3 FILENAMES FOR WAV AUDIO FILES!
// IMPORTANT NOTE: WAV 44100 STEREO 16BIT


bool debugOptions[10] = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0};   //change default here, helpful for startup debugging

                                                        
const char* debugOptionsText[10] =  {"", "Input","Audio", "Action", "Peak Audio", "FFT Audio",
                                "Animation","Animation RGB Dump"};
                                
#define DEBUG_INPUT               1  //input functions 
#define DEBUG_AUDIO               2  //audio functions 
#define DEBUG_ACTION              3  //action functions 
#define DEBUG_PEAK                4  //Peak Audio functions
#define DEBUG_FFT                 5  //Peak Audio functions  
#define DEBUG_ANIMATION           6  //Animation functions   //THIS MAY SLOW ANIMATION                            
#define DEBUG_ANIMATION_RGB       7  //Show Full Animation Frame RGB 
                                     //Does not require DEBUG_ANIMATION to be on
                                     //THIS MAY SLOW ANIMATION
                                     


/*
 * Audio System Includes & Globals
 * Reference the Audio Design Tool - https://www.pjrc.com/teensy/gui/index.html
 * 
 */

#define NUM_HORN_WAVS     2
#define LINE_IN_MONITOR   .5  //set this from 0 to 1.0 to have passthrough
#define TOTAL_VOICES      4

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
 

// GUItool: begin automatically generated code
AudioSynthWavetable      wavetable[TOTAL_VOICES]; //xy=137.5,87.0001859664917
AudioPlaySdWav           playSdWav1;     //xy=180.3334197998047,239.00009775161743
AudioInputI2S            i2sIN;          //xy=182.0002212524414,434.99998712539673
AudioMixer4              mxWave;         //xy=191.666748046875,322.6667580604553
AudioEffectFade          fadeR;          //xy=336.3333320617676,459.00007486343384
AudioEffectFade          fadeL;          //xy=336.6667289733887,413.66669273376465
AudioMixer4              mxInL;          //xy=521.666748046875,251.0001072883606
AudioMixer4              mxInR;          //xy=522.666748046875,329.0001072883606
AudioOutputI2S           i2sOUT;         //xy=710.3333282470703,280.6668019294739
AudioMixer4              mxFFT;          //xy=713.0000495910645,158.66676950454712
AudioAnalyzePeak         peakIn;         //xy=877.3333854675293,178.66677331924438
AudioAnalyzeFFT1024      fft1024_1;      //xy=889.3333854675293,123.66677331924438
AudioConnection          patchCord1(playSdWav1, 0, mxInL, 0);
AudioConnection          patchCord2(playSdWav1, 1, mxInR, 0);
AudioConnection          patchCord3(i2sIN, 0, fadeL, 0);
AudioConnection          patchCord4(i2sIN, 1, fadeR, 0);
AudioConnection          patchCord5(mxWave, 0, mxInL, 1);
AudioConnection          patchCord6(mxWave, 0, mxInR, 1);
AudioConnection          patchCord7(fadeR, 0, mxInR, 2);
AudioConnection          patchCord8(fadeL, 0, mxInL, 2);
AudioConnection          patchCord9(mxInL, 0, mxFFT, 2);
AudioConnection          patchCord10(mxInL, 0, i2sOUT, 0);
AudioConnection          patchCord11(mxInR, 0, mxFFT, 3);
AudioConnection          patchCord12(mxInR, 0, i2sOUT, 1);
AudioConnection          patchCord13(mxFFT, fft1024_1);
AudioConnection          patchCord14(mxFFT, peakIn);
AudioControlSGTL5000     sgtl5000_1;     //xy=517.666748046875,432.0001072883606
// GUItool: end automatically generated code

//THESE WILL NOT WORK IN THE AUDIO TOOL
AudioConnection          patchCordmxWave0(wavetable[0], 0, mxWave, 0);
AudioConnection          patchCordmxWave1(wavetable[1], 0, mxWave, 1);
AudioConnection          patchCordmxWave2(wavetable[2], 0, mxWave, 2);
AudioConnection          patchCordmxWave3(wavetable[3], 0, mxWave, 3);


/*
 * PlayQueue - to avoid calling audio system during USB event handler
 * Note that we only have one playSdWave object on Bumblebee, 
 * so no need for array here as we did on tie-silencer
 * 
 */
String playQueue;
 


/*
 * MIDI Synth Includes & Globals
 * Reference: MidiSynthKeyboard example from Teensy Audio Examples
 * 
 */
 
//SYNTH Includes
#include "standard_DRUMS_samples.h"
#include "piano_samples.h"
#include "distortiongt_samples.h"
#include "trumpet_samples.h"
#include "timpani_samples.h"


//Synth Globals
int allocateVoice(byte channel, byte note);
int findVoice(byte channel, byte note);
void freeVoices();

int used_voices = 0;
int stopped_voices = 0;
int evict_voice = 0;
int notes_played = 0;

#define NUM_INSTRUMENTS 5
#define MIN_NOTE_VELOCITY 64

int colorCycle = 0;

struct voice_t {
  int wavetable_id;
  byte channel;
  byte note;
};
voice_t voices[TOTAL_VOICES];

//LED ALL THE THINGS!
#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>


#define MAX_CYCLE_LEDS 64 //number of LEDs in the longest cycle object


#define HL_NUM_LEDS 24 
#define HL_DATA_PIN 33
CRGB hlLEDS[HL_NUM_LEDS];

#define EYE_NUM_LEDS 16 
#define EYE_DATA_PIN 8
CRGB eyeLEDS[EYE_NUM_LEDS]; //Two Rings

#define TRAY_NUM_LEDS 52 
#define TRAY_DATA_PIN 26
CRGB trayLEDS[TRAY_NUM_LEDS]; 

#define BLING_NUM_LEDS 10
#define BLING_DATA_PIN 32
CRGB blingLEDS[BLING_NUM_LEDS]; 

#define FLARE_BRIGHTNESS 255
#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_HEADLIGHT_BRIGHTNESS 255

#define FLARE_COLOR CRGB::White 

#define FFT_MIN 64 //this prevents some colors from showing as red at low values in FFT 


#define HL_SPOT_LEFT_PIN      29  
#define HL_SPOT_RIGHT_PIN     30
#define GFX_RED_PIN           35
#define GFX_BLUE_PIN          36
#define GFX_GREEN_PIN         37




//Show & Mode Globals

#include <Metro.h> //Include Metro library
Metro showMetro = Metro(100); 
Metro fftMetro = Metro(50); 
Metro chaseMetro = Metro(20); 
Metro playQueueMetro = Metro(50);
Metro bgmMetro = Metro(500);
Metro colorChangeMetro = Metro(10000000);

#define SHOW_DEFAULT  0
#define SHOW_FLARE    1
#define SHOW_SPEAKING 2
#define SHOW_PLAYING  3
#define SHOW_HONKING  4

#define NUM_MODES       8
#define DEFAULT_MODE    3

#define MODE_OFF            0
#define MODE_FFT_SPLIT      1
#define MODE_FFT_STRAIGHT   2
#define MODE_PEAK_SPLIT     3
#define MODE_PEAK_STRAIGHT  4
#define MODE_CHASE1         5
#define MODE_CHASE2         6
#define MODE_CHASE3         7

const char* modeText[NUM_MODES] =  {"Off", "FFT Split","FFT Straight", "Peak Split", "Peak Straight", "Chase 1",
                                "Chase 2","Chase 3"};


int currentChasePosition = 0;
int lastChasePosHL = 0;
int lastChasePosEYE = 0;
int lastChasePosTRAY = 0;
int lastChasePosBLING = 0;

#define CHASE3_LENGTH 5

int currentShow = 0; //shows = speaking / playing, honking, default, off
int currentMode = DEFAULT_MODE; //modes are what is happening during the default show; fft, chase, etc.
#define NUM_MODE_COLORS 6
//CRGB modeColors[NUM_MODE_COLORS] = {CRGB::Gold, CRGB::Blue, CRGB::Red, CRGB::Orange, CRGB::Green, CRGB::Aqua, CRGB::Purple, CRGB::Pink};

//colors are not mapping right for some reason, got these by experimentation
#define H_RED       0
#define H_PINK      42
#define H_PURPLE    85
#define H_AQUA      128
#define H_GREEN     171
#define H_YELLOW    213

const char* colorText[NUM_MODE_COLORS] =  {"Red", "Pink","Aqua", "Green", "Blue", "Yellow"};


#define DEFAULT_MODE_COLOR 4

int modeColorHueVals[NUM_MODE_COLORS] = {H_RED, H_YELLOW, H_AQUA, H_PURPLE, H_GREEN, H_PINK}; 
//int modeColorHueVals[NUM_MODE_COLORS] = {HUE_YELLOW, HUE_BLUE, HUE_PURPLE, HUE_RED, HUE_GREEN}; 
int currentModeColor = DEFAULT_MODE_COLOR; //start yellow



int leftHeadlightStatus = 0;
int rightHeadlightStatus = 0;



//for USB host functions
#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
KeyboardController keyboard1(myusb);
MIDIDevice midi1(myusb);
MIDIDevice midi2(myusb);

// this is for wav file playback to reduce locking
unsigned long lastPlayStart = 0;

// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

#define NUM_VOICE_FILES 26
#define NUM_SOUND_FILES 7
int curInstrument = 0;


#define SOURCE_KEY                    0
#define SOURCE_MIDI_NOTE              1
#define SOURCE_MIDI_CONTROL_CHANGE    2


#define ACTION_DO_NOT_USE             0 //just putting this here as a reminder to not use it :)
#define ACTION_CHANGE_COLOR           1
#define ACTION_CHANGE_SPEED           2
#define ACTION_CHANGE_MODE            3
#define ACTION_CHANGE_INSTRUMENT      4

#define ACTION_PLAY_WAV               10
#define ACTION_PLAY_WAV_RND           11
#define ACTION_PLAY_WAV_TRANSFORM     12
#define ACTION_PLAY_WAV_OPTIMUS       13
#define ACTION_PLAY_WAV_HORN          14
#define ACTION_PLAY_WAV_NAMEIS        15


//#define ACTION_PLAY_NOTE              20

#define ACTION_FLARE                  30
#define ACTION_HEADLIGHT_TOGGLE       31

#define ACTION_STOP                   99


#define ACTION_MAP_SIZE 21

/*
 * The ActionMap allows us to have multiple types of input events
 * that get mapped to an action. We can have a MIDI note, or an HID keyboard
 * key, etc. that all go to the same action
 * 
 */
int ActionMap[][3] = {
  //src, key, action
  //{SOURCE_KEY, 214, ACTION_CHANGE_COLOR},              //remote right
  //{SOURCE_KEY, 211, ACTION_CHANGE_MODE},               //remote left
  {SOURCE_KEY, 214, ACTION_PLAY_WAV_HORN},              //remote right
  {SOURCE_KEY, 211, ACTION_PLAY_WAV_TRANSFORM},               //remote left

  {SOURCE_KEY,  27, ACTION_PLAY_WAV_NAMEIS},           //remote up
  {SOURCE_KEY,  98, ACTION_HEADLIGHT_TOGGLE},        //remote down
  {SOURCE_KEY, 198, ACTION_PLAY_WAV_RND},              //remote play
  
  {SOURCE_MIDI_NOTE, 44, ACTION_PLAY_WAV_HORN},             //MIDI pad 1 BANK A + Dashboard (CH3) Horn
  {SOURCE_MIDI_NOTE, 32, ACTION_PLAY_WAV_HORN},             //MIDI pad 1 BANK B
  {SOURCE_MIDI_NOTE, 45, ACTION_PLAY_WAV_TRANSFORM},        //MIDI pad 2 BANK A
  {SOURCE_MIDI_NOTE, 33, ACTION_PLAY_WAV_TRANSFORM},        //MIDI pad 2 BANK B
  {SOURCE_MIDI_NOTE, 46, ACTION_PLAY_WAV_NAMEIS},          //MIDI pad 3 BANK A
  {SOURCE_MIDI_NOTE, 34, ACTION_PLAY_WAV_NAMEIS},          //MIDI pad 3 BANK B
  {SOURCE_MIDI_NOTE, 47, ACTION_HEADLIGHT_TOGGLE},          //MIDI pad 4 BANK A
  {SOURCE_MIDI_NOTE, 35, ACTION_HEADLIGHT_TOGGLE},          //MIDI pad 4 BANK B
  {SOURCE_MIDI_NOTE, 48, ACTION_PLAY_WAV_RND},              //MIDI pad 5 BANK A + Dashboard (CH3) Left Button
  {SOURCE_MIDI_NOTE, 36, ACTION_PLAY_WAV_RND},              //MIDI pad 5 BANK B
  {SOURCE_MIDI_NOTE, 49, ACTION_CHANGE_COLOR},              //MIDI pad 6 BANK A + Dashboard (CH3) Right Button
  {SOURCE_MIDI_NOTE, 37, ACTION_CHANGE_COLOR},              //MIDI pad 6 BANK B
  {SOURCE_MIDI_NOTE, 50, ACTION_CHANGE_MODE},               //MIDI pad 7 BANK A
  {SOURCE_MIDI_NOTE, 38, ACTION_CHANGE_MODE},               //MIDI pad 7 BANK B
  {SOURCE_MIDI_NOTE, 51, ACTION_CHANGE_INSTRUMENT},         //MIDI pad 8 BANK A
  {SOURCE_MIDI_NOTE, 39, ACTION_CHANGE_INSTRUMENT},         //MIDI pad 8 BANK B  NOTE: MPK MIDI KEYS on CH1, PADS on CH2
   
};

/*
 * TODO: REMOVE THESE? I believe these were replaced with the SOURCE_ mappings above
 */
#define BB_REMOTE 1
#define BB_MIDI_NOTE 1
#define BB_MIDI_CONTROL 2
#define BB_MIDI_PITCH 3


void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("Setup Started.");

  //setup audio system
  AudioMemory(128);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);
  
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  //SYNTH Setup
  actionChangeInstrument(false); //setup instrument
  

  //seed random function
  randomSeed(analogRead(0));

  //USB Host Setup
  Serial.println("USB Host Setup");
  myusb.begin();
  keyboard1.attachPress(OnPress);
  
  
  midi1.setHandleNoteOff(OnNoteOff);
  midi1.setHandleNoteOn(OnNoteOn);
  midi1.setHandleControlChange(OnControlChange);
  midi2.setHandleNoteOff(OnNoteOff);
  midi2.setHandleNoteOn(OnNoteOn);
  midi2.setHandleControlChange(OnControlChange);

 //Serial.printf("*** Device %s %x:%x - connected ***\n", "midi1", (USBDriver) midi1->idVendor(), (USBDriver) midi1->idProduct());
 //Serial.printf("*** Device %s %x:%x - connected ***\n", "keyboard1", (USBHIDInput) &keyboard1->idVendor(), (USBHIDInput) &keyboard1->idProduct());


   mxFFT.gain(0, 1);    //fft
   mxFFT.gain(1, 1);    //fft
  //mxOutL.gain(0, 1);   //no effects
  //mxOutR.gain(0, 1);   //no effects
  //mxOutL.gain(1, 0.0); //granular
  //mxOutR.gain(1, 0.0); //granular

  mxInL.gain(0, .3); //sdwav //the external input is lower, so this is set down to compensate
  mxInR.gain(0, .3); //sdwav
  mxInL.gain(1, 1); //wavetable
  mxInR.gain(1, 1); //wavetable
  mxInL.gain(2, LINE_IN_MONITOR); //i2s in
  mxInR.gain(2, LINE_IN_MONITOR); //i2s in

  
  mxWave.gain(0, 1);
  mxWave.gain(1, 1);
  mxWave.gain(2, 1);
  mxWave.gain(3, 1);

  pinMode(HL_SPOT_LEFT_PIN, OUTPUT); 
  pinMode(HL_SPOT_RIGHT_PIN, OUTPUT);
  pinMode(GFX_RED_PIN, OUTPUT);
  pinMode(GFX_GREEN_PIN, OUTPUT);
  pinMode(GFX_BLUE_PIN, OUTPUT);

/* Reducing current at startup
  int testDelay = 100;
  analogWrite(HL_SPOT_LEFT_PIN, 255);
  delay(testDelay);
  analogWrite(HL_SPOT_LEFT_PIN, 0);
  
  analogWrite(HL_SPOT_RIGHT_PIN, 255);
  delay(testDelay);
  analogWrite(HL_SPOT_RIGHT_PIN, 0);

  analogWrite(GFX_RED_PIN, 255);
  delay(testDelay);
  analogWrite(GFX_RED_PIN, 0);
  
  analogWrite(GFX_GREEN_PIN, 255);
  delay(testDelay);
  analogWrite(GFX_GREEN_PIN, 0);

  analogWrite(GFX_BLUE_PIN, 255);
  delay(testDelay);
  analogWrite(GFX_BLUE_PIN, 0);
  
*/
  //Rings like BRG space to be the same as the RGB strip
  //TODO: NEED TO CALIBRATE OTHER RINGS
  LEDS.addLeds<WS2812SERIAL,EYE_DATA_PIN,  BRG>(eyeLEDS,   EYE_NUM_LEDS);
  LEDS.addLeds<WS2812SERIAL,TRAY_DATA_PIN, BRG>(trayLEDS,  TRAY_NUM_LEDS); //not rgb, grb, bgr
  LEDS.addLeds<WS2812SERIAL,HL_DATA_PIN,   BRG>(hlLEDS,    HL_NUM_LEDS);
  LEDS.addLeds<WS2812SERIAL,BLING_DATA_PIN,BRG>(blingLEDS,    BLING_NUM_LEDS);
  
  LEDS.setBrightness(DEFAULT_BRIGHTNESS);


  FastLED.clear();
  eyeLEDS[0] = CRGB(255,255,255);
  trayLEDS[0] = CRGB(255,255,255);
  hlLEDS[0] = CRGB(255,255,255);
  blingLEDS[0] = CRGB(255,255,255);
  FastLED.show();
  delay(1000);

  playSdWav1.play("HORN1.WAV");

  delay(2000);

  playSdWav1.play("MYNAMEIS.WAV");


  
  Serial.println("Setup Complete.");
  printDebugOptions();
  
}

void loop() {

  myusb.Task();
  midi1.read();
  midi2.read();

  if (showMetro.check() == 1) { // check if the metro has passed its interval .
      updateModeShow(); //update the show 10 times a second 
      } //end showMetro check
 
  if (fftMetro.check() == 1) { // check if the metro has passed its interval .
      if (currentShow == SHOW_DEFAULT) {
        switch (currentMode) {
          case MODE_FFT_STRAIGHT:    
          case MODE_FFT_SPLIT:       updateFFT(); break;
          case MODE_PEAK_STRAIGHT:   
          case MODE_PEAK_SPLIT:      updatePeak(); break;
         
        } //end switch
      } //end if
    } //end fftMetro check
  
  if (chaseMetro.check() == 1) {
      if (currentShow == SHOW_DEFAULT) {
        switch (currentMode) {
          case MODE_CHASE1:             
          case MODE_CHASE2:              
          case MODE_CHASE3:           
                updateChase();
                break;   
        } //end switch
      } //end if
  } //end chaseMetro  

  if (playQueueMetro.check() == 1) { // check if the metro has passed its interval
    String fn = playQueue;
    if (fn.length() >0) {
      //decrease audio in level during playback
      fadeL.fadeOut(100);
      fadeR.fadeOut(100);
      
      playQueue = "";
      playWAV(fn); 
    }
  }

  if (bgmMetro.check() == 1) { // check if the metro has passed its interval
    //check to see if wav is playing, if not restart
    if (!playSdWav1.isPlaying()) { 
      fadeL.fadeIn(1000);
      fadeR.fadeIn(1000);
    }
  }

  if (colorChangeMetro.check() == 1) { // check if the metro has passed its interval
    actionChangeColor(false);
  }
  
  debugOptionsCheck();       
}


void updateModeShow() {

 switch (currentShow) {

  case SHOW_FLARE:
    //do stuff
    //Serial.println("SHOW:FLARE");
    
    //FastLED.setBrightness (FLARE_BRIGHTNESS);
    fill_solid(hlLEDS,HL_NUM_LEDS, FLARE_COLOR);
    fill_solid(eyeLEDS,EYE_NUM_LEDS, FLARE_COLOR);
    fill_solid(trayLEDS,TRAY_NUM_LEDS, FLARE_COLOR);
    fill_solid(blingLEDS,BLING_NUM_LEDS, FLARE_COLOR);
    
    
    analogWrite(HL_SPOT_LEFT_PIN, 33);
    analogWrite(HL_SPOT_RIGHT_PIN, 33);
    analogWrite(GFX_RED_PIN, 33);
    analogWrite(GFX_GREEN_PIN, 33);
    analogWrite(GFX_BLUE_PIN, 33);

    
    FastLED.show();
    //TODO: GET RID OF THIS DELAY
    //The metro for show is 100 currently, could set this and then clear on next pass...
    delay(50);
    analogWrite(HL_SPOT_LEFT_PIN, 0);
    analogWrite(HL_SPOT_RIGHT_PIN, 0);
    analogWrite(GFX_RED_PIN, 0);
    analogWrite(GFX_GREEN_PIN, 0);
    analogWrite(GFX_BLUE_PIN, 0);
    
    FastLED.clear();
    //FastLED.setBrightness (DEFAULT_BRIGHTNESS);
    FastLED.show();
    currentShow = SHOW_DEFAULT;
    break;
  case SHOW_SPEAKING:
    //do stuff
    //Serial.println("FLARE");
    break;
  case SHOW_HONKING:
    //do stuff
    break;
  case SHOW_DEFAULT:

    //ensure headlights are in proper status
    leftHeadlightBrightness(DEFAULT_HEADLIGHT_BRIGHTNESS * leftHeadlightStatus);
    rightHeadlightBrightness(DEFAULT_HEADLIGHT_BRIGHTNESS * rightHeadlightStatus);
    
    break;  
 }
   
   //check mode to see if we should update FFT
  //showFFT();

/*
 * Use to debug color vals
  for (int c = 0; c< NUM_MODE_COLORS; c++) {
  leds[c].setHue(modeColorHueVals[c]); 
  }
  */

  FastLED.show();
}


void OnPress(int key)
{
  if (debugOptions[DEBUG_INPUT]) {
    Serial.printf("key: %d\n", key);
  }
  actionFlare();
  mapAction(SOURCE_KEY, key, 0);
    
}

void OnNoteOn(byte channel, byte note, byte velocity)
{
   if (debugOptions[DEBUG_INPUT]) {
     Serial.printf("Note on, ch=%d, note=%d, velocity=%d\n",channel, note, velocity);
   }

  if (velocity < MIN_NOTE_VELOCITY) velocity = MIN_NOTE_VELOCITY; 

  //we will use channel 1 for notes, and channel 2 for actions
  if (channel == 1) {
    freeVoices();
    int wavetable_id = allocateVoice(channel, note);   
    wavetable[wavetable_id].playNote(note, velocity);
  }
  else {
    actionFlare();
    mapAction(SOURCE_MIDI_NOTE, note, velocity);
  }
  
}

void OnNoteOff(byte channel, byte note, byte velocity)
{
  if (debugOptions[DEBUG_INPUT]) {
     Serial.printf("Note off, ch=%d, note=%d, velocity=%d\n",channel, note, velocity);    
  }

  int wavetable_id = findVoice(channel, note);
  if (wavetable_id != TOTAL_VOICES)
    wavetable[wavetable_id].stop();
}

void OnControlChange(byte channel, byte control, byte value)
{
 if (debugOptions[DEBUG_INPUT]) {
     Serial.printf("Control Change, ch=%d, control=%d, value=%d\n",channel, control, value);    
  }

  if((channel == 1) && ( control == 1)) {
    int color = map( value, 0, 127, 0, NUM_MODE_COLORS - 1);
    if (debugOptions[DEBUG_INPUT]) {
      Serial.printf("color - %d\n",color);
    }
    currentModeColor = color;
  }

  if((channel == 1) && ( control == 2)) {
    int metrospeed = map( value, 0, 127, 3000, 50);
    if (value < 5) metrospeed = 1000000; //is there a way to stop a metro?
      if (debugOptions[DEBUG_INPUT]) {
        Serial.printf("color change speed map - %d\n",metrospeed);
      }
    colorChangeMetro.interval(metrospeed); 
    colorChangeMetro.reset();
  }

  if((channel == 1) && ( control == 3)) {
    int brt = map( value, 0, 127, 30, 255);
      if (debugOptions[DEBUG_INPUT]) {
        Serial.printf("brightness change map - %d\n",brt);
      }
    LEDS.setBrightness(brt);
  }
  
  
  if((channel == 1) && ( control == 5)) {
    int instrument = map( value, 0, 127, 0, NUM_INSTRUMENTS - 1);
    if (debugOptions[DEBUG_INPUT]) {
      Serial.printf("instrument map - %d\n",instrument);
    }
    if (instrument == curInstrument) return;
    curInstrument = instrument;
    int announce = true;
    //copied from actionChangeInstrument
    for (int i = 0; i < TOTAL_VOICES; ++i) {

      switch (instrument) {
        case 0: wavetable[i].setInstrument(standard_DRUMS); if (i==0 && announce) actionPlayWAV("DRUMS.WAV");   break; //only play audio first time through loop
        case 1: wavetable[i].setInstrument(piano);          if (i==0 && announce) actionPlayWAV("PIANO.WAV");   break;
        case 2: wavetable[i].setInstrument(distortiongt);   if (i==0 && announce) actionPlayWAV("GUITAR.WAV");  break;
        case 3: wavetable[i].setInstrument(trumpet);        if (i==0 && announce) actionPlayWAV("TRUMPET.WAV"); break;
        case 4: wavetable[i].setInstrument(timpani);        if (i==0 && announce) actionPlayWAV("TIMPANI.WAV"); break;
    
      }  

    wavetable[i].amplitude(1);
    voices[i].wavetable_id = i;
    voices[i].channel = voices[i].note = 0xFF;
    
  }
  
  }
  if((channel == 1) && ( control == 6)) {
    int note = map( value, 0, 127, 24, 96);
    
    freeVoices();
    int wavetable_id = allocateVoice(channel, note);   
    wavetable[wavetable_id].playNote(note, 100);
    delay(10);
    wavetable[wavetable_id].stop();
  }

  
}


/*
 * updateChase() is called by the chase timer
 * It then calculates the current position for each ring based on its number of LEDs
 * and calls the updateChaseLEDs function for each ring
 * and then increments the global position tracker
 * 
 */
 
void updateChase() {

  //to keep the cycles tracking on different size rings, we use a 360 degree cycle and then map that back to the ring LED
  
  int posHL =   map (currentChasePosition, 0, 359, 0, HL_NUM_LEDS-1  ); //headlights
  int posEYE =  map (currentChasePosition, 0, 359, EYE_NUM_LEDS-1, 0 ); //eyes
  int posTRAY = map (currentChasePosition, 0, 359, 0, TRAY_NUM_LEDS-1 ); //tray 
  int posBLING = map (currentChasePosition, 0, 359, 0, BLING_NUM_LEDS-1 ); //bling
  
  FastLED.clear();

  int updates = 0;
  
  if (posHL != lastChasePosHL) updates = 1; lastChasePosHL = posHL; updateChaseLEDs(hlLEDS, HL_NUM_LEDS, posHL);                 
  if (posEYE != lastChasePosEYE) updates = 1; lastChasePosEYE = posEYE; updateChaseLEDs(eyeLEDS, EYE_NUM_LEDS, posEYE);              
  if (posTRAY != lastChasePosTRAY) updates = 1; lastChasePosTRAY = posTRAY; updateChaseLEDs(trayLEDS, TRAY_NUM_LEDS, posTRAY);             
  if (posBLING != lastChasePosBLING) updates = 1; lastChasePosBLING = posBLING; updateChaseLEDs(blingLEDS, BLING_NUM_LEDS, posBLING);             
  
  
  if (updates) FastLED.show();

  //set GFX brightness based on chase position
  int brt = map (currentChasePosition, 0, 359, 0, 255 ); 
  gfxRGB(CHSV(modeColorHueVals[currentModeColor], 255, brt));

  
  int angleIncrease = 360/MAX_CYCLE_LEDS;       
  currentChasePosition += angleIncrease;   
  if (currentChasePosition >= 360) currentChasePosition = 0;
}

/*
 * updateChaseLEDs is called by updateChase()
 * It updates the chase animation for that ring
 * The function allows an offset for multiple rings on the same array
 * (Would sure have been simpler to give each ring its own pin and to just mirror with FASTLED!
 * 
 */
void updateChaseLEDs(CRGB leds[], int numleds, int pos) {
  
  if (pos > numleds-1) {
    pos = numleds; //safety
    Serial.println ("ERROR: updateChase() pos > numleds!");
  }
  
  if (currentMode == MODE_CHASE1) {
      int l = pos;
      if (debugOptions[DEBUG_ANIMATION]) {
        Serial.printf("Chase1(%d):%d)\n", pos, l);
        }
      leds[l] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
      
  }
  else if (currentMode == MODE_CHASE2) {  
    
      int l = pos;
      leds[l] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
      
      int l2 = ledRingIncrement(pos, numleds, numleds/2);      
      leds[l2] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
         
  }
  else if (currentMode == MODE_CHASE3) {
    
     //Can't use fill_solid here because fill_solid fills both rings 

     //fill with primary color
     for (int l=0; l < numleds; l++) {
      leds[l] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
     }

     for (int i=0; i< CHASE3_LENGTH; i++) {
       int l = ledRingIncrement(pos, numleds, i);
       leds[l]    = CRGB(0,0,0);
     }
  }
 
  FastLED.show();
}

/*
 * ledRingIncrement safetly increments an led position number
 * an will wrap it around the ring
 * Note: I have not tested negative increments...
 * 
 */
int  ledRingIncrement(int pos, int numleds, int incr) {
   int out = pos+incr; 
   
   if (out >= numleds) {
      out = out - numleds;
   }
   
   return out;
}


void updatePeak() {

  if (peakIn.available()) {
    
     float peak = peakIn.read();

     if (debugOptions[DEBUG_PEAK]) {
       if (peak > 0.01) {
         Serial.print(peak);
         int monoPeak = peak * 30;
         Serial.print("|");
         for (int cnt=0; cnt<monoPeak; cnt++) {
            Serial.print(">");
         }
         Serial.println();
       }
    }
     
  
    FastLED.clear();
    updatePeakLEDs(hlLEDS,    HL_NUM_LEDS,    peak); 
    updatePeakLEDs(eyeLEDS,   EYE_NUM_LEDS,   peak); 
    updatePeakLEDs(trayLEDS,  TRAY_NUM_LEDS,  peak); 
    updatePeakLEDs(blingLEDS,  BLING_NUM_LEDS,  peak); 
    
    FastLED.show();

    //update groundfx
    gfxRGB(CHSV(modeColorHueVals[currentModeColor], 255, peak*255));

    if (peak >.9) {
      leftHeadlightBrightness(255); 
      rightHeadlightBrightness(255);
    }
    else {
      if (!leftHeadlightStatus) leftHeadlightBrightness(0); 
      if (!rightHeadlightStatus) rightHeadlightBrightness(0);
    }
     
  }
   
  
}

void updatePeakLEDs(CRGB leds[], int numleds, float peak) { 
     
      int monoPeak = peak * numleds;
      
      switch (currentMode) {
        case MODE_PEAK_STRAIGHT:
                
            for (int l = 0; l< numleds; l++) {
              if (l < monoPeak) {
                leds[l] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
            } //end if below peak
          } //end for
        break;

       case MODE_PEAK_SPLIT:
            
          int bins = numleds/2; //these arent really bins, but reusing code from FFT
      
          for (int b = 0; b< bins; b++) {
            if (b < (monoPeak / 2)) {
                if (b==0) {                                       //center LED
                  leds[bins] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
                  }
                else {
                  leds[bins-b] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
                  leds[bins+b] = CHSV(modeColorHueVals[currentModeColor], 255, 255);
                }
            } //end if below peak
          } //end for
            break;
        
      }
    
}

void updateFFT() {
  
  if (fft1024_1.available()) {
    FastLED.clear();
    updateFFTLEDs(hlLEDS, HL_NUM_LEDS); 
    updateFFTLEDs(eyeLEDS, EYE_NUM_LEDS); 
    updateFFTLEDs(trayLEDS, TRAY_NUM_LEDS);
    updateFFTLEDs(blingLEDS, BLING_NUM_LEDS);
     
    FastLED.show();
  }  

  //update GFX from the peak since FFT doesnt make sense here
    if (peakIn.available()) {
     float peak = peakIn.read();
     //Serial.println(peak);
     gfxRGB(CHSV(modeColorHueVals[currentModeColor], 255, peak*255));

    if (peak >.9) {
      leftHeadlightBrightness(255); 
      rightHeadlightBrightness(255);
    }
    else {
      if (!leftHeadlightStatus) leftHeadlightBrightness(0); 
      if (!rightHeadlightStatus) rightHeadlightBrightness(0);
    }
    
    }
  
}

void updateFFTLEDs(CRGB leds[], int numleds) {
  //FastLED.clear(); //clearing here on every cycle leads to a lot of flickering.


    int bin_color = 0;
    int fft_brt = 0;
   
      
    if (currentMode == MODE_FFT_STRAIGHT) {
      for (int i=0; i<numleds; i++) {  
        float n = fft1024_1.read(i);
        bin_color = map(i, 0, numleds, 0, 255);
        fft_brt = n*10*255*20; 
        if (fft_brt < FFT_MIN) fft_brt = 0; //min val
        if (fft_brt > 255) fft_brt = 255; //max
        
        leds[i] = CHSV(modeColorHueVals[currentModeColor], 255, fft_brt);      
      } //end for - BY LED
    } //end if straight
    else if (currentMode == MODE_FFT_SPLIT) {
    //mirror
    int bins = numleds / 2;
    int bin_width = 400 / numleds;  //don't want the top bins with just high freq
    
    for (int b=0; b<bins; b++) { 
      //float n = fft1024_1.read(b);
      float n = 0;
      for (int sample=0; sample<bin_width; sample++) {
        n = n + fft1024_1.read((b*bin_width)+sample);
      }
      n = n / bin_width; //we want the average
      
      //todo: implement FFT style switching
      
     
      //rainbow
      //bin_color = map(b, 0, bins, 0, 200);
      bin_color = modeColorHueVals[currentModeColor];
      
      fft_brt = n*10*255*20; 
      if (fft_brt > 255) fft_brt = 255; //max val
      if (fft_brt < FFT_MIN) fft_brt = 0; //min val

      if (debugOptions[DEBUG_FFT]) {
        Serial.printf("%d->(%d,%d)..\n", b, bin_color, fft_brt);
      }
      
      if (b==0) {                                       //center LED
        leds[bins] = CHSV(bin_color, 255, fft_brt); 
      }
      else {
        leds[bins-b] = CHSV(bin_color, 255, fft_brt);  //behind center 
        leds[bins+b] = CHSV(bin_color, 255, fft_brt);  //ahead of canter
      }

      
    } //end for - BY BIN
 
    }//end if split
    
}


void gfxRGB( const CRGB& rgb)
{
  analogWrite(GFX_RED_PIN,   rgb.r );
  analogWrite(GFX_GREEN_PIN, rgb.g );
  analogWrite(GFX_BLUE_PIN,  rgb.b );
}

void leftHeadlightBrightness(int brt)
{
 analogWrite(HL_SPOT_LEFT_PIN, brt);
}

void rightHeadlightBrightness(int brt)
{
 analogWrite(HL_SPOT_RIGHT_PIN, brt);
}

/* 
 *  ACTION CODE
 *  
 */

void mapAction(int src, int key, int data) {
  for (int s = 0; s< ACTION_MAP_SIZE; s++) {
    if (ActionMap[s][0] == src && ActionMap[s][1] == key) {
      processAction(ActionMap[s][2], src, key, data);     
    } //end if
  } //end for
}

void processAction (int action, int src, int key, int data) {
  switch (action) {
    case ACTION_CHANGE_COLOR:       actionChangeColor(true); break;
    case ACTION_CHANGE_SPEED:       actionChangeSpeed(); break;
    case ACTION_CHANGE_MODE:        actionChangeMode(); break;
    case ACTION_CHANGE_INSTRUMENT:  curInstrument++; actionChangeInstrument(true); break;
    //case ACTION_PLAY_NOTE:          actionPlayNote(key, data); break;
    case ACTION_FLARE:              actionFlare(); break;
    case ACTION_HEADLIGHT_TOGGLE:   actionHeadlightToggle(); break;
    
    case ACTION_STOP:               actionStop(); break;
    case ACTION_PLAY_WAV:           actionPlayWAV(data); break;
    case ACTION_PLAY_WAV_RND:       actionVideoSelect(1); actionPlayRandomWAV(); break;
    case ACTION_PLAY_WAV_HORN:      actionVideoSelect(5); actionPlayHornWAV(); break;
    case ACTION_PLAY_WAV_TRANSFORM: actionVideoSelect(3); actionPlayWAV("TRANSFRM.WAV"); break;
    case ACTION_PLAY_WAV_OPTIMUS:   actionVideoSelect(2); actionPlayWAV("OPTIMUS.WAV"); break;
    case ACTION_PLAY_WAV_NAMEIS:   actionVideoSelect(2); actionPlayWAV("MYNAMEIS.WAV"); break;
    
  }
}


void actionChangeColor(bool announce) {
  
 currentModeColor++; 
 if (currentModeColor >= NUM_MODE_COLORS) currentModeColor = 0;

 if (debugOptions[DEBUG_ACTION] && announce) Serial.printf("actionChangeColor(%s)\n", colorText[currentModeColor]);

}

void actionChangeSpeed() {
 if (debugOptions[DEBUG_ACTION])  Serial.println("actionChangeSpeed"); 
 //update speed setting
}

void actionChangeMode() {
 
 currentMode++; 
 if (currentMode >= NUM_MODES) currentMode = 0;

 FastLED.clear();
 FastLED.show();

 if (debugOptions[DEBUG_ACTION]) Serial.printf("actionChangeMode(%s)\n", modeText[currentMode]);

}

void actionChangeInstrument(bool announce) {
 if (debugOptions[DEBUG_ACTION]) Serial.println("actionChangeInstrument"); 

 if (curInstrument > (NUM_INSTRUMENTS-1)) curInstrument=0;

  if (debugOptions[DEBUG_ACTION]) Serial.printf("setInstrument:%d\n", curInstrument);
  
  for (int i = 0; i < TOTAL_VOICES; ++i) {

    switch (curInstrument) {
      case 0: wavetable[i].setInstrument(standard_DRUMS); if (i==0 && announce) actionPlayWAV("DRUMS.WAV");   break; //only play audio first time through loop
      case 1: wavetable[i].setInstrument(piano);          if (i==0 && announce) actionPlayWAV("PIANO.WAV");   break;
      case 2: wavetable[i].setInstrument(distortiongt);   if (i==0 && announce) actionPlayWAV("GUITAR.WAV");  break;
      case 3: wavetable[i].setInstrument(trumpet);        if (i==0 && announce) actionPlayWAV("TRUMPET.WAV"); break;
      case 4: wavetable[i].setInstrument(timpani);        if (i==0 && announce) actionPlayWAV("TIMPANI.WAV"); break;
  
    }  

    wavetable[i].amplitude(1);
    voices[i].wavetable_id = i;
    voices[i].channel = voices[i].note = 0xFF;
  }
}


void actionPlayRandomWAV() {
 if (debugOptions[DEBUG_ACTION]) Serial.println("actionPlayRandomWAV()");
 playRandomVoiceFile();
}


//NOT IMPLEMENTED
void actionPlayWAV(int filenum) {
 if (debugOptions[DEBUG_ACTION]) Serial.printf("actionPlayWAV()- %d\n", filenum);

 //build filename
 //play wav file
}


void actionPlayWAV (char* filename) {
 //note: this function does not have the rate protection code to prevent button spamming locks
 if (debugOptions[DEBUG_ACTION]) Serial.printf("actionPlayWAV (char *)  - %s\n", filename);

 queueWAV(filename);
}


//needed for string constants in the processAction function
void actionPlayWAV (char const* filename) {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("actionPlayWAV (char const*)  - %s\n", (char*) filename);
  
  actionPlayWAV((char*) filename); //cast to char* to call the other function
}


void actionPlayHornWAV() {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("actionPlayHornWAV()\n");
  
  int i = random(1, NUM_HORN_WAVS + 1); //random returns between min and (max-1);
    //generate filename
  String fn = "HORN";
  fn = fn + i + ".WAV";
  
  queueWAV(fn);
}

void actionVideoSelect (int video) {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("actionVideoSelect(%d)\n", video);
  
  //send midi song select here
  //midi1.sendSongSelect(video);
  //midi2.sendSongSelect(video);
}
void actionFlare() {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("actionFlare()\n");
  currentShow = SHOW_FLARE;
  //should i just move the blocking version of the code here?
}

void actionHeadlightToggle() {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("actionHeadlightToggle()\n");
    
    leftHeadlightStatus = !leftHeadlightStatus;
    rightHeadlightStatus = !rightHeadlightStatus;
    
}

void actionStop() {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("actionStop()\n");
  currentMode = MODE_OFF;
  
  //set mode to all LEDs off?  
}


/*
 * Audio Playback
 * playWAV - this plays a specific wav file
 * there is a "debounce" on this to prevent locking behavior
 * 
 */

 void playWAV (String fn) {

  if (debugOptions[DEBUG_AUDIO]) Serial.printf("playWAV(%s)\n", fn.c_str());
  
  playSdWav1.play(fn.c_str());
  
} //end playWAV

/*
 * Audio: queueWAV(filename)
 *        puts a filename into the queue to be picked up
 *        by the queue metro and played
 *        this avoids locking behavior
 *        
 */

void queueWAV(String fn) {
  playQueue = fn;
  if (debugOptions[DEBUG_AUDIO]) Serial.printf("queueWAV(%s)\n", fn);
}

/*
 * Audio Playback
 * playRandomVoiceFile - this plays a random wav file from the voice files
 * there is NO "debounce" on this to prevent locking behavior
 */
int playRandomVoiceFile() {
    
    //pick random number
    int i = random (1, NUM_VOICE_FILES + 1); //random returns up to (max-1)

    //generate filename
    String fn = "/bb-wavs/bb";
    fn = fn + i + ".wav";

    if (debugOptions[DEBUG_AUDIO]) Serial.printf("playRandomVoiceFile() - %s\n", fn.c_str());
    queueWAV(fn);
    
    return i;
}


/*
 * Audio Playback
 * playRandomSoundFile - this plays a random wav file from the voice files
 * there is NO "debounce" on this to prevent locking behavior
 */
int playRandomSoundFile() {
    
    //pick random number
    int i = random (1, NUM_SOUND_FILES);

    //generate filename
    String fn = "/bb-wavs/bbs";
    fn = fn + i + ".wav";

    if (debugOptions[DEBUG_AUDIO]) Serial.printf("playRandomSoundFile() - %s\n", fn.c_str());
    queueWAV(fn);
    return i;
}


/*
 * allocateVoice - from the MidiSynthKeyboard example
 * 
 */

int allocateVoice(byte channel, byte note) {
  int i;
  int nonfree_voices = stopped_voices + used_voices;
  if (nonfree_voices < TOTAL_VOICES) {
    for (i = nonfree_voices; i < TOTAL_VOICES && voices[i].channel != channel; ++i);
    if (i < TOTAL_VOICES) {
      voice_t temp = voices[i];
      voices[i] = voices[nonfree_voices];
      voices[nonfree_voices] = temp;
    }
    i = nonfree_voices;
    used_voices++;
  }
  else {
    if (stopped_voices) {
      i = evict_voice % stopped_voices;
      voice_t temp = voices[i];
      stopped_voices--;
      voices[i] = voices[stopped_voices];
      voices[stopped_voices] = temp;
      used_voices++;
      i = stopped_voices;
    }
    else
      i = evict_voice;
  }

  voices[i].channel = channel;
  voices[i].note = note;

  evict_voice++;
  evict_voice %= TOTAL_VOICES;

  return voices[i].wavetable_id;
}


/*
 * findVoice - from the MidiSynthKeyboard example
 * 
 */


int findVoice(byte channel, byte note) {
  int i;
  //find match
  int nonfree_voices = stopped_voices + used_voices;
  for (i = stopped_voices; i < nonfree_voices && !(voices[i].channel == channel && voices[i].note == note); ++i);
  //return TOTAL_VOICES if no match
  if (i == (nonfree_voices)) return TOTAL_VOICES;

  voice_t temp = voices[i];
  voices[i] = voices[stopped_voices];
  voices[stopped_voices] = temp;
  --used_voices;

  return voices[stopped_voices++].wavetable_id;
}


/*
 * freeVoices - from the MidiSynthKeyboard example
 * 
 */

void freeVoices() {
  for (int i = 0; i < stopped_voices; i++)
    if (wavetable[voices[i].wavetable_id].isPlaying() == false) {
      voice_t temp = voices[i];
      --stopped_voices;
      voices[i] = voices[stopped_voices];
      int nonfree_voices = stopped_voices + used_voices;
      voices[stopped_voices] = voices[nonfree_voices];
      voices[nonfree_voices] = temp;
    }
}


/*
 * debugOptionsCheck() - this function checks the Serial input and
 *                       changes debug options as well as emulates some 
 *                       button presses
 *                       
 */
void debugOptionsCheck() {
  int incomingByte = 0;
  
  if (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      int option;
      
      switch (incomingByte) {
          case '0':                         //turn all options off
            for (int o = 1; o<10; o++) {    //we dont use zero
              debugOptions[o] = 0;
            }
            Serial.println ("All debug options turned OFF");
            break;
            
          case '1' ... '9': 
            option = incomingByte - '0';
            debugOptions[option] = !debugOptions[option]; 
            //Serial.printf("Debug option %d is now %s\n", option, debugOptions[option]?"ON":"OFF");
            break;

          //remove, comment or alter these cases if there are no hardware buttons
          //case 'q': mapAction(SOURCE_BUTTON, 1, 0); break;
          //case 'w': mapAction(SOURCE_BUTTON, 2, 0); break;
          //case 'e': mapAction(SOURCE_BUTTON, 3, 0); break;
          //case 'r': mapAction(SOURCE_BUTTON, 4, 0); break;
          }
         
         printDebugOptions();
          
      }
       
}

/*
 * printDebugOptions() - this function outputs a debug option list with
 *                       with current status, and provides some input 
 *                       instructions
 *  
 */
void printDebugOptions() {
  Serial.println("\nDebug Options Status");
  Serial.println("Use serial input keys 1 through 9 to change debug options");
  
  //remove, comment or alter this line if there are no hardware buttons
  Serial.println("Use serial input keys QWER to emulate buttons 1 through 4");
  
  for (int o=1; o<10; o++) {    //we don't use zero
    {
      if (debugOptionsText[o]) //don't print undefined options
        Serial.printf("   Option %d = %s %s\n", o, debugOptions[o]?"ON: ":"OFF:", debugOptionsText[o]);
    }
  }
  Serial.println("\n");       //a little extra padding in the output
}
 
 
