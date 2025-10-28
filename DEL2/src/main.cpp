#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace carrier
{
  namespace pin 
  {
    constexpr uint8_t oledDcPower{6};
    constexpr uint8_t oledCs{10};
    constexpr uint8_t oledReset{5};
  }

  namespace oled
  {
    constexpr uint8_t screenWidth{128};
    constexpr uint8_t screenHeight{64};
  }
}

Adafruit_SSD1306 display(
  carrier::oled::screenWidth,
  carrier::oled::screenHeight,
  &SPI,
  carrier::pin::oledDcPower,   // D/C
  carrier::pin::oledReset,     // RESET
  carrier::pin::oledCs         // CS
);


// HELPER FUNCTIONS
void drawHeader();
void drawFrame();
void drawArc();
void drawCounters();




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //can0.begin();

  if( !display.begin(SSD1306_SWITCHCAPVCC) )
  {
    Serial.println(F("ERROR: display.begin(SSD1306_SWITCHCAPVCC) failed."));

    for(;;)
    {
      // Blink angry LED pattern or something, initializing LED failed.
    }
  }

  // Clear and draw the static UI once
  display.clearDisplay();

  drawFrame();
  drawHeader();
  drawArc();
  drawCounters();

  display.display();
}

void loop() {
  // put your main code here, to run repeatedly:
}



void drawFrame() 
{
  display.drawRoundRect(
    0,      // x0
    0,      // y0
    carrier::oled::screenWidth, 
    carrier::oled::screenHeight,
    6,      // radius
    SSD1306_WHITE
  );
}

void drawHeader() 
{
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(8, 6);
  display.print(F("MAS-245"));

  display.setCursor(72, 6);
  display.print(F("Gruppe 3"));

  //display.drawLine(0, 16, carrier::oled::screenWidth, 16, SSD1306_WHITE);
}

void drawArc() {
 
  const int16_t centerX = 64;
  const int16_t baseY   = 10;
  const int16_t radius  = 64;

 
  for (int angleDeg = 0; angleDeg <= 180; angleDeg += 1) {
    float rad = angleDeg * 3.1415926f / 180.0f;
    int16_t x = centerX + radius * cos(rad);
    int16_t y = baseY   + radius * sin(rad) * 0.2f;
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}


void drawCounters() {
  // "Sent:" label
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(8, 36);
  display.print(F("Sent:"));
  display.setCursor(48, 36);
  display.print(F("0"));   // placeholder

  // "Recv:" label
  display.setCursor(8, 48);
  display.print(F("Recv:"));
  display.setCursor(48, 48);
  display.print(F("0"));   // placeholder
}