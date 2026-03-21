/*
 * ============================================================
 *  Smart IoT Parking – ESP32 Firmware (SERIAL BRIDGE VERSION)
 * ============================================================
 *
 *  DIFFICULTY-FREE INTEGRATION:
 *  ─────────────────────────────────────────────────────────
 *  This version does NOT use WiFi or HTTP directly.
 *  Instead, it sends "Tags" over the USB Serial cable.
 *  A Python script on your laptop (serial_bridge.py) reads 
 *  these tags and updates your Flask dashboard instantly.
 *
 *  REQUIRED ARDUINO LIBRARIES:
 *    - MFRC522
 *    - ESP32Servo
 *    - LiquidCrystal_I2C (by Frank de Brabander)
 *  ─────────────────────────────────────────────────────────
 */

#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= PIN DEFINITIONS =================
#define RFID_SS    5
#define RFID_RST   22
#define RFID_SCK   18
#define RFID_MISO  19
#define RFID_MOSI  23

#define ENTRY_IR_PIN 27
#define EXIT_IR_PIN  14

#define SLOT1_PIN    32
#define SLOT2_PIN    33
#define SLOT3_PIN    25
#define SLOT4_PIN    26

#define GATE_SERVO_PIN 13

#define LCD_SDA 21
#define LCD_SCL 15

// ================= OBJECTS =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(RFID_SS, RFID_RST);
Servo   gateServo;

// ================= SETTINGS =================
bool irActiveLow = true; 

// Valid RFID Tags (Hex)
byte validTag1[4] = {0xC5, 0x7C, 0x31, 0x07};
byte validTag2[4] = {0x3B, 0x22, 0x27, 0x07};
byte validTag3[4] = {0x67, 0x49, 0xF9, 0x06};
byte validTag4[4] = {0x29, 0xF3, 0xFD, 0x06};

const int GATE_OPEN   = 90;
const int GATE_CLOSED = 0;

// Local Billing (Simulated)
const int FEE_PER_SECOND = 1;

// ================= STATE TRACKING =================
bool prevSlot[4] = {false, false, false, false};
int  slotPins[4] = {SLOT1_PIN, SLOT2_PIN, SLOT3_PIN, SLOT4_PIN};

struct VehicleRecord {
  byte uid[4];
  bool inside;
  unsigned long entryTime;
};
VehicleRecord vehicles[4];

unsigned long lastDisplayUpdate = 0;
bool rfidEventActive = false;
unsigned long rfidEventEnd = 0;

// =====================================================================
//  BRIDGE HELPERS (The secret sauce)
// =====================================================================

void bridgeNotifyScan(String uidStr) {
  // Sending this tag tells the Python script to call /api/scan
  Serial.print("[SCAN]");
  Serial.println(uidStr);
}

void bridgeNotifySlot(int slotId, bool occupied) {
  // Sending this tag tells the Python script to call /api/slot_update
  Serial.print("[SLOT]");
  Serial.print(slotId);
  Serial.print(",");
  Serial.println(occupied ? "1" : "0");
}

// =====================================================================
//  HARDWARE HELPERS
// =====================================================================

bool sensorActive(int pin) {
  return irActiveLow ? (digitalRead(pin) == LOW) : (digitalRead(pin) == HIGH);
}

int countFreeSlots() {
  int freeCount = 0;
  for(int i=0; i<4; i++) if(!sensorActive(slotPins[i])) freeCount++;
  return freeCount;
}

String uidToString(byte *uid, byte size) {
  String s = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
    if (i < size - 1) s += ":";
  }
  s.toUpperCase();
  return s;
}

bool sameUID(byte *a, byte *b) {
  for (int i = 0; i < 4; i++) if (a[i] != b[i]) return false;
  return true;
}

int findCardIndex(byte *uid) {
  if (sameUID(uid, validTag1)) return 0;
  if (sameUID(uid, validTag2)) return 1;
  if (sameUID(uid, validTag3)) return 2;
  if (sameUID(uid, validTag4)) return 3;
  return -1;
}

