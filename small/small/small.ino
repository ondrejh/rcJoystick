#include <Servo.h>

volatile unsigned long tRx;
char rxBuff[64];
volatile uint8_t rxCnt;

//bool busy = false;

#define BIT5 (1<<5)

#define LED_INIT() do{DDRB|=BIT5;}while(0)
#define LED_ON() do{PORTB|=BIT5;}while(0)
#define LED_OFF() do{PORTB&=~BIT5;}while(0)
#define LED_SWAP() do{PORTB^=BIT5;}while(0)
#define LED ((PORTB&BIT5)!=0)

Servo steer;
Servo lift;
Servo grab;

void setup() {
  Serial.begin(115200);

  steer.attach(10);
  lift.attach(11);
  grab.attach(12);
  
  LED_INIT();
}

void setServo(uint8_t n, uint16_t val)
{
  switch(n) {
    case 0:
      steer.write(map(val, 1000, 2000, 0, 180));
      break;
    case 1:
      break;
    case 2:
      lift.write(map(val, 1000, 2000, 0, 180));
      break;
    case 3:
      grab.write(map(val, 1000, 2000, 0, 180));
      break;
    default:
      break;
  }
}

void loop() {
  static uint32_t lastReceived = 0;
  
  while (Serial.available()) {
    static char lastB = 0;
    char b = (char)Serial.read();
    if ((lastB==0x20) && (b==0x40)) {
      rxCnt = 0;
    }
    else {
      rxBuff[rxCnt++] = b;
      if (rxCnt>=12) {
        LED_ON();
        lastReceived = millis();
        for (int i=0; i<6; i++) {
          union {
            uint8_t b[2];
            int8_t i[2];
            int16_t w;
          } val;
          val.b[0] = rxBuff[2*i];
          val.b[1] = rxBuff[2*i+1];
          //val.w -= 0x3DC;//0x3E8;
          //val.w >>= 2;
          setServo(i,val.w);
          /*if (Serial) {
            Serial.print(val.w);//i[1]);
            if (i<5)
              Serial.print(" ");
            else
              Serial.print("\r\n");
          }*/
        }
        //delay(15);
        //busy = false;
      }
    }
    lastB = b;
  }

  if ((millis()-lastReceived)>=50)
    LED_OFF();
}
