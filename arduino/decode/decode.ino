static uint8_t t;
static uint8_t cnt;

#define IN ((PIND&(1<<4))!=0)
#define OUT(x) do{x?PORTB|=(1<<6):PORTB&=~(1<<6);}while(0)

volatile uint32_t tRx;
char rxBuff[64];
volatile uint8_t rxCnt;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  Serial1.begin(115200);
  DDRB |= (1<<6);
  DDRD &= ~(1<<4);

  tRx = micros();
  rxCnt = 0;
}

void printHex8(char c) {
  Serial.print((c>>4)&0xF,HEX);
  Serial.print(c&0xF,HEX);
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t now = micros();
  if ((((int32_t)(now-tRx)) > 200) && (rxCnt>0)) {
    OUT(true);
    for (int i=0;i<rxCnt;i++) {
      printHex8(rxBuff[i]);//Serial.print((uint8_t)rxBuff[i],HEX);
      if (i<(rxCnt-1))
        Serial.print(" ");
      else
        Serial.print("\r\n");
    }
    rxCnt=0;
    OUT(false);
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

