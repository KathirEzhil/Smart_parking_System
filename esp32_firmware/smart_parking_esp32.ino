/*
 * ============================================================
 *  Smart IoT Parking – ESP32 Firmware (Flask-integrated)
 * ============================================================
 *
 *  WHAT'S NEW vs. original:
 *  ─────────────────────────────────────────────────────────
 *  1. Connects to your WiFi network on startup.
 *  2. On every ENTRY or EXIT event, POSTs RFID data to:
 *       POST  http://<FLASK_IP>:5000/api/scan
 *       Body: {"rfid": "<UID>"}
 *  3. Whenever any slot IR sensor changes state, sends:
 *       POST  http://<FLASK_IP>:5000/api/slot_update
 *       Body: {"slot_id": N, "occupied": true/false}
 *  4. The Flask server handles all billing / DB recording.
 *     The ESP keeps doing local LCD + gate logic as before.
 *  ─────────────────────────────────────────────────────────
 *
 *  REQUIRED ARDUINO LIBRARIES (install via Library Manager):
 *    - MFRC522
 *    - ESP32Servo
 *    - LiquidCrystal_I2C
 *    - ArduinoJson  (v6+)
 *
 *  WiFi & HTTPClient are built into the ESP32 Arduino core.
 */

#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// =====================================================================
//  ★  CHANGE THESE TO MATCH YOUR SETUP  ★
// =====================================================================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// IP address of the laptop / PC running Flask (run `ipconfig` on Windows)
// Example: "192.168.1.12"
const char* FLASK_IP      = "192.168.X.X";
const int   FLASK_PORT    = 5000;
// =====================================================================

// ================= RFID =================
#define RFID_SS    5
#define RFID_RST   22
#define RFID_SCK   18
#define RFID_MISO  19
#define RFID_MOSI  23

// ================= IR SENSORS =================
#define ENTRY_IR_PIN 27
#define EXIT_IR_PIN  14

#define SLOT1_PIN    32
#define SLOT2_PIN    33
#define SLOT3_PIN    25
#define SLOT4_PIN    26

// ================= SERVO =================
#define GATE_SERVO_PIN 13

// ================= LCD =================
#define LCD_SDA 21
#define LCD_SCL 15
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= OBJECTS =================
MFRC522 rfid(RFID_SS, RFID_RST);
Servo   gateServo;

// ================= SETTINGS =================
bool irActiveLow = true;

byte validTag1[4] = {0xC5, 0x7C, 0x31, 0x07};
byte validTag2[4] = {0x3B, 0x22, 0x27, 0x07};

const int GATE_OPEN   = 90;
const int GATE_CLOSED = 0;

const int FEE_PER_SECOND = 1;

// ================= DISPLAY REFRESH CONTROL =================
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_REFRESH_INTERVAL = 1000;

bool rfidEventActive = false;
unsigned long rfidEventEnd = 0;

// ================= SLOT CHANGE TRACKING =================
// We track the previous state of each IR slot to send updates only when state CHANGES
bool prevSlot[4] = {false, false, false, false};
int  slotPins[4] = {SLOT1_PIN, SLOT2_PIN, SLOT3_PIN, SLOT4_PIN};

// ================= VEHICLE RECORD =================
struct VehicleRecord {
  byte uid[4];
  bool inside;
  unsigned long entryTime;
  unsigned long exitTime;
};

VehicleRecord vehicles[2];

// =====================================================================
//  HTTP HELPERS
// =====================================================================

/* Build the base URL prefix. */
String buildUrl(const char* path) {
  return String("http://") + FLASK_IP + ":" + FLASK_PORT + path;
}

/*
 * POST /api/scan
 * Call once on ENTRY and once on EXIT with the RFID UID string.
 * The Flask backend decides entry vs exit automatically.
 */
