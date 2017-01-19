#include <avr/power.h>
#include <EEPROM.h>
#include "FastLED.h"

FASTLED_USING_NAMESPACE

//For Nick's Test Setup
#define NUM_LEDS 72       //Per Wing

//For John's Hundo
//#define NUM_LEDS 125      //Per Wing

//Where are the LED Strps?
#define LED_PIN1 11
#define LED_PIN2 12

//FastLED Config
CRGB leds[NUM_LEDS];
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
bool gReverseDirection = false;

// BUTTON
const int buttonPin = 2;    // the number of the pushbutton pin

// STATE MACHINE
int modeLED = 0;
int brightnessLED = 0;
bool modeChange = false;
bool inStartup = true;
bool modeBrightness = false;

// EEPROM Definitions
#define EEPROM_MODE_ADDRESS 100
#define EEPROM_BRIGHTNESS_ADDRESS 200

void setup() {

  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH); // connect internal pull-up

  attachInterrupt(0, ISR0, FALLING);

  Serial.begin(9600);
  
  modeLED = EEPROM.read(EEPROM_MODE_ADDRESS);

  brightnessLED = EEPROM.read(EEPROM_BRIGHTNESS_ADDRESS);
  if ( brightnessLED > 255 || brightnessLED < 8 ) {
    brightnessLED = 32;
  }
  Serial.print ("modeLED: ");
  Serial.println (modeLED);

  FastLED.addLeds<NEOPIXEL, LED_PIN1>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<NEOPIXEL, LED_PIN2>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightnessLED);

  colorWipe(CRGB::Purple, 20); // Violet
  Serial.println ("Setup Over");

  inStartup = false;
}

void loop() {

  if (modeBrightness) {
    setupBrightness();
    return;
  }
  
  switch (modeLED) {
    case 0:
      colorWash(CRGB::Black);
      break;
    case 1:
      modeLEDrainbow();
      break;
    case 2:
      modeConfetti();
      break;
    case 3:
      modeLEDFire2012();
      break;
    case 4:
      CylonBouncer();
      break;
    case 5:
      colorWash(CRGB::Red);
      break;
    case 6:
      colorWash(CRGB::Green);
      break;
    case 7:
      colorWash(CRGB::Blue);
      break;
    case 8:
      colorWash(CRGB::Purple);
      break;
    case 9:
      colorWash(CRGB::White);
      break;
    case 10:
      modeLEDcop();
      break;
    default: //if the mode rolls past 10, default should catch it. Assume we've reached the end and cycle back to the beginning.
      modeLED = 0;
      Serial.println("modeLED rolling over to 0");
      break;
  }

  //Reset modeChange flag, used to make sure we don't get stuck running a mode's loop, even though the button has been pressed
  modeChange = false;
}


void setupBrightness() {
    //1. Display current brightness on scale 1-10 using inside pixels)
    //2. Increment brightness by 1 on scale when a short press is detected
    //3. Roll over to 1 if we go past 10 pixels
    //4. On long press, save current brightness value to EEPROM and exit mode

   int led_steps = (brightnessLED / 25);
    for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
    } 
   for (int i = 0; i < led_steps; i++) {
      leds[i] = CRGB::White; 
    }
    FastLED.setBrightness(brightnessLED);
    FastLED.show();         
}

void ISR0() {
  //Serial.println("Detected Button Press / Entered ISR");
  long count = 0;
 
  if (inStartup) {
    //Serial.println("Button Press Detected on Startup. Brightness Setting Mode Entered");
    modeBrightness = true;
    bool inStartup = false;
    return;
 }
  
  while (digitalRead(buttonPin) == LOW)
  {
    delay(10);
    count++;

    if (count > 2000)
    {
      Serial.println("Long Press Detected. ");
      saveMode();
      break;
    }
  }

  if (count > 100 && count < 1500)
  {
    Serial.println("Short Press Detected.");
    
    if (modeBrightness) {
      brightnessLED = brightnessLED + 25;
     if (brightnessLED > 255) {
        brightnessLED = 25;
      }
      Serial.print("brightnessLED change to: ");
      Serial.println(brightnessLED);
   }
    else {
      modeLED++;                      // increment the mode
      Serial.print("modeLED change to: ");
      Serial.println(modeLED);
      modeChange = true; 
    }
  }
}

void saveMode() {
  colorWipe(CRGB::White, 10);
  
  if (modeBrightness) {
    EEPROM.write(EEPROM_BRIGHTNESS_ADDRESS, brightnessLED);
    Serial.println ("EEPROM brightnessLED updated to: ");
    Serial.print (brightnessLED);
    Serial.println();
    FastLED.setBrightness(brightnessLED);
    modeBrightness = false;
  }
  else {
    EEPROM.write(EEPROM_MODE_ADDRESS, modeLED);
    Serial.println ("EEPROM modeLED updated to: ");
    Serial.print (modeLED);
    Serial.println(); 
    }

}

void modeLEDrainbow() {
  
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(10);
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the 
}

void modeConfetti() {
 // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(10);
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the 
}

#define COOLING  55
#define SPARKING 120

void modeLEDFire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(10);
}

void CylonBouncer(){
  int EyeSize = 8;
  int SpeedDelay = 5;
  int ReturnDelay = 50;
  int red = 255;
  int green = 0;
  int blue = 0;
  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    if (modeChange) break;
    colorWash(CRGB::Black);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue);
      
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    FastLED.show();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);

  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    if (modeChange) break; 
    colorWash(CRGB::Black);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue);
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    FastLED.show();
    delay(SpeedDelay);
  }
  
  delay(ReturnDelay); 
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
   // FastLED
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;

}

void modeLEDcop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red; 
  }
  FastLED.show();
  delay(250);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;   
  }
  FastLED.show();
  delay(250);
}

void colorWash(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

// Fill the dots one after the other with a color (from middle to outside)
void colorWipe(uint32_t color, uint8_t wait) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color; 
    FastLED.show();
    delay(wait);
  }
}

