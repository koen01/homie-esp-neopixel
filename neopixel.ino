#include <Homie.h>
#include <Adafruit_NeoPixel.h>

#define FW_NAME       "wemos-rgb-strip"
#define FW_VERSION    "0.0.4"
#define NUM_LEDS 60 


/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

// RGB Led pin & PIR setup
#define RGB_PIN       D2
const int PIR_PIN = D5;
Bounce debouncer = Bounce(); // Bounce is built into Homie, so you can use it without including it first
int lastPirValue = -1;

// defaults for brightness and color
int r = 128;
int g = 126;
int b = 255;
int brightness = 100;
bool ledstate = true;
String set_color ;
String default_color = "128,126,255" ;

// listen for publishes to led/rgb/set (in format "R,G,B")
HomieNode ledNode("led", "rgb");
// listen for publishes to led/brightness/set (in format 0 - 255)
HomieNode brightnessNode("brightness", "brightness");
// listen for publishes to switch/switch/set (in format ON - OFF)
HomieNode switchNode("switch", "switch");
// PIR Node
HomieNode pirNode("motion", "motion");

// when we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, RGB_PIN, NEO_GRB + NEO_KHZ800);

void loopHandler() {
  int pirValue = debouncer.read();

  if (pirValue != lastPirValue) {
     Homie.getLogger() << "Pir is now " << (pirValue ? "ON" : "OFF") << endl;

     pirNode.setProperty("motion").send(pirValue ? "true" : "false");
     lastPirValue = pirValue;
  }
}


bool switchHandler(const HomieRange& range, const String& value) {
  if (value == "ON" && !ledstate) {
    FadeIn(r,g,b);
    switchNode.setProperty("switch").send(value);
    ledstate = true;
  }
  if (value == "ON" && ledstate == true) {
    switchNode.setProperty("switch").send(value);
    ledstate = true;
  }

  else if (value == "OFF") {
    FadeOut(r, g, b);
    pixels.show();
    switchNode.setProperty("switch").send(value);
    ledstate = false;
  }
}

bool rgbHandler(const HomieRange& range, const String& value) {
  set_color = value;
  String r_str = getValue(value, ',', 0);
  String g_str = getValue(value, ',', 1);
  String b_str = getValue(value, ',', 2);

  r = r_str.toInt();
  g = g_str.toInt();
  b = b_str.toInt();

  setAll(r, g, b);
  ledNode.setProperty("rgb").send(value);
  return true;
}

bool brightnessHandler(const HomieRange& range, const String& value) {
  brightness = value.toInt();
  setAll((brightness*r/255),(brightness*g/255),(brightness*b/255));
  brightnessNode.setProperty("brightness").send(value);
  ledNode.setProperty("rgb").send(set_color);
  return true;
}

void setupHandler() {
  pixels.begin();
  FadeIn((brightness*r/255),(brightness*g/255),(brightness*b/255));
  pixels.show();
  switchNode.setProperty("switch").send("ON");
  ledNode.setProperty("rgb").send(default_color);
}


void setup() {
  
  pinMode(PIR_PIN, INPUT);
  digitalWrite(PIR_PIN, LOW);
  debouncer.attach(PIR_PIN);
  debouncer.interval(50);

  Homie_setFirmware(FW_NAME, FW_VERSION);

  ledNode.advertise("rgb").settable(rgbHandler);
  brightnessNode.advertise("brightness").settable(brightnessHandler);
  switchNode.advertise("switch").settable(switchHandler);
  pirNode.advertise("motion");

  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  Homie.setup();
}

void loop() {
  Homie.loop();
  debouncer.update();
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    pixels.setPixelColor(i, red, green, blue); 
  }
  pixels.show();
}

void FadeIn(byte red, byte green, byte blue){
  float r, g, b;
      
  for(int k = 0; k < 256; k=k+1) { 
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    pixels.show();
  }
     
}

void FadeOut(byte red, byte green, byte blue){
  float r, g, b;
      
  for(int k = 255; k >= 0; k=k-2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    pixels.show();
  }
}
