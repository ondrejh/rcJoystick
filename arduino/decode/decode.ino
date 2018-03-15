static uint8_t t;
static uint8_t cnt;

#define BIT5 (1<<5)

#define LED_INIT() do{DDRB|=BIT5;}while(0)
#define LED_ON() do{PORTB|=BIT5;}while(0)
#define LED_OFF() do{PORTB&=~BIT5;}while(0)
#define LED_SWAP() do{PORTB^=BIT5;}while(0)
#define LED ((PORTB&BIT5)!=0)

#define RXBUFLEN 64
uint8_t rxBuf[RXBUFLEN];
uint8_t rxBufPtr = 0;
uint32_t rxTim = 0;

uint8_t pRxCnt = 0, pCnt = 0; 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  LED_INIT();
}

void loop() {
  static int s = 0;
  static uint32_t ledTim = 0;
  static uint32_t pRecTim = 0;
  uint32_t nu = micros();
  uint32_t nm = millis();

  if ((pRxCnt==pCnt) && (rxBufPtr>16) && ((nu-rxTim)>200) && (rxBuf[0]==0x20) && (rxBuf[1]==0x40)) {
    rxBufPtr = 0;
    pRxCnt ++;
    pRecTim = nm;
  }

  if (pCnt!=pRxCnt) {
    pCnt = pRxCnt;
    LED_ON();
  }

  if (LED && ((nm-pRecTim)>200))
    LED_OFF();

  /*switch (s) {
    case 0: // waiting packet
      if (((nu-rxTim)>200)&&(rxBufPtr>16)) {
        s++;
        LED_ON();
      }
      else {
        if ((nm-ledTim)>500) {
          ledTim = nm;
          LED_SWAP();
        }
      }
      break;
    case 1: // receiving
      if ((nu-rxTim)>20000) {
        LED_OFF();
        ledTim = nm;
        s--;
      }
      break;
  }*/
  //uint32_t now = micros();
  //if (LED&&((now-rxTim)>20)) {
  //  LED_OFF();
  //}
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    uint32_t now = micros();
    if ((now - rxTim)>200) {
      rxBufPtr = 0;
    }
    rxBuf[rxBufPtr++] = (uint8_t)Serial.read();
    rxTim = now;
    if (rxBufPtr>=RXBUFLEN)
      rxBufPtr=RXBUFLEN-1;
  }
}
