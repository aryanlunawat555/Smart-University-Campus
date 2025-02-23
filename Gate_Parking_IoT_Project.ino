#define BLYNK_TEMPLATE_ID "TMPL376McIYI9"
#define BLYNK_TEMPLATE_NAME "Board1"
#define BLYNK_AUTH_TOKEN "gO8N55BcwPtfmexfu2e1-jDYoQcIXpap"

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// RFID pins and servo
#define RST_PIN         D3    
#define SS_PIN          D4    
#define SERVO_PIN       D1    

// IR sensor pins
#define IR_SENSOR1_PIN  D0 
#define IR_SENSOR2_PIN  D2 
#define IR_SENSOR3_PIN  D8

// Create instances
MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;

// Valid RFID card UID (replace with your actual card UID)
byte validUID[4] = {0xA1, 0x94, 0x1C, 0x1B}; 

// Blynk credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "vivo Y35";
char pass[] = "24262426";

// Parking status variables
bool parkingSpot1Occupied = false;
bool parkingSpot2Occupied = false;
bool parkingSpot3Occupied = false;
int availableSpots = 3;

// Gate control variables
bool doorLocked = true;
unsigned long unlockTime = 0;
const unsigned long AUTO_LOCK_DELAY = 2000;
bool manualUnlock = false;

void connectToWiFi() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to Wi-Fi");
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi!");
  } else {
    Serial.println("Failed to connect to Wi-Fi.");
  }
}

void connectToBlynk() {
  Blynk.config(auth);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecting to Blynk...");
    Blynk.connect();
    int retryCount = 0;
    while (!Blynk.connected() && retryCount < 20) {
      delay(500);
      Serial.print(".");
      retryCount++;
    }
    if (Blynk.connected()) {
      Serial.println("Connected to Blynk!");
    } else {
      Serial.println("Failed to connect to Blynk.");
    }
  }
}

void setup() {
  Serial.begin(115200);

  // RFID and Servo initialization
  SPI.begin();
  rfid.PCD_Init();
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  // Initialize IR sensor pins
  pinMode(IR_SENSOR1_PIN, INPUT);
  pinMode(IR_SENSOR2_PIN, INPUT);
  pinMode(IR_SENSOR3_PIN, INPUT);

  Serial.println("Initializing...");

  connectToWiFi();
  connectToBlynk();

  Blynk.virtualWrite(V6, "Locked");

  Serial.println("Place RFID card and monitor parking spots.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  if (!Blynk.connected()) {
    connectToBlynk();
  }
  
  Blynk.run();
  checkParkingSpots();

  if (!doorLocked && !manualUnlock && (millis() - unlockTime >= AUTO_LOCK_DELAY)) {
    lockDoor();
  }
  if (doorLocked && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (checkUID()) {
      unlockDoor();
      Blynk.virtualWrite(V1, 1); 
    }
    rfid.PICC_HaltA();
  }
}

void checkParkingSpots() {
  parkingSpot1Occupied = digitalRead(IR_SENSOR1_PIN) == LOW;
  parkingSpot2Occupied = digitalRead(IR_SENSOR2_PIN) == LOW;
  parkingSpot3Occupied = digitalRead(IR_SENSOR3_PIN) == LOW;

  availableSpots = 3 - (parkingSpot1Occupied + parkingSpot2Occupied + parkingSpot3Occupied);

  Blynk.virtualWrite(V2, parkingSpot1Occupied ? 1 : 0);
  Blynk.virtualWrite(V3, parkingSpot2Occupied ? 1 : 0);
  Blynk.virtualWrite(V4, parkingSpot3Occupied ? 1 : 0);
  Blynk.virtualWrite(V5, availableSpots);
}

bool checkUID() {
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != validUID[i]) return false;
  }
  return true;
}

void unlockDoor() {
  myServo.write(90);
  doorLocked = false;
  unlockTime = millis();
  Blynk.virtualWrite(V6, "Unlocked");
}

void lockDoor() {
  myServo.write(0);
  doorLocked = true;
  Blynk.virtualWrite(V6, "Locked");
  Blynk.virtualWrite(V1, 0);
}

BLYNK_WRITE(V1) {
  manualUnlock = param.asInt();
  if (manualUnlock) {
    unlockDoor();
  } else {
    lockDoor();
  }
}
