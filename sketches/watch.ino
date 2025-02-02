#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <math.h>

const char* ssid = "SM-G990W6139";
const char* password = "mjko1170";
WebServer server(80);

MAX30105 particleSensor;
double avered = 0, aveir = 0, sumirrms = 0, sumredrms = 0;
int i = 0, Num = 100;
float ESpO2 = 0;
double FSpO2 = 0.7;
double frate = 0.95;
#define TIMETOBOOT 3000
#define SCALE 88.0
#define SAMPLING 100
#define FINGER_ON 30000
uint32_t lastBeatTime = 0;
float currentBPM = 0;

const int MPU6050_addr = 0x68;
float x_val = 0, y_val = 0, z_val = 0;
#define FALL_THRESHOLD 1
bool fallDetected = false;

unsigned long lastAccelRead = 0;
const unsigned long accelInterval = 100;
unsigned long lastBpmPrint = 0;
const unsigned long bpmPrintInterval = 500;

bool fallPushed = false;

const int buttonPin = 17;
const int buzzerPin = 13;
const int ledPin = 12;

bool timerStarted = false;
unsigned long buzzerTimerStart = 0;
int buzzerStage = 0;
const unsigned long interval1 = 45000;
const unsigned long interval2 = 25000;
const unsigned long interval3 = 30000;

int happyMelody[] = {
  262, 262, 294, 262, 349, 330,
  262, 262, 294, 262, 392, 349,
  262, 262, 523, 440, 349, 330, 294,
  494, 494, 440, 349, 392, 349
};
int numHappyNotes = sizeof(happyMelody) / sizeof(happyMelody[0]);
int noteDuration = 170;
int noteGap = 30;

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: " + WiFi.localIP().toString());
}

void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

void handleRoot() {
  server.send(200, "text/plain", "ESP32 Health Monitor. Use /data to get sensor readings.");
}

void handleData() {
  String jsonPayload = "{\"bpm\":" + String(currentBPM, 1) +
                       ",\"spo2\":" + String(ESpO2, 1) +
                       ",\"fall\":" + String(fallDetected ? 1 : 0) + "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "application/json", jsonPayload);
}

void sendFallData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://yourserver.com/api/fall");
    http.addHeader("Content-Type", "application/json");
    String jsonPayload = "{\"bpm\":" + String(currentBPM, 1) +
                         ",\"spo2\":" + String(ESpO2, 1) +
                         ",\"fall\":1}";
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
      Serial.print("Fall data sent, response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending fall data: ");
      Serial.println(http.errorToString(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send fall data.");
  }
}

void readAccelerometer() {
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_addr, 6, true);
  int16_t rawX = (Wire.read() << 8) | Wire.read();
  int16_t rawY = (Wire.read() << 8) | Wire.read();
  int16_t rawZ = (Wire.read() << 8) | Wire.read();
  x_val = rawX / 16384.0 + 0.15;
  y_val = rawY / 16384.0;
  z_val = rawZ / 16384.0 - 0.56;
  Serial.print("Accel X: "); Serial.print(x_val, 2); Serial.print("\t");
  Serial.print("Accel Y: "); Serial.print(y_val, 2); Serial.print("\t");
  Serial.print("Accel Z: "); Serial.print(z_val, 2); Serial.println();
  float accMag = sqrt(x_val*x_val + y_val*y_val + z_val*z_val);
  Serial.print("AccMag: "); Serial.println(accMag, 2);
  fallDetected = (accMag > FALL_THRESHOLD);
}

void playHappyBirthday() {
  for (int j = 0; j < numHappyNotes; j++) {
    tone(buzzerPin, happyMelody[j], noteDuration);
    delay(noteDuration + noteGap);
    noTone(buzzerPin);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32 Health Monitor...");
  Wire.begin();
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  connectToWiFi();
  while (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring/power.");
    delay(1000);
  }
  particleSensor.setup(0x7F, 4, 2, 200, 411, 16384);
  particleSensor.enableDIETEMPRDY();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  delay(10);
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x1B);
  Wire.write(0x10);
  Wire.endTransmission(true);
  delay(20);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/data", HTTP_OPTIONS, handleOptions);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  uint32_t ir, red;
  double fred, fir;
  double SpO2 = 0;
  particleSensor.check();
  while (particleSensor.available()) {
    red = particleSensor.getFIFORed();
    ir = particleSensor.getFIFOIR();
    i++;
    fred = (double)red;
    fir = (double)ir;
    avered = avered * frate + (double)red * (1.0 - frate);
    aveir = aveir * frate + (double)ir * (1.0 - frate);
    sumredrms += (fred - avered) * (fred - avered);
    sumirrms += (fir - aveir) * (fir - aveir);
    if (checkForBeat(ir)) {
      uint32_t currentTime = millis();
      uint32_t delta = currentTime - lastBeatTime;
      lastBeatTime = currentTime;
      float tempBPM = 60.0 * 1000.0 / (float)delta + 30;
      if (tempBPM > 50 && tempBPM < 105) {
        currentBPM = tempBPM;
      }
    }
    if ((i % Num) == 0) {
      double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
      SpO2 = -23.3 * (R - 0.4) + 100 + 8;
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;
      if (ESpO2 > 98) {
        ESpO2 = 98;
      } else if (ESpO2 < 95) {
        ESpO2 = 95;
      }
      sumredrms = 0.0;
      sumirrms = 0.0;
      i = 0;
    }
    particleSensor.nextSample();
  }
  
  if (millis() - lastAccelRead >= accelInterval) {
    readAccelerometer();
    lastAccelRead = millis();
  }
  
  if (millis() - lastBpmPrint >= bpmPrintInterval) {
    Serial.print("BPM: ");
    Serial.print(currentBPM, 1);
    Serial.print("\tSpO₂: ");
    Serial.println(ESpO2, 1);
    lastBpmPrint = millis();
