// #include <LiquidCrystal.h>
#include <LM35.h>
#include <RH_ASK.h>
#include "temp-alarm-base.h"

#define NUMBER_OF_SAMPLES 128
#define SAMPLING_DELAY 15

#define LCD_PIN_RS 12
#define LCD_PIN_E 11
#define LCD_PIN_RW 10
#define LCD_PIN_D0 2
#define LCD_PIN_D1 3
#define LCD_PIN_D2 4
#define LCD_PIN_D3 5

#define RF_BITRATE 2000
#define RF_PIN_RX 11
#define RF_PIN_TX 255
#define RF_PIN_PTT 255

#define DEBUG

// static LiquidCrystal lcd(LCD_PIN_RS, LCD_PIN_E, LCD_PIN_D0, LCD_PIN_D1, LCD_PIN_D2, LCD_PIN_D3);
static RH_ASK rf;

void setup() {
  analogReference(INTERNAL);
#ifdef DEBUG
  Serial.begin(9600);
#endif
  if (!rf.init()) {
#ifdef DEBUG
    Serial.println(F("init RF module failed!"));
#endif
  } else {
#ifdef DEBUG
    delay(2000);
    Serial.println(F("RF module init done"));
#endif
    rf.setModeRx();
  }
}

#ifdef DEBUG
static uint16_t lastTime = 0L;
#endif

static LM35 temp(A0, NUMBER_OF_SAMPLES, FARENHEIGHT, SAMPLING_DELAY);

#define TEMP_TIMEOUT 2500


RemoteTemp parseTemp(char* msg) {
  RemoteTemp t;
  t.lastMessageTime = millis();
  String smsg = String(msg);
  t.id = (byte)smsg.substring(0,2).toInt();
  t.value = smsg.substring(2).toDouble();
  return t;
}

bool isTempExpired(RemoteTemp t) {
  return millis() - t.lastMessageTime > TEMP_TIMEOUT;
}

static RemoteTemp temps[8];

void loop() {
  // int reading = analogRead(A0); // [0, 1024)
  // sumSq += LM35_CONVERSION_CONSTANT*LM35_CONVERSION_CONSTANT * reading*reading;

  uint8_t buf[10];
  uint8_t buflen = sizeof(buf);
  if (rf.recv(buf, &buflen)) {
    Serial.print("Received message: ");
    Serial.println((char*)buf);
    RemoteTemp tmsg = parseTemp((char*)buf);
    if (tmsg.id >= 0) {
      temps[tmsg.id] = tmsg;
    }
  }

#ifdef DEBUG
  uint16_t thisTime = millis();
  uint16_t elapsed = thisTime - lastTime;
  if (elapsed > 2500) {
    Serial.print(thisTime);
    Serial.println(": waiting");
    lastTime = thisTime;
  }
#endif
  if (!temp.sampleTemp()) {
    return;
  }
  // if (++sampleIndex < NUMBER_OF_SAMPLES) {
  //   delay(SAMPLING_DELAY);
  //   return;
  // }
  // sampleIndex = 0;
  // double val = toF(sqrt(sumSq / NUMBER_OF_SAMPLES));
  // sumSq = 0.0;
#ifdef DEBUG
 Serial.print(F("BASE temp: "));
 Serial.println(temp.currentTemp(), 4);
 Serial.println();
 for (int i = 0; i < 8; i++) {
   RemoteTemp t = temps[i];
   if (isTempExpired(t)) {
     t.id = -1;
   }
   if (t.id >= 0) {
     Serial.print("Sensor ");
     Serial.print(t.id);
     Serial.print(": ");
     Serial.println(t.value, 3);
   }
 }
#endif
//  // lcd.setCursor(0,1);
//  // lcd.print(val, 2);
//  // lcd.print("   ");
//  // lcd.print(uptime/1000.0, 2);
  delay(SAMPLING_DELAY);
}

inline double toF(double inC) {
  return (inC * 9.0 / 5.0) + 32.0;
}
