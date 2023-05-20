// use: https://github.com/dgduncan/SevenSegment
#include <SegmentDisplay.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Encoder.h>
#define MAX_NUM_SOUND_BANKS 2
#define MAX_NUM_SOUND_FILES 16
#define BUTTON_UP 0
#define BUTTON_DOWN 1

// Audio Declarations
AudioControlSGTL5000     sgtl5000_1;     //xy=1125,313

// GUItool: begin automatically generated code
AudioPlaySdWav           wavPlayer;     //xy=217,501
AudioOutputI2S           i2s1;           //xy=435,502
AudioConnection          patchCord1(wavPlayer, 0, i2s1, 0);
AudioConnection          patchCord2(wavPlayer, 1, i2s1, 1);
// GUItool: end automatically generated code

//Display LEDs Declaration
// SegmentDisplay segmentDisplay(30, 32, 33, 28, 31, 26, 29, 9);
//                             E   D   C  DP  B   A   G   F
//                             1   2   4  5   6   7   9  10
SegmentDisplay segmentDisplay(31, 28, 33, 9, 32, 30, 26, 29);

//Teensy audio initialization Code
//SD CARD PINS
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  11
#define SDCARD_SCK_PIN   13

int maxSdReadAttempts = 10;
int numSdReadAttempts = 0;
File root;
enum SoundBank{
SOUND_BANK_A,
SOUND_BANK_B,
};
String sounds[MAX_NUM_SOUND_BANKS][MAX_NUM_SOUND_FILES];
SoundBank soundBank = SOUND_BANK_A;
int soundFile = 0; // display value
String soundFilename;


// select the input pins for the potentiometers
int potPin1 = A0;
int potPin2 = A1;
int potPin3 = A2;
int potPin4 = A3;
Encoder enc(0,1);

// pushbutton connected to digital pinS
int btnPin1 = 2;
int btnPin2 = 5; 
int btnPin3 = 25; 
int btnPin4 = 27; 
int gatePin1 = 3; 
int gatePin2 =6; 
int encBtnPin =4;

 // Variables to store port values 
float pot1 = 0; 
float pot2 = 0;
float pot3 = 0;
float pot4 = 0;

int btn1 = BUTTON_UP;
int btn2 = BUTTON_UP;
int btn3 = BUTTON_UP;
int btn4 = BUTTON_UP;

int gate1 = 0;
int gate2 = 0;

long oldEncPos = -999;
long newEncPos = -999;
bool encFlag = 0; // is display decimal flag displayed
int encPressed = 0; // is encoder button pressed
char displayValue = 0;

void setup() {  
  // open the serial port at 9600 bps:
  Serial.begin(9600); 
  Serial.println("Booting Sampler");
  
  audioSetup();
  sdCardSetup();

  loadSoundFiles();
  wavPlayer.play("SDTEST1.WAV");
  Serial.println("Playing sound file...");

  pinSetup();

  Serial.print("SOUND_BANK:"); Serial.println(soundBank);
}


void loop() {
  pot1 = max(analogRead(potPin1), 20);
  pot2 = analogRead(potPin2);
  pot3 = max(analogRead(potPin3), 1);
  pot4 = analogRead(potPin4);
  btn1 = !digitalRead(btnPin1);
  btn2 = !digitalRead(btnPin2);
  btn3 = !digitalRead(btnPin3);
  btn4 = !digitalRead(btnPin4);
  encPressed = !digitalRead(encBtnPin);
  newEncPos = enc.read() / 4;

  if (encPressed) { 
    encFlag = !encFlag;
    soundBank = int(encFlag);
    // soundFile = displayValue;

    segmentDisplay.displayHex(displayValue, encFlag);
    soundFilename = sounds[soundBank][soundFile];
    // wavPlayer.play(soundFilename);
    Serial.println(soundFilename);
  }
  
  if (newEncPos != oldEncPos) {
    int hex = abs(newEncPos) % 16;
    if(newEncPos < 0) {
      hex = 16 - hex;
    }

    if( sounds[soundBank][hex].length() <= 0 ) return;

    displayValue = hex;
    segmentDisplay.displayHex(displayValue, encFlag);

    soundFilename = sounds[soundBank][hex];
    changeSoundFile(soundFilename);

    Serial.print(hex);
    Serial.print(" - ");
    Serial.print(soundBank);
    Serial.print(":");
    Serial.print(soundFile);
    // Serial.print(displayValue);
    // Serial.print(soundFilename);
    Serial.println("");
    oldEncPos = newEncPos;
  }

  if (btn1 == BUTTON_DOWN) {
    // Serial.println("btn 1 DOWN");
    loadSoundFiles();
  }

  // Serial.println(wavPlayer.positionMillis());
  // Serial.println(btn1);
}

void audioSetup() {
  Serial.println("Setting up audio...");
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(1);
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.micGain(36); //NEEDED?

  Serial.println("Audio ready");
}

void sdCardSetup() {
  Serial.println("Setting up SD card...");
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (numSdReadAttempts < maxSdReadAttempts) {
      Serial.println("Unable to access the SD card");
      numSdReadAttempts++;
      delay(500);
    }
    Serial.println("ERROR: Failed to read SD card.");
  }
  numSdReadAttempts = 0;
}

void pinSetup() {
  // sets the digital pins as inputs and set pullups
  Serial.println("Setting digital pins...");
  pinMode(btnPin1, INPUT_PULLUP);
  pinMode(btnPin2, INPUT_PULLUP);
  pinMode(btnPin3, INPUT_PULLUP);
  pinMode(btnPin4, INPUT_PULLUP);
  pinMode(gatePin1, INPUT_PULLUP);
  pinMode(gatePin2, INPUT_PULLUP);
  pinMode(encBtnPin, INPUT_PULLUP);
  Serial.println("done");

  // set Display LEDs ports as Outputs
  Serial.println("Setting display...");
  pinMode(30, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(28, OUTPUT);
  pinMode(31, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(29, OUTPUT);
  pinMode(9, OUTPUT);
  Serial.println("done");
}

void printDirectory(File dir, int numTabs) {
  while( true ) {
    File entry = dir.openNextFile();
    
    if (!entry) { break; }

    for(uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    
    Serial.print(entry.name());
    
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    
    entry.close();
  }
}

void loadSoundFiles() {
    Serial.println("Loading sound files...");
    Serial.println("Reading root...");
    root = SD.open("/");
    // printDirectory( root, 0 );

    int soundIndex = 0;

    while(true) {
      File entry = root.openNextFile();
      if(!entry) { break; }

      sounds[soundBank][soundIndex] = entry.name();
      Serial.print(entry.name());
      Serial.print(" : ");
      Serial.println(sounds[soundBank][soundIndex]);
      soundIndex++;
      entry.close();
    }
    root.close();
    Serial.println("Loading sound files done.");
}

void changeSoundFile( String filename ) {
  Serial.print("change sound"); Serial.println(filename);
  int length = filename.length() + 1;
  char buf [length];
  filename.toCharArray(buf, length);
  wavPlayer.stop();
  wavPlayer.play(buf);
}
