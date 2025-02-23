#define BLYNK_TEMPLATE_ID "TMPL3o34QEZZf"
#define BLYNK_TEMPLATE_NAME "Smart Classroom"
#define BLYNK_AUTH_TOKEN "qSk0ysoLGAyQEWk7ouVQVvFEZZndlUNl"

#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>

#define BLYNK_PRINT Serial

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "vivo Y35";
char pass[] = "24262426";

WiFiClient client;  // WiFi client to connect to ThingSpeak
unsigned long myChannelNumber = 2748636;  // Replace with your ThingSpeak channel number
const char *myWriteAPIKey = "GTO1VLJK5UO7AB6V";  // Replace with your ThingSpeak Write API Key

int soilMoistureSensor = A0;
int pump_relay = D3;
int WET = D1;
int DRY = D2;
int PIRSensor = D5;
int Motor_Relay = D6;
int led = D8;
int flameSensor = D4;
int buzzer = D0;

// Flags for manual control
bool pumpControl = false;
bool motorControl = false;
bool lightControl = false;

bool flameAlertSent = false; // Flag to avoid multiple notifications

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  ThingSpeak.begin(client);

  pinMode(soilMoistureSensor, INPUT);
  pinMode(pump_relay, OUTPUT);
  pinMode(WET, OUTPUT);
  pinMode(DRY, OUTPUT);
  pinMode(PIRSensor, INPUT);
  pinMode(Motor_Relay, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(flameSensor, INPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(pump_relay, HIGH);
  digitalWrite(Motor_Relay, HIGH);
  digitalWrite(WET, LOW);
  digitalWrite(DRY, LOW);
  digitalWrite(led, LOW);
  digitalWrite(buzzer, LOW);
}

BLYNK_CONNECTED() {
  // Sync the state of virtual pins with the app on device reconnect
  Blynk.syncVirtual(V1, V2, V3);
}

BLYNK_WRITE(V1) {
  pumpControl = param.asInt();
  digitalWrite(pump_relay, pumpControl ? LOW : HIGH);
}

BLYNK_WRITE(V2) {
  motorControl = param.asInt();
  digitalWrite(Motor_Relay, motorControl ? LOW : HIGH);
}

BLYNK_WRITE(V3) {
  lightControl = param.asInt();
  digitalWrite(led, lightControl ? HIGH : LOW);
}

void loop() {
  Blynk.run();

  // Flame Sensor
  int flameDetected = digitalRead(flameSensor);
  digitalWrite(buzzer, flameDetected == HIGH ? LOW : HIGH);

  if (flameDetected == LOW && !flameAlertSent) {
    // Send alert via Blynk event
    Blynk.logEvent("flame_alert", "🔥 Flame detected! Take immediate action!");
    flameAlertSent = true; // Avoid multiple notifications
  } else if (flameDetected == HIGH) {
    flameAlertSent = false; // Reset flag when no flame is detected
  }

  // Soil Moisture Sensor
  int moistureLevel = analogRead(soilMoistureSensor);
  Blynk.virtualWrite(V0, moistureLevel); // Send moisture level to Blynk

  ThingSpeak.setField(1, moistureLevel);  // Send moisture data to Field 1
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);  // Update ThingSpeak

  if (!pumpControl) { // Auto-control pump if not manually controlled
    if (moistureLevel < 900) {
      digitalWrite(pump_relay, HIGH); // Turn off pump
      digitalWrite(WET, HIGH);
      digitalWrite(DRY, LOW);
      Blynk.virtualWrite(V1, 0); // Reflect OFF state in Blynk
    } else {
      digitalWrite(pump_relay, LOW); // Turn on pump
      digitalWrite(WET, LOW);
      digitalWrite(DRY, HIGH);
      Blynk.virtualWrite(V1, 1); // Reflect ON state in Blynk
    }
  }

  // PIR Sensor Motion Detection
  bool motionDetected = digitalRead(PIRSensor);
  if (!lightControl) { // Auto-control LED if not manually controlled
    if (motionDetected) {
      digitalWrite(led, HIGH);
      Blynk.virtualWrite(V3, 1); // Reflect ON state in Blynk
    } else {
      digitalWrite(led, LOW);
      Blynk.virtualWrite(V3, 0); // Reflect OFF state in Blynk
    }
  }

  if (!motorControl) { // Auto-control motor if not manually controlled
    if (motionDetected) {
      digitalWrite(Motor_Relay, LOW); // Turn on motor
      Blynk.virtualWrite(V2, 1); // Reflect ON state in Blynk
    } else {
      digitalWrite(Motor_Relay, HIGH); // Turn off motor
      Blynk.virtualWrite(V2, 0); // Reflect OFF state in Blynk
    }
  }

  delay(500); // For smoother readings
}
