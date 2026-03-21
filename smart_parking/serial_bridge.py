import serial
import requests
import time
import sys

# ==========================================
# CONFIGURATION
# ==========================================
# Change this to match your Arduino IDE port (e.g. "COM3", "COM21")
SERIAL_PORT = "COM21" 
BAUD_RATE   = 115200
FLASK_URL   = "http://127.0.0.1:5000"
# ==========================================

def start_bridge():
    print(f"--- Smart Parking Serial Bridge ---")
    print(f"Connecting to {SERIAL_PORT} at {BAUD_RATE} baud...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2) # Wait for ESP32 reboot
        print(f"SUCCESS: Bridge is active. Listening for events...")
    except Exception as e:
        print(f"FAILED to open serial port: {e}")
        print("Check if Arduino Serial Monitor is OPEN (it must be closed for this to work).")
        return

    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                # Format: [SCAN]CARD_UID
                if line.startswith("[SCAN]"):
                    uid = line.replace("[SCAN]", "").strip()
                    print(f"Detected RFID Scan: {uid} -> Forwarding to Flask...")
                    try:
                        resp = requests.post(f"{FLASK_URL}/api/scan", json={"rfid": uid})
                        print(f"   [Flask Response] {resp.status_code}: {resp.text}")
                    except Exception as e:
                        print(f"   [Flask Error] Connection failed: {e}")
                
                # Format: [SLOT]ID,STATUS (STATUS 1=OCCUPIED, 0=FREE)
                elif line.startswith("[SLOT]"):
                    content = line.replace("[SLOT]", "").strip()
                    try:
                        parts = content.split(",")
                        slot_id = int(parts[0])
                        occupied = bool(int(parts[1]))
                        status_str = "Occupied" if occupied else "Free"
                        
                        print(f"Detected Slot Change: Slot {slot_id} is now {status_str} -> Forwarding to Flask...")
                        resp = requests.post(f"{FLASK_URL}/api/slot_update", json={
                            "slot_id": slot_id,
                            "occupied": occupied
                        })
                        print(f"   [Flask Response] {resp.status_code}")
                    except Exception as e:
                        print(f"   [Parsing/Flask Error] {e}")
                
                else:
                    # Print normal serial debug from ESP32
                    print(f"[ESP32] {line}")
                    
        except KeyboardInterrupt:
            print("\nShutting down bridge...")
            ser.close()
            sys.exit()
        except Exception as e:
            print(f"Error in loop: {e}")
            time.sleep(1)

if __name__ == "__main__":
    start_bridge()