void notifyRFIDScan(const String& uidStr) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi not connected – skip scan notify");
    return;
  }

  HTTPClient http;
  String url = buildUrl("/api/scan");
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<64> doc;
  doc["rfid"] = uidStr;
  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code > 0) {
    String resp = http.getString();
    Serial.printf("[HTTP] /api/scan  → %d  %s\n", code, resp.c_str());
  } else {
    Serial.printf("[HTTP] /api/scan  ERROR: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

/*
 * POST /api/slot_update
 * slot_id  : 1-indexed (1…4)
 * occupied : true = car detected, false = empty
 */
void notifySlotUpdate(int slotId, bool occupied) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi not connected – skip slot update");
    return;
  }

  HTTPClient http;
  String url = buildUrl("/api/slot_update");
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<64> doc;
  doc["slot_id"]  = slotId;
  doc["occupied"] = occupied;
  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code > 0) {
    Serial.printf("[HTTP] /api/slot_update slot=%d occ=%d → %d\n", slotId, occupied, code);
  } else {
    Serial.printf("[HTTP] /api/slot_update ERROR: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

// =====================================================================
//  EXISTING HELPERS (unchanged)
// =====================================================================

bool sensorActive(int pin) {
  int val = digitalRead(pin);
  return irActiveLow ? (val == LOW) : (val == HIGH);
}

bool slotOccupied(int pin) { return sensorActive(pin); }

int countOccupiedSlots() {
  int occ = 0;
  if (slotOccupied(SLOT1_PIN)) occ++;
  if (slotOccupied(SLOT2_PIN)) occ++;
  if (slotOccupied(SLOT3_PIN)) occ++;
  if (slotOccupied(SLOT4_PIN)) occ++;
  return occ;
}

int countFreeSlots() { return 4 - countOccupiedSlots(); }

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

bool matchUID(byte *uid, byte size, byte *allowed) {
  if (size != 4) return false;
  for (byte i = 0; i < 4; i++) if (uid[i] != allowed[i]) return false;
  return true;
}

bool sameUID(byte *a, byte *b) {
  for (int i = 0; i < 4; i++) if (a[i] != b[i]) return false;
  return true;
}

bool isAuthorizedTag(byte *uid, byte size) {
  return matchUID(uid, size, validTag1) || matchUID(uid, size, validTag2);
}

int findVehicleRecord(byte *uid, byte size) {
  if (size != 4) return -1;
  for (int i = 0; i < 2; i++) if (sameUID(uid, vehicles[i].uid)) return i;
  return -1;
}

void initVehicleRecords() {
  for (int i = 0; i < 4; i++) {
    vehicles[0].uid[i] = validTag1[i];
    vehicles[1].uid[i] = validTag2[i];
  }
  vehicles[0].inside = vehicles[1].inside = false;
  vehicles[0].entryTime = vehicles[1].entryTime = 0;
  vehicles[0].exitTime  = vehicles[1].exitTime  = 0;
}

void lcdMessage(String a, String b) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(a.substring(0, 16));
  lcd.setCursor(0, 1); lcd.print(b.substring(0, 16));
}

void showParkingStatus() {
  int freeSlots = countFreeSlots();
  int occSlots  = countOccupiedSlots();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Free:"); lcd.print(freeSlots);
  lcd.print(" Occ:");  lcd.print(occSlots);
  lcd.setCursor(0, 1);
  lcd.print("S1:"); lcd.print(slotOccupied(SLOT1_PIN) ? "O" : "F");
  lcd.print("S2:"); lcd.print(slotOccupied(SLOT2_PIN) ? "O" : "F");
  lcd.print("S3:"); lcd.print(slotOccupied(SLOT3_PIN) ? "O" : "F");
  lcd.print("S4:"); lcd.print(slotOccupied(SLOT4_PIN) ? "O" : "F");
}

void openGateOnly() {
  gateServo.write(GATE_OPEN);
  delay(3000);
  gateServo.write(GATE_CLOSED);
}

bool readRFIDCard(String &tag, byte *uidBuf, byte &uidSize) {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial())   return false;
  uidSize = rfid.uid.size;
  for (byte i = 0; i < uidSize; i++) uidBuf[i] = rfid.uid.uidByte[i];
  tag = uidToString(uidBuf, uidSize);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

String formatDuration(unsigned long ms) {
  unsigned long totalSec = ms / 1000;
  unsigned long minutes  = totalSec / 60;
  unsigned long seconds  = totalSec % 60;
  return String(minutes) + "m " + String(seconds) + "s";
}

void startRfidEvent(unsigned long blockMs) {
  rfidEventActive = true;
  rfidEventEnd    = millis() + blockMs;
}

// =====================================================================
//  SLOT CHANGE DETECTOR  –  sends HTTP only on state change
// =====================================================================
void checkAndSendSlotUpdates() {
  for (int i = 0; i < 4; i++) {
    bool curr = slotOccupied(slotPins[i]);
    if (curr != prevSlot[i]) {
      prevSlot[i] = curr;
      Serial.printf("[SLOT] Slot %d changed → %s\n", i + 1, curr ? "OCCUPIED" : "FREE");
      notifySlotUpdate(i + 1, curr);  // slot_id is 1-indexed
    }
  }
}

// =====================================================================
//  RFID HANDLER  –  now also calls notifyRFIDScan()
// =====================================================================
void handleRFID() {
  byte   uidBuf[10];
  byte   uidSize = 0;
  String tag     = "";

  if (!readRFIDCard(tag, uidBuf, uidSize)) return;

  bool authorized = isAuthorizedTag(uidBuf, uidSize);
  bool entryCar   = sensorActive(ENTRY_IR_PIN);
  bool exitCar    = sensorActive(EXIT_IR_PIN);
  int  freeSlots  = countFreeSlots();

  Serial.println("RFID scanned: " + tag);

  // ---------- Unauthorized ----------
  if (!authorized) {
    Serial.println("Denied: invalid RFID");
    startRfidEvent(2500);
    lcdMessage("Access Denied", "Invalid RFID");
    delay(2000);
    return;
  }

  int rec = findVehicleRecord(uidBuf, uidSize);
  if (rec == -1) {
    Serial.println("Denied: no record found");
    startRfidEvent(2500);
    lcdMessage("Access Denied", "UID Not Stored");
    delay(2000);
    return;
  }

  // ---------- Both gates active ----------
  if (entryCar && exitCar) {
    Serial.println("Denied: both sensors active");
    startRfidEvent(2500);
    lcdMessage("Error", "Both Gates Actv");
    delay(2000);
    return;
  }

  // ---------- ENTRY ----------
  if (entryCar && !exitCar) {
    if (vehicles[rec].inside) {
      Serial.println("Denied: already inside");
      startRfidEvent(2500);
      lcdMessage("Entry Denied", "Already Inside");
      delay(2000);
      return;
    }
    if (freeSlots <= 0) {
      Serial.println("Denied: parking full");
      startRfidEvent(2500);
      lcdMessage("Entry Denied", "Parking Full");
      delay(2000);
      return;
    }

    vehicles[rec].inside    = true;
    vehicles[rec].entryTime = millis();
    vehicles[rec].exitTime  = 0;

    Serial.println("ENTRY ALLOWED – notifying Flask");
    notifyRFIDScan(tag);                     // ← HTTP POST to Flask

    startRfidEvent(4000);
    lcdMessage("Entry Allowed", "Gate Opening...");
    openGateOnly();
    return;
  }

  // ---------- EXIT ----------
  if (exitCar && !entryCar) {
    if (!vehicles[rec].inside) {
      Serial.println("Denied: no entry record");
      startRfidEvent(2500);
      lcdMessage("Exit Denied", "No Entry Record");
      delay(2000);
      return;
    }

    vehicles[rec].exitTime  = millis();
    unsigned long parkedMs  = vehicles[rec].exitTime - vehicles[rec].entryTime;
    unsigned long parkedSec = parkedMs / 1000;
    if (parkedSec == 0) parkedSec = 1;
    unsigned long fee       = parkedSec * FEE_PER_SECOND;

    Serial.println("EXIT ALLOWED – notifying Flask");
    Serial.printf("Duration: %s | Fee: Rs.%lu\n", formatDuration(parkedMs).c_str(), fee);

    notifyRFIDScan(tag);                     // ← HTTP POST to Flask (Flask recalculates fee)

    startRfidEvent(9000);
    lcdMessage("Exit Allowed", formatDuration(parkedMs));
    delay(2000);
    lcdMessage("Parking Fee", "Rs." + String(fee));
    delay(2500);
    lcdMessage("Gate Opening", "Thank You!");
    openGateOnly();

    vehicles[rec].inside = false;
    return;
  }

  // ---------- No car at gate ----------
  Serial.println("No car at gate");
  startRfidEvent(2500);
  lcdMessage("No Car At Gate", "Scan Again");
  delay(2000);
}

// =====================================================================
//  SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);

  // IR sensors
  pinMode(ENTRY_IR_PIN, INPUT_PULLUP);
  pinMode(EXIT_IR_PIN,  INPUT_PULLUP);
  for (int i = 0; i < 4; i++) pinMode(slotPins[i], INPUT_PULLUP);

  // SPI / RFID
  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI);
  rfid.PCD_Init();

  // Servo
  gateServo.attach(GATE_SERVO_PIN);
  gateServo.write(GATE_CLOSED);

  // LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  initVehicleRecords();

  // ---- WiFi ----
  lcdMessage("Connecting WiFi", WIFI_SSID);
  Serial.printf("[WiFi] Connecting to %s …\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    lcdMessage("WiFi Connected", WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] FAILED – continuing offline");
    lcdMessage("WiFi FAILED", "Offline Mode");
  }
  delay(1500);

  // Seed initial slot states to Flask
  for (int i = 0; i < 4; i++) {
    prevSlot[i] = slotOccupied(slotPins[i]);
    notifySlotUpdate(i + 1, prevSlot[i]);
  }

  lcdMessage("Smart Parking", "System Ready");
  delay(1000);
  showParkingStatus();
  Serial.println("[SYSTEM] Ready.");
}

// =====================================================================
//  LOOP
// =====================================================================
void loop() {
  // Expire RFID event lock
  if (rfidEventActive && millis() >= rfidEventEnd) {
    rfidEventActive = false;
  }

  // Auto-refresh LCD every second when no RFID message is showing
  if (!rfidEventActive && (millis() - lastDisplayUpdate >= DISPLAY_REFRESH_INTERVAL)) {
    showParkingStatus();
    lastDisplayUpdate = millis();
  }

  // Check slot IR changes and push to Flask if changed
  checkAndSendSlotUpdates();

  // Listen for RFID scans
  handleRFID();
}
