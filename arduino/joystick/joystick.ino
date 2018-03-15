#include "Joystick.h"

// Create Joystick
Joystick_ Joystick;

static uint8_t t;
static uint8_t cnt;

#define IN ((PIND&(1<<4))!=0)
#define OUT(x) do{x?PORTB|=(1<<6):PORTB&=~(1<<6);}while(0)

// max 0x07CC
// min 0x03F1

volatile uint32_t tRx;
char rxBuff[64];
volatile uint8_t rxCnt;

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  DDRB |= (1<<6);
  DDRD &= ~(1<<4);

  tRx = micros();
  rxCnt = 0;

  // Set Range Values
  Joystick.setXAxisRange(-127, 127);
  Joystick.setYAxisRange(-127, 127);
  Joystick.setZAxisRange(-127, 127);
  Joystick.setRxAxisRange(0, 360);
  Joystick.setRyAxisRange(360, 0);
  Joystick.setRzAxisRange(0, 720);
  Joystick.setThrottleRange(0, 255);
  Joystick.setRudderRange(255, 0);

  Joystick.begin();
  //Serial.begin(115200);
}

/*void printHex8(char c) {
  Serial.print((c>>4)&0xF,HEX);
  Serial.print(c&0xF,HEX);
}*/

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t now = micros();
  if ((((int32_t)(now-tRx)) > 200) && (rxCnt>0)) {
    static int thiscnt = 0;
    thiscnt++;
    if (thiscnt>=3) {
      if ((rxCnt>=14) && (rxBuff[0]==0x20) && (rxBuff[1]==0x40)) {
        OUT(true);
        for (int i=0; i<6; i++) {
          union {
            uint8_t b[2];
            int8_t i[2];
            int16_t w;
          } val;
          val.b[0] = rxBuff[2*i+2];
          val.b[1] = rxBuff[2*i+3];
          val.w -= 0x3DC;//0x3E8;
          val.w >>= 2;
          switch (i) {
          case 0: Joystick.setXAxis(val.i[1]);/*val.i[1]);*/ break;
          case 1: Joystick.setYAxis(val.i[1]); break;
          case 2: Joystick.setZAxis(val.i[1]); break;
          }
          /*Serial.print(val.i[1]);
          if (i<5)
            Serial.print(" ");
          else
            Serial.print("\r\n");*/
        }
        //Joystick.sendState();
        OUT(false);
        thiscnt = 0;
      }
    }
    /*else 
    for (int i=0;i<rxCnt;i++) {
      printHex8(rxBuff[i]);//Serial.print((uint8_t)rxBuff[i],HEX);
      if (i<(rxCnt-1))
        Serial.print(" ");
      else
        Serial.print("\r\n");
    }*/
    rxCnt=0;
  }
}

void serialEvent1() {
  while (Serial1.available()) {
    // get the new byte:
    rxBuff[rxCnt++] = (char)Serial1.read();
    tRx = micros();
    /*uint32_t now=millis();
    static uint32_t last;
    static bool o = false;
    if ((now-last)>2) {
      OUT(o);
      o = !o;
    }
    last = now;*/
  }
}

