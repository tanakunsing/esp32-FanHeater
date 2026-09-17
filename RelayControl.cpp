#include "RelayControl.h"
#include "Config.h"

bool fanState   = false;
bool heat1State = false;
bool heat2State = false;
bool swingState = false;

void relayInit() {
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_HEAT1, OUTPUT);
  pinMode(PIN_HEAT2, OUTPUT);
  pinMode(PIN_SWING, OUTPUT);
  powerOff();
}

void fan(bool state) {
  fanState = state;
  digitalWrite(PIN_FAN, state ? HIGH : LOW);
  Serial.println(state ? F("Fan ON") : F("Fan OFF"));
}

void heat1(bool state) {
  heat1State = state;
  digitalWrite(PIN_HEAT1, state ? HIGH : LOW);
  Serial.println(state ? F("Heat1 (1000W) ON") : F("Heat1 (1000W) OFF"));
}

void heat2(bool state) {
  heat2State = state;
  digitalWrite(PIN_HEAT2, state ? HIGH : LOW);
  Serial.println(state ? F("Heat2 (1500W) ON") : F("Heat2 (1500W) OFF"));
}

void swing(bool state) {
  swingState = state;
  digitalWrite(PIN_SWING, state ? HIGH : LOW);
  Serial.println(state ? F("Swing ON") : F("Swing OFF"));
}

// 1. Power On = เปิดพัดลม
void powerOn() {
  fan(true);
}

// 2. Power Off = ปิดอุปกรณ์ทั้งหมด
void powerOff() {
  fan(false);
  heat1(false);
  heat2(false);
  swing(false);
}

void printStatus() {
  Serial.print(F("Fan="));   Serial.print(fanState ? "ON " : "OFF ");
  Serial.print(F("Heat1=")); Serial.print(heat1State ? "ON " : "OFF ");
  Serial.print(F("Heat2=")); Serial.print(heat2State ? "ON " : "OFF ");
  Serial.print(F("Swing=")); Serial.println(swingState ? "ON" : "OFF");
}
