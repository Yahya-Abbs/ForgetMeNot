#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include <ESP32Servo.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

const char* ssid = ""; 
const char* password = ""; 
int count = 0; 
const int buzzer = 23;
const int ledPin = 18;


PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

String tagId = "None";

void setup() {
    Serial.begin(115200);
    nfc.begin();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    pinMode(buzzer, OUTPUT);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);


    servo1.attach(14);
    servo2.attach(27);
    servo3.attach(25);
    servo4.attach(32); 

}

void run(Servo &servo) {
    delay(500);
    for(int pos = 110; pos >= 0; pos--) {
      delay(15);
      servo.write(pos);
    }

    delay(5000);
    for(int pos = 0; pos <= 110; pos++) {
        delay(15);
        servo.write(pos);
    }
}

void run2(Servo &servo) {
    delay(500);
    for(int pos = 90; pos >= 0; pos--) {
      delay(15);
      servo.write(pos);
    }

    delay(5000);
    for(int pos = 0; pos <= 90; pos++) {
        delay(15);
        servo.write(pos);
    }
}



// void run4() {
//     servo4.write(85);
//     delay(1100);
//     servo4.write(92);
//     delay(1000);
//     servo4.write(97);
//     delay(1100);
//     servo4.write(92);
// }

unsigned long startTime = 0;
void loop() {
    if (nfc.tagPresent()){
        count += 1; 
        NfcTag tag = nfc.read();
        tagId = tag.getUidString();
        Serial.println("Tag detected");

        tone(buzzer, 1000, 500); 
        digitalWrite(ledPin, HIGH);

        if (count == 1) {
            run(servo3);
            digitalWrite(ledPin, LOW);
            delay(40000);
            
        }
        else if (count == 2) {
            run(servo1);
            digitalWrite(ledPin, LOW);
            delay(20000);
        }
        else if (count == 3) {  
            run2(servo2);
            digitalWrite(ledPin, LOW);
            delay(30000);
            count = 0; 
        }
    }
}