#include <math.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

extern uint32_t hsv2rgb(uint16_t h, uint8_t s, uint8_t v);

#define PIXEL_COUNT 50
#define PIN 4

#define ANIMATION_SLOTS 32

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


struct hsvcolor
{
  uint16_t  h;
  uint8_t   s;
  uint8_t   v;
};

hsvcolor lustrePalette[] = {
  {1109, 153, 237},
  {1126, 249, 183},
  {1143, 168, 160},
  {1160, 216, 201},
  {1181, 221, 119},
  {1181, 239, 132},
  {119, 175, 219},
  {1194, 183, 145},
  {1245, 178, 252},
  {1245, 198, 219},
  {1250, 193, 221},
  {1275, 244, 214},
  {1309, 237, 193},
  {132, 249, 188},
  {1331, 204, 234},
  {1352, 244, 249},
  {136, 214, 242},
  {1527, 193, 211},
  {153, 247, 234},
  {1531, 140, 237},
  {162, 175, 247},
  {187, 226, 237},
  {34, 226, 186},
  {93, 209, 188}
};


hsvcolor primaryPalette[] = {
  {0, 255, 0},
  {64, 255, 0},
  {128, 255, 0},
  {192, 255, 0},
  {256, 255, 0},
  {320, 255, 0},
  {384, 255, 0},
  {448, 255, 0},
  {512, 255, 0},
  {576, 255, 0},
  {640, 255, 0},
  {704, 255, 0},
  {768, 255, 0},
  {832, 255, 0},
  {896, 255, 0},
  {960, 255, 0},
  {1024, 255, 0},
  {1088, 255, 0},
  {1152, 255, 0},
  {1216, 255, 0},
  {1280, 255, 0},
  {1344, 255, 0},
  {1408, 255, 0},
  {1472, 255, 0}
};


// bitset to track which lights are actively animating
uint64_t busyPixels = 0;

int chooseRandomPixel() {
  int n;
  while (1) {
    n = random(0, PIXEL_COUNT);
    if (bitRead(busyPixels, n) == 0) {
      bitSet(busyPixels, n);
      return n;
    }
  }
}


class Animation
{
  public:
  
  int                 id;

  unsigned long   startTime;
  boolean         active;
  boolean         sync; // to synchronize start on v=0

  int             colorIndex;
  int             pixelIndex;

  void init(int id, unsigned long startTime) {
    this->id = id;
    this->startTime = startTime;
    this->active = false;
    this->sync = false;

    this->colorIndex = random(0,24); // index into global palette array
    this->pixelIndex = chooseRandomPixel();
    //bitSet(busyPixels, this->pixelIndex);

    Serial.print("* anim:");
    Serial.print(this->pixelIndex);
    Serial.print(" startTime:");
    Serial.print(this->startTime);
    Serial.print(" color:");
    Serial.print(this->colorIndex);
    Serial.println();
  }

  void reset() {
    this->active = false;
    bitClear(busyPixels, this->pixelIndex);

    Serial.print("* resetting:");
    Serial.println(this->id);
  }

  void tick(unsigned long ms) {
    if (!this->active) return;    

    int r = ms - this->startTime; // number of ms we've been running
    uint8_t v = breathe_intensity(ms - this->startTime);

    // v may be > 0 for our initial frame, which looks bad. so if we
    // haven't synced our start to v=0, just wait
    if (!this->sync) {
      if (v > 0) {
        return;
      } else {
        this->sync = true;
      }
    }

    // XXX make the palette customizable
    hsvcolor color = primaryPalette[this->colorIndex];
    uint32_t currentColor = hsv2rgb(color.h, color.s, v);
    strip.setPixelColor(this->pixelIndex, currentColor);

    if (r > 4000 && v == 0) { // XXX randomize this runtime
      this->reset();

      unsigned long startTime = ms + (1000 * random(0,4));
      this->init(this->id, startTime);
    }
  }  
};

Animation anim[ANIMATION_SLOTS];

void setup() {
  Serial.begin(115200);
  
  // setup neopixels
  strip.begin();
  strip.setBrightness(80);
  strip.show(); // Initialize all pixels to 'off'

  
  unsigned long now = millis();
  for (int i=0; i<ANIMATION_SLOTS; i++) {
    unsigned long startTime = now + (500 * i);
    anim[i].init(i, startTime);
  }
}

void loop() {

  unsigned long ms = millis();
  
  for (int i=0; i<ANIMATION_SLOTS; i++) {
    if (!anim[i].active && ms >= anim[i].startTime) {
      Serial.print("starting animation:");
      Serial.println(i);
      anim[i].active = true;
    }
    
    anim[i].tick(ms);
  }

  strip.show();
  delay(10);
}