void lcdMessage(String a, String b) {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(a);
  lcd.setCursor(0,1); lcd.print(b);
}

void showStatus() {
  int f = countFreeSlots();
  lcd.clear(); 
  lcd.setCursor(0,0); lcd.print("Free Slots: "); lcd.print(f);
  lcd.setCursor(0,1);
  for(int i=0; i<4; i++) {
    lcd.print(sensorActive(slotPins[i]) ? "O" : "F");
    if(i < 3) lcd.print(" ");
  }
}

// =====================================================================
//  CORE LOGIC
// =====================================================================

void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  byte uid[4];
  for (byte i = 0; i < 4; i++) uid[i] = rfid.uid.uidByte[i];
  String uidStr = uidToString(uid, 4);
  int idx = findCardIndex(uid);

  if (idx == -1) {
    lcdMessage("Access Denied", "Invalid Card");
    delay(2000);
    return;
  }

  bool entryReq = sensorActive(ENTRY_IR_PIN);
  bool exitReq  = sensorActive(EXIT_IR_PIN);

  if (entryReq && !vehicles[idx].inside) {
    if (countFreeSlots() > 0) {
      vehicles[idx].inside = true;
      vehicles[idx].entryTime = millis();
      lcdMessage("Entry Allowed", "Welcome!");
      bridgeNotifyScan(uidStr); // BRIDGE CALL
      gateServo.write(GATE_OPEN); delay(3000); gateServo.write(GATE_CLOSED);
    } else {
      lcdMessage("Rejected", "Parking Full");
      delay(2000);
    }
  } 
  else if (exitReq && vehicles[idx].inside) {
    unsigned long durationSec = (millis() - vehicles[idx].entryTime) / 1000;
    int fee = max(1, (int)durationSec * FEE_PER_SECOND);
    
    lcdMessage("Exit Allowed", "Fee: Rs." + String(fee));
    bridgeNotifyScan(uidStr); // BRIDGE CALL
    delay(2500);
    gateServo.write(GATE_OPEN); delay(3000); gateServo.write(GATE_CLOSED);
    vehicles[idx].inside = false;
  }
  else {
    lcdMessage("No Car Detected", "Move to Sensor");
    delay(2000);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  rfidEventActive = true;
  rfidEventEnd = millis() + 500;
}

void setup() {
  Serial.begin(115200);
  
  // Pins
  pinMode(ENTRY_IR_PIN, INPUT_PULLUP);
  pinMode(EXIT_IR_PIN,  INPUT_PULLUP);
  for(int i=0; i<4; i++) pinMode(slotPins[i], INPUT_PULLUP);

  // Peripherals
  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI);
  rfid.PCD_Init();
  gateServo.attach(GATE_SERVO_PIN);
  gateServo.write(GATE_CLOSED);
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init(); lcd.backlight();

  lcdMessage("Smart Parking", "System Loading...");
  delay(1000);
  
  // Initial sync of slots to Bridge
  for(int i=0; i<4; i++) {
    prevSlot[i] = sensorActive(slotPins[i]);
    bridgeNotifySlot(i+1, prevSlot[i]);
  }
  
  Serial.println("[SYSTEM] Serial Bridge Mode Active.");
}

void loop() {
  if (rfidEventActive && millis() > rfidEventEnd) rfidEventActive = false;

  // Update LCD every 1s
  if (!rfidEventActive && (millis() - lastDisplayUpdate > 1000)) {
    showStatus();
    lastDisplayUpdate = millis();
  }

  // Check Slot Changes
  for(int i=0; i<4; i++) {
    bool curr = sensorActive(slotPins[i]);
    if (curr != prevSlot[i]) {
      prevSlot[i] = curr;
      bridgeNotifySlot(i+1, curr); // BRIDGE CALL
    }
  }

  handleRFID();
}
