#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FlexCAN_T4.h>

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

  volatile uint32_t sentCount = 0;
  volatile uint32_t recvCount = 0;

  unsigned long lastSendMillis = 0;
  constexpr unsigned long sendIntervalMs = 1000;

  volatile uint32_t lastSentId = 0; // <-- new: store last sent ID
  volatile uint32_t lastRecvId = 0; // <-- NEW: store last received ID
  volatile bool     lastRecvExtended = false; // NEW: whether last received ID was extended
}

struct Message 
{
    uint8_t sequenceNumber;
    float temperature;
};

// HELPER FUNCTIONS
void drawHeader();
void drawFrame();
void drawArc();
void drawCounters();
void readCanMessage();
void sendCan();
void sendCan(const Message& message);


void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  can0.begin();
  can0.setBaudRate(250000);

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

void loop() 
{
  // put your main code here, to run repeatedly:
  readCanMessage();

  unsigned long now = millis();
  if (now - lastSendMillis >= sendIntervalMs) {
    lastSendMillis = now;
  sendCan();

  drawCounters();
  display.display();
  }
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
  display.print(F("Gruppe 4"));

  //display.drawLine(0, 16, carrier::oled::screenWidth, 16, SSD1306_WHITE);
}

void drawArc() 
{
 
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

void drawCounters() 
{
  // "Sent:" label
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(8, 36);
  display.print(F("Sent:"));
  display.fillRect(40, 36, 40, 8, SSD1306_BLACK);
  display.setCursor(40, 36);
  display.print(sentCount);   // dynamic
  display.setCursor(72, 36);
  display.print(F("ID:"));  
  display.setCursor(90, 36);
  
  display.fillRect(90, 36, 38, 8, SSD1306_BLACK);
  // print lastSentId as hex with 0x prefix
  char idBuf[12];
  snprintf(idBuf, sizeof(idBuf), "0x%03lX", (unsigned long)lastSentId);
  display.setCursor(90, 36);
  display.print(idBuf);

  // "Recv:" label
  display.setCursor(8, 48);
  display.print(F("Recv:"));
  display.fillRect(40, 48, 40, 8, SSD1306_BLACK);
  display.setCursor(40, 48);
  display.print(recvCount);   // dynamic
  display.setCursor(72, 48);
  display.print(F("ID:"));
  display.fillRect(90, 48, 38, 8, SSD1306_BLACK);

  // print lastRecvId; use wider format for extended IDs
  if (lastRecvExtended) {
    snprintf(idBuf, sizeof(idBuf), "0x%08lX", (unsigned long)lastRecvId);
  } else {
    snprintf(idBuf, sizeof(idBuf), "0x%03lX", (unsigned long)lastRecvId);
  }
  display.setCursor(90, 48);
  display.print(idBuf);

}



void readCanMessage() 
{
  while (can0.read(msg)) {
    // A message was received
    recvCount++;
// record ID and whether it's extended
    lastRecvId = msg.id;
  //  lastRecvExtended = (msg.flags.ext != 0);

// print basic info to serial for debugging
    Serial.print(F("CAN RX ID: 0x"));
    Serial.println(msg.id, HEX);
    Serial.print(F("DLC: "));
    Serial.println(msg.len);
    Serial.print(F("Data: "));
    for (uint8_t i = 0; i < msg.len; ++i) {
      if (msg.buf[i] < 0x10) Serial.print('0');
      Serial.print(msg.buf[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
  }
}


void sendCan()
{
  msg.len = 3;
  msg.id = 0x002;
  msg.buf[0] = 0x26;
  msg.buf[1] = 0x42;
  msg.buf[2] = 0x00;
  bool ok = can0.write(msg);
  if (ok) {
    sentCount++;
    lastSentId = msg.id; // <-- record ID of sent message
  } else {
    Serial.println(F("CAN TX failed"));
  }
}

void sendCan(const Message& message) 
{
    // Create a CAN message object (assuming CanMsg is a type you have defined)
    CAN_message_t msg;

    // Set the CAN message ID and length
    msg.id = 0x245;
    msg.len = 1 + sizeof(float);  // Will not work with double, as double on this platform is size 8, and msg.buf maximum size is 8.

    // Serialize the uint8_t field directly into the first byte of the buffer
    msg.buf[0] = message.sequenceNumber;

    // Serialize the float field into the remaining bytes of the buffer. Asume same endianness for all platforms.
    memcpy(&msg.buf[1], &message.temperature, sizeof(float));

    // Send the CAN message
    bool ok = can0.write(msg);
    if (!ok) {
      Serial.println(F("CAN send failed."));
    } else {
      sentCount++;
      lastSentId = msg.id; // <-- record ID of sent message
    }
}