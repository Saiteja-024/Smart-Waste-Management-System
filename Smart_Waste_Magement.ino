#define BLYNK_TEMPLATE_ID "TMPL32xWViSCL"
#define BLYNK_TEMPLATE_NAME "smartbin"
#define BLYNK_AUTH_TOKEN "eHuvPEdkpbWMUioNc8G6BVf1qYAPbYeM"

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // LCD Display Library
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>  // Servo library

// WiFi credentials
char ssid[] = "Realme 8 5G";     
char pass[] = "0987654321";      

// MQ-135 Sensor (Air Quality)
#define MQ135_A0 A0  // Analog pin for air quality measurement
#define MQ135_D0 D0  // Digital pin for threshold detection
#define RELAY_PIN D8  

// Ultrasonic for Water Level Monitoring
#define trigPinWater D3  
#define echoPinWater D4  
const float tankDepth = 22;  

// Ultrasonic for Bin Opening
#define trigPinBin D5  
#define echoPinBin D6  
#define servoPin D7  

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); 

BlynkTimer timer;
Servo servo;
bool binOpen = false;  

// 📌 Function to read air quality and send to Blynk
void sendAirQuality() {
    int airQualityValue = analogRead(MQ135_A0);
    int airThreshold = digitalRead(MQ135_D0);  // Read digital pin for threshold detection

    Blynk.virtualWrite(V1, airQualityValue); 

    Serial.print("Air Quality: ");
    Serial.print(airQualityValue);
    Serial.print(" | Threshold: ");
    Serial.println(airThreshold);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Air Quality:");
    lcd.setCursor(0, 1);
    lcd.print(airQualityValue);

    if (airQualityValue > 300 || airThreshold == HIGH) {  
        Serial.println("⚠ Poor Air Quality! Activating Relay!");
        Blynk.logEvent("gaz_detected", "Air Pollution Alert!");
        digitalWrite(RELAY_PIN, HIGH);
        lcd.setCursor(10, 1);
        lcd.print("BAD!");
    } else {
        digitalWrite(RELAY_PIN, LOW);
    }
}

// 📌 Function to measure water level and send to Blynk
void sendWaterLevel() {
    digitalWrite(trigPinWater, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPinWater, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinWater, LOW);
    
    long duration = pulseIn(echoPinWater, HIGH);
    float distance = duration * 0.034 / 2;
    float percentage = ((tankDepth - distance) / tankDepth) * 100;

    percentage = constrain(percentage, 0, 100);
    Blynk.virtualWrite(V0, percentage);

    Serial.print("Water Level: ");
    Serial.print(percentage);
    Serial.println("%");
}

// 📌 Function to measure distance for bin opening
int measureBinDistance() {
    digitalWrite(trigPinBin, LOW);
    delayMicroseconds(5);
    digitalWrite(trigPinBin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinBin, LOW);
    
    long duration = pulseIn(echoPinBin, HIGH);
    return duration * 0.034 / 2;
}

// 📌 Function to control bin opening
void checkBin() {
    int distance = measureBinDistance();
    Serial.print("Bin Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance <= 5 && !binOpen) {
        Serial.println("🔴 Object detected! Opening bin...");
        servo.attach(servoPin);
        servo.write(150);
        delay(1000);
        servo.detach();
        binOpen = true;
    } 
    else if (distance > 5 && binOpen) {
        Serial.println("🟢 No object detected. Closing bin...");
        servo.attach(servoPin);
        servo.write(0);
        delay(1000);
        servo.detach();
        binOpen = false;
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(MQ135_A0, INPUT);
    pinMode(MQ135_D0, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    pinMode(trigPinWater, OUTPUT);
    pinMode(echoPinWater, INPUT);

    pinMode(trigPinBin, OUTPUT);
    pinMode(echoPinBin, INPUT);
    servo.attach(servoPin);
    servo.write(0);
    delay(500);
    servo.detach();

    Wire.begin(D2, D1);  
    lcd.init();
    lcd.backlight();

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    
    timer.setInterval(2000L, sendAirQuality);
    timer.setInterval(1000L, sendWaterLevel);
    timer.setInterval(500L, checkBin);
}

void loop() {
    Blynk.run();
    timer.run();
}
