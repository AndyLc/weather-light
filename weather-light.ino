#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "ssid";
const char* password = "password";
const char* apiLink = "http://api.openweathermap.org/customlink";

int theaterChaseRainbowJ = 0;
int theaterChaseRainbowDir = 1;
float lastTemp = -1;
bool danger = false;
bool rain = false;
int forecastDistance = 8;

#define PIN D4

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);
uint32_t danger_color = strip.Color(205, 0, 20);
uint32_t loading_color = strip.Color(255, 255, 255);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..");
  }
  Serial.println("Connected!");
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    
    HTTPClient http;
    http.begin(apiLink);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      //parse JSON
      String json = http.getString();
      int index = 0;
      //get the JSON relevant forecastDistance * 3 hrs into the future.
      for (int i = 0; i < forecastDistance; i++) {
        index = json.indexOf("{\"dt\"", index) + 1;
      }
      index = index - 1;
      int endIndex = json.indexOf(",{\"dt", index);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(json.substring(index, endIndex));
      
      if (!root.success()) {
        Serial.println("parseObject() failed");
      } else {
        lastTemp = (float) root["main"]["temp"];
        JsonArray& weathertypes = root["weather"];
        for (int i = 0; i < weathertypes.size(); i++) {
          int id = weathertypes.get<JsonObject>(i)["id"];
          Serial.println(id);
          if (id == 212 || id == 232 || id == 202 || id == 312 || id == 504 || id == 503 || id == 502 || id == 511 || id == 522 || id == 602 || id == 622 || (id > 700 && id < 800)) {
            danger = true;
          } else if (id >= 500 && id < 600) {
            rain = true;
          } else {
            danger = false;
            rain = false;
          }
        }
      }
    }
    http.end();   //Close connection
  }

  //set color config for light
  if (lastTemp == -1) {
    theaterChaseNTimes(20, loading_color, 100);
  } else if (danger) {
    theaterChaseNTimes(75, danger_color, 100);
  } else if (rain) {
    theaterChaseRainbowNTimes(50, 150);
  } else if (lastTemp > 302.594) {
    rainbowNTimes(50, 25, "hot"); 
  } else if (lastTemp < 288.706) {
    rainbowNTimes(50, 25, "cold"); 
  } else {
    rainbowCycle(50, 25);
  }
}

void rainbowNTimes(uint8_t wait, int n, String type) {
  uint16_t i, j;
  if (type == "hot") {
    for(int k = 0; k < n; k++) {
      for(j=0; j<256; j++) {
        for(i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, HotWheel((i+j) & 255));
        }
        strip.show();
        delay(wait);
      }
    }
  } else {
    for(int k = 0; k < n; k++) {
      for(j=0; j<256; j++) {
        for(i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, ColdWheel((i+j) & 255));
        }
        strip.show();
        delay(wait);
      }
    }
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, int k) {
  uint16_t i, j;

  for(j=0; j<256*k; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChaseNTimes(int n, int32_t c, uint8_t wait) {
  for (int k=0; k<n; k++) {
      for (int q=0; q < 2; q++) {
        for (uint16_t i=0; i < strip.numPixels(); i=i+2) {
          strip.setPixelColor(i+q, c);    //turn every third pixel on
        }
        strip.show();
        delay(wait);
        for (uint16_t i=0; i < strip.numPixels(); i=i+2) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
        delay(wait);
      }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbowNTimes(int n, uint8_t wait) {
  int dir = 1;
  for (int k=0; k < n; k++) {
      for (int q=0; q < 2; q++) {
        for (uint16_t i=0; i < strip.numPixels(); i=i+2) {
          strip.setPixelColor(i+q, ColdWheel( (i + theaterChaseRainbowJ) % 255));    //turn every third pixel on
        }
        theaterChaseRainbowJ+=theaterChaseRainbowDir;
        if (theaterChaseRainbowJ >= 255) {
          theaterChaseRainbowDir = -1;
        } else if (theaterChaseRainbowJ == 0) {
          theaterChaseRainbowDir = 1;
        }
        strip.show();
        delay(wait);
        for (uint16_t i=0; i < strip.numPixels(); i=i+2) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
        delay(wait);
      }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t HotWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 128) {
    return strip.Color(128 + WheelPos, 0, 255 - WheelPos * 2);
  } else {
    return strip.Color(255 - (WheelPos - 128), WheelPos * 2 - 128, 0); 
  }
}

uint32_t ColdWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 128) {
    return strip.Color(128 - WheelPos, WheelPos * 2, 255);
  } else {
    return strip.Color(WheelPos - 128, 255, 255 - (WheelPos - 128) * 2); 
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t RegularWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
