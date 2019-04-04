#include <math.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

extern uint32_t hsv2rgb(uint16_t h, uint8_t s, uint8_t v);

#define PIXEL_COUNT 50
#define PIN 2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIN, NEO_RGB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

//
// show vars
//
uint32_t currentColor = strip.Color(0,0,255);
uint8_t  currentIndex = 0;
boolean  currentState = true;

void setup() {
  Serial.begin(115200);
  
  // setup neopixels
  strip.begin();
  strip.setBrightness(80);
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {

  // loop around the color wheel, holding S & V constant
  uint16_t h = 1280;
  uint8_t v = 255; // start at full brightness
  
  while (true) {
    // XXX need to brush up on my c idioms!
//    h += 8;
//    if (h >= 1536) {
//      h = 0;
//    }

    v = breathe_intensity(millis());

    currentColor = hsv2rgb(h, 255, v);
    Serial.println(v);

    for (int j=0; j<PIXEL_COUNT; j++) {
      strip.setPixelColor(j, currentColor);      
    }
    strip.show();
    
    delay(10);
  }  
}

// simple breathe animation
// http://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/

uint8_t breathe_intensity(unsigned long millis) {
  float val = (exp(sin(millis/2000.0*PI)) - 0.36787944)*108.0;
  return int(val);
}
