#include <math.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

extern uint32_t hsv2rgb(uint16_t h, uint8_t s, uint8_t v);

#define PIXEL_COUNT 50
#define PIN 2

#define ANIMATION_SLOTS 4

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


// simple breathe animation
// http://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/

uint8_t breathe_intensity(unsigned long millis) {
  float val = (exp(sin(millis/2000.0*PI)) - 0.36787944)*108.0;
  return int(val);
}

class Animation
{
  int                 id;
  Adafruit_NeoPixel *strip;

  // reassigned on each new animation start
  unsigned long   startTime;
  boolean         active;

  uint16_t        hue;
  int             pixelIndex;
  
  public:
  // XXX only necessary because we're using default ctor for static array simplicity
  void init(int id, Adafruit_NeoPixel *s) {
    this->id = id;
    this->strip = s;    
  }

  void debug() {
    Serial.print("* anim:");
    Serial.print(this->id);
    Serial.print(" hue:");
    Serial.print(this->hue);
    Serial.println();
  }

  void start(unsigned long ms, uint16_t hue, int pixelIndex) {
    if (this->active) {
      Serial.println("trying to start an already active show");
      return;
    }

    this->active = true;
    this->startTime = ms;
    this->hue = hue;
    this->pixelIndex = pixelIndex;
  }

  void tick(unsigned long ms) {
    if (!this->active) return;
    
    uint8_t v = breathe_intensity(ms - this->startTime);
    uint32_t currentColor = hsv2rgb(this->hue, 255, v);
   
    this->strip->setPixelColor(this->pixelIndex, currentColor);
  }  
};

Animation anim[ANIMATION_SLOTS];
uint16_t hues[] = {0,512,1024,1280};
int tick = 0;
int started = 0;


void setup() {
  Serial.begin(115200);
  
  // setup neopixels
  strip.begin();
  strip.setBrightness(80);
  strip.show(); // Initialize all pixels to 'off'
  
  for (int i=0; i<ANIMATION_SLOTS; i++) {
    anim[i].init(i, &strip);
  }
}

void loop() {

  unsigned long ms = millis();

  if (tick++ % 500 == 0) {
    if (started < ANIMATION_SLOTS) {
      Serial.print("starting animation:");
      Serial.println(started);

      anim[started].start(ms, hues[started], started);
      started++;
    }
  }
  
  for (int i=0; i<ANIMATION_SLOTS; i++) {
    anim[i].tick(ms);
  }

  strip.show();
  delay(10);
}
