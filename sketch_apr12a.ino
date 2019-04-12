#include <bluefruit.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIXEL_COUNT 50
#define BLUETOOTH_DEVICE_NAME "Blinkywinky"

BLEUart bleuart;

// Function prototypes for packetparser.cpp
uint8_t readCompletePacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);

uint8_t checkPacket(BLEUart *ble_uart, uint16_t timeout);
void resetPacket();
boolean isPacketComplete();
boolean verifyPacket();
void dumpPacket();


// Packet buffer
extern uint8_t packetbuffer[];

#define PIN 4

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

extern uint32_t hsv2rgb(uint16_t h, uint8_t s, uint8_t v);


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

hsvcolor springPalette[] = {
  {1122, 165, 216},
  {217, 204, 221},
  {264, 239, 211},
  {721, 175, 249},
  {200, 140, 244},
  {456, 198, 255},
  {1262, 237, 209},
  {221, 140, 249},
  {251, 186, 252},
  {132, 183, 244},
  {482, 232, 153},
  {8, 252, 175},
  {298, 214, 196},
  {520, 173, 216},
  {243, 153, 255},
  {375, 201, 196},
  {1181, 216, 140},
  {401, 214, 252},
  {1126, 239, 165},
  {1100, 219, 137},
  {1467, 147, 204},
  {1109, 188, 211},
  {307, 242, 244},
  {256, 181, 226}
};


class Animation
{
  public:
  
  int             id;

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
    this->pixelIndex = id;

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
    hsvcolor color = springPalette[this->colorIndex];
    uint32_t currentColor = hsv2rgb(color.h, color.s, v);
    strip.setPixelColor(this->pixelIndex, currentColor);

    if (r > 4000 && v == 0) { // XXX randomize this runtime
      this->reset();

      unsigned long startTime = ms + (1000 * random(0,4));
      this->init(this->id, startTime);
    }
  }  
};


// new regime! let's try one animation per pixel with no
// reallocation.. just delays
Animation anim[PIXEL_COUNT];
int globalBrightness = 80;


void setup() {
  // setup from bluefruit controller sketch
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println(F("Adafruit Bluefruit52 Controller App Example"));
  Serial.println(F("-------------------------------------------"));

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName(BLUETOOTH_DEVICE_NAME);

  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();  

  // setup neopixels
  strip.begin();
  strip.setBrightness(globalBrightness);
  strip.show(); // Initialize all pixels to 'off'

  // init animations
  unsigned long now = millis();
  for (int i=0; i<PIXEL_COUNT; i++) {
    unsigned long startTime = now + (500 * i);
    anim[i].init(i, startTime);
  }
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// only called if we have a valid command packet
void parseCommand() {

  /* no color input for now - need code convert RGB->HSV
  // Color
  if (packetbuffer[1] == 'C') {
    uint8_t red = packetbuffer[2];
    uint8_t green = packetbuffer[3];
    uint8_t blue = packetbuffer[4];
    
    Serial.print ("RGB #");
    if (red < 0x10) Serial.print("0");
    Serial.print(red, HEX);
    if (green < 0x10) Serial.print("0");
    Serial.print(green, HEX);
    if (blue < 0x10) Serial.print("0");
    Serial.println(blue, HEX);

    // set current global color
    currentColor = strip.Color(red, green, blue);
  }
  */

  // Buttons
  if (packetbuffer[1] == 'B') {
    uint8_t buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';

    // only act on release
    if (pressed) {
      return;
    }

    switch (buttnum) {
      case 1:
        Serial.println("choose palette 1");
        break;
      case 2:
        Serial.println("choose palette 2");
        break;
      case 3:
        Serial.println("choose palette 3");
        break;
      case 4:
        Serial.println("choose palette 4");
        break;
      case 5:
        Serial.println("up arrow");
        if (globalBrightness < 100) {
          globalBrightness += 10;
          strip.setBrightness(globalBrightness);
          Serial.print("setting brightness:");
          Serial.println(globalBrightness);
        }
        break;
      case 6:
        Serial.println("down arrow");
        if (globalBrightness >= 10) {
          globalBrightness -= 10;
          strip.setBrightness(globalBrightness);
          Serial.print("setting brightness:");
          Serial.println(globalBrightness);
        }
        break;
      case 7:
        Serial.println("left arrow");
        break;
      case 8:
        Serial.println("right arrow");
        break;
    }
  }
}

void loop() {

  // poll bluetooth for incoming commands (shouldn't block)
  if (checkPacket(&bleuart, 5)) {
    if (isPacketComplete()) {
      if (verifyPacket()) {
        parseCommand();
      } else {
        Serial.print("Checksum mismatch in packet : ");
        dumpPacket();
      }
      resetPacket();
    } else {
        Serial.print("incomplete packet, waiting to process!");
    }
  }

  // update next animation frames
  unsigned long ms = millis();
  
  for (int i=0; i<PIXEL_COUNT; i++) {
    if (!anim[i].active && ms >= anim[i].startTime) {
      Serial.print("starting animation:");
      Serial.println(i);
      anim[i].active = true;
    }
    
    anim[i].tick(ms);
  }
  
  // push updates to the strip and sleep  
  strip.show();
  delay(10);
}
