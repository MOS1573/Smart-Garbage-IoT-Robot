// Smart bin (ESP32 + Blynk IoT).
//
// - An IR sensor at the lid detects a hand; a servo opens the lid for 5 s.
// - An ultrasonic sensor inside the lid measures how full the bin is.
// - Fill level, distance and lid state go to a Blynk dashboard over Wi-Fi.
//
// Blynk datastreams (virtual pins):
//   V0  distance to the rubbish, cm   (0..BIN_HEIGHT_CM)
//   V1  fill level, %                 (0..100, gauge)
//   V2  lid angle, deg                (0 closed, 90 open)
//
// Libraries: Blynk (1.x), ESP32Servo.
// The original project used Blynk.Edgent for Wi-Fi provisioning; this version
// takes the credentials below so it builds without the Edgent support files.

#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxxx"     // from your Blynk template
#define BLYNK_TEMPLATE_NAME "Smart Dustbin"
#define BLYNK_AUTH_TOKEN    "your-device-token"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASS = "your-password";

// Pins (as wired in the handbook, Fig. 2-1)
const int IR_PIN    = 34;   // LOW when a hand is in front of the lid
const int SERVO_PIN = 13;
const int ECHO_PIN  = 32;
const int TRIG_PIN  = 33;

const int BIN_HEIGHT_CM  = 21;    // distance from sensor to the bottom when empty
const int LID_OPEN_ANGLE = 90;
const unsigned long LID_OPEN_MS = 5000;

Servo lid;
BlynkTimer timer;

bool lid_open = false;
unsigned long lid_opened_at = 0;

int readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  const long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return BIN_HEIGHT_CM;      // no echo: treat as empty
  return duration * 0.034 / 2;                  // speed of sound 0.034 cm/us
}

void sendFillLevel() {
  const int distance = constrain(readDistanceCm(), 0, BIN_HEIGHT_CM);
  // Rubbish close to the sensor means a full bin.
  const int bin_level = map(distance, BIN_HEIGHT_CM, 0, 0, 100);
  Blynk.virtualWrite(V0, distance);
  Blynk.virtualWrite(V1, bin_level);
}

// Runs every 100 ms: open the lid for a hand, close it again after 5 s.
void checkLid() {
  const unsigned long now = millis();
  if (!lid_open && digitalRead(IR_PIN) == LOW) {
    lid.write(LID_OPEN_ANGLE);
    lid_open = true;
    lid_opened_at = now;
    Blynk.virtualWrite(V2, LID_OPEN_ANGLE);
  } else if (lid_open && now - lid_opened_at >= LID_OPEN_MS) {
    lid.write(0);
    lid_open = false;
    Blynk.virtualWrite(V2, 0);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(IR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lid.attach(SERVO_PIN);
  lid.write(0);

  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  timer.setInterval(100L, checkLid);
  timer.setInterval(1000L, sendFillLevel);
}

void loop() {
  Blynk.run();
  timer.run();
}
