#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FlexCAN_T4.h>

#define RADIUS 3
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 16
#define GROUPNUM 3


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
  namespace joystick
    {
    constexpr uint8_t joyLeft {18};
    constexpr uint8_t joyRight {17};
    constexpr uint8_t joyClick {19};
    constexpr uint8_t joyUp {22}; 
    constexpr uint8_t joyDown {23};
    }
}

namespace 
{
  CAN_message_t msg;

  FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> can0;
  //FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;

  Adafruit_SSD1306 display( carrier::oled::screenWidth,
                            carrier::oled::screenHeight,
                            &SPI,
                            carrier::pin::oledDcPower,
                            carrier::pin::oledReset,
                            carrier::pin::oledCs);
}

void startPos();
void ownPaddle();
void oppPaddle();

void setup() {
  Serial.begin(9600);
  if( !display.begin(SSD1306_SWITCHCAPVCC) )
  {
    Serial.println(F("ERROR: display.begin(SSD1306_SWITCHCAPVCC) failed."));

    for(;;)
    {
      // Blink angry LED pattern or something, initializing LED failed.
    }
  }

  display.clearDisplay();
  startPos();
  ownPaddle();
  oppPaddle();

}

void loop() {
    
}

void startPos()
{
  //display.clearDisplay();
  display.drawCircle(64, 32, RADIUS, WHITE);
  display.display();
}

void ownPaddle()
{
  //display.clearDisplay();
  display.drawRect(10, 32 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);
  display.display();
}

void oppPaddle()
{
  //display.clearDisplay();
  display.drawRect(118, 32 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);
  display.display();
}