void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
  static uint8_t t;
  static uint8_t cnt;
  // put your main code here, to run repeatedly:
  while (Serial1.available()) {
    Serial1.read();
    cnt++;
    uint8_t now = millis();
    if ((now-t)>6) {
      Serial.println(cnt);
      cnt=0;
    }
    t=now;
  }
}
