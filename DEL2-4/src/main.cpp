#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FlexCAN_T4.h>

#define RADIUS 3
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 20

#define OWN_GROUPNUM 3
#define OPP_GROUPNUM 1

#define OWN_BALL_ID    (OWN_GROUPNUM + 50)
#define OPP_BALL_ID    (OPP_GROUPNUM + 50)
#define OWN_PADDLE_ID  (OWN_GROUPNUM + 20)
#define OPP_PADDLE_ID  (OPP_GROUPNUM + 20)

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

  Adafruit_SSD1306 display( carrier::oled::screenWidth,
                            carrier::oled::screenHeight,
                            &SPI,
                            carrier::pin::oledDcPower,
                            carrier::pin::oledReset,
                            carrier::pin::oledCs);
}

void ballPos(int x, int y);
void ownPaddle(int y);
void oppPaddle(int y);
void resetBall();
void sendPaddleY();
void sendBallPos();

int ownPaddle_y = 32;
int oppPaddle_y = 32;
int ballPos_x = 64;
int ballPos_y = 32;
int vX = 0;
int vY = 0;
bool started = false;
bool iAmMaster = false;

elapsedMicros tick100Hz;

int mirrorX(int x)
{
  return (carrier::oled::screenWidth - 1) - x;
}

void onCanReceive(const CAN_message_t &rx)
{
  if (rx.id == OPP_PADDLE_ID && rx.len >= 1)
  {
    oppPaddle_y = constrain((int)rx.buf[0], PADDLE_HEIGHT/2, carrier::oled::screenHeight - PADDLE_HEIGHT/2);
  }

  if (rx.id == OPP_BALL_ID && rx.len >= 2)
  {
    int rx_x = rx.buf[0];
    int rx_y = rx.buf[1];
    ballPos_x = mirrorX(rx_x);
    ballPos_y = rx_y;
    if (!iAmMaster)
    {
      started = true;
      vX = 0; 
      vY = 0;
    }
  }
}

void sendPaddleY()
{
  CAN_message_t tx;
  tx.id = OWN_PADDLE_ID;
  tx.len = 1;
  tx.buf[0] = (uint8_t)constrain(ownPaddle_y, 0, 255);
  can0.write(tx);
}

void sendBallPos()
{
  if (!iAmMaster) return;
  CAN_message_t tx;
  tx.id = OWN_BALL_ID;
  tx.len = 2;
  tx.buf[0] = (uint8_t)constrain(ballPos_x, 0, 255);
  tx.buf[1] = (uint8_t)constrain(ballPos_y, 0, 255);
  can0.write(tx);
}

void setupCAN()
{
  can0.begin();
  can0.setBaudRate(500000);
  can0.setMaxMB(16);
  can0.enableFIFO();
  can0.enableFIFOInterrupt();
  can0.onReceive(onCanReceive);
  can0.mailboxStatus();
}

void setup() 
{
  Serial.begin(9600);
  if( !display.begin(SSD1306_SWITCHCAPVCC) )
  {
    Serial.println(F("ERROR: display.begin failed."));
    for(;;){}
  }

  pinMode(carrier::joystick::joyUp,   INPUT_PULLUP);
  pinMode(carrier::joystick::joyDown, INPUT_PULLUP);

  setupCAN();

  display.clearDisplay();
  ballPos(64,32);
  ownPaddle(32);
  oppPaddle(32);
  display.display();
}

void loop()
{
  int joyUpState   = digitalRead(carrier::joystick::joyUp);
  int joyDownState = digitalRead(carrier::joystick::joyDown);

  static int frame = 0;
  frame++;

  bool paddleMoved = false;

  if (joyUpState == LOW && frame % 5 == 0)
  {
    ownPaddle_y--;
    paddleMoved = true;
  }
  else if (joyDownState == LOW && frame % 5 == 0)
  {
    ownPaddle_y++;
    paddleMoved = true;
  }

  if (paddleMoved && !started)
  {
    iAmMaster = true;
    started = true;
    vX = +1; 
    vY = 0;
  }

  ownPaddle_y = constrain(ownPaddle_y, PADDLE_HEIGHT/2, carrier::oled::screenHeight - PADDLE_HEIGHT/2);
  oppPaddle_y = constrain(oppPaddle_y, PADDLE_HEIGHT/2, carrier::oled::screenHeight - PADDLE_HEIGHT/2);

  if (frame % 7 == 0)
  {
    sendPaddleY();
  }

  if (iAmMaster && started && frame % 7 == 0)
  {
    ballPos_x += vX;
    ballPos_y += vY;

    if (ballPos_y - RADIUS <= 0)
    {
      ballPos_y = RADIUS;
      vY = -vY;
    }
    if (ballPos_y + RADIUS >= carrier::oled::screenHeight)
    {
      ballPos_y = carrier::oled::screenHeight - RADIUS;
      vY = -vY;
    }

    int lp_x = 10;
    int lp_y_top = ownPaddle_y - PADDLE_HEIGHT/2;
    int lp_y_bot = ownPaddle_y + PADDLE_HEIGHT/2;
    if (ballPos_x - RADIUS <= lp_x + PADDLE_WIDTH && ballPos_x >= lp_x && ballPos_y >= lp_y_top && ballPos_y <= lp_y_bot)
    { 
      vX = abs(vX);
      if (ballPos_y < ownPaddle_y) vY = -1;
      else if (ballPos_y > ownPaddle_y) vY = +1;
      ballPos_x = lp_x + PADDLE_WIDTH + RADIUS;
    }

    int rp_x = 118;
    int rp_y_top = oppPaddle_y - PADDLE_HEIGHT/2;
    int rp_y_bot = oppPaddle_y + PADDLE_HEIGHT/2;
    if (ballPos_x + RADIUS >= rp_x && ballPos_x <= rp_x + PADDLE_WIDTH && ballPos_y >= rp_y_top && ballPos_y <= rp_y_bot)
    {
      vX = -abs(vX);
      if (ballPos_y < oppPaddle_y) vY = -1;
      else if (ballPos_y > oppPaddle_y) vY = +1;
      ballPos_x = rp_x - RADIUS;
    }

    if (ballPos_x < -10 || ballPos_x > carrier::oled::screenWidth + 10)
    {
      resetBall();
    }
  }

  if (iAmMaster && started && tick100Hz >= 10000)
  {
    tick100Hz = 0;
    sendBallPos();
  }

  display.clearDisplay();
  ballPos(ballPos_x,ballPos_y);
  ownPaddle(ownPaddle_y);
  oppPaddle(oppPaddle_y);
  display.display();
}

void ballPos(int x, int y)
{
  display.drawCircle(x, y, RADIUS, WHITE);
}

void ownPaddle(int y)
{
  display.drawRect(10, y - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);
}

void oppPaddle(int y)
{
  display.drawRect(118, y - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);
}

void resetBall() 
{
  ballPos_x = 64;
  ballPos_y = 32;
  vX = 0; 
  vY = 0;
  started = false;
}
