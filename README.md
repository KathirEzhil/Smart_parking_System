# Smart IoT Parking Management System

## Overview
This is a production-quality Flask web application meant to manage an IoT-based smart parking system.
At present, everything functions using simulation routes, making it easy to test without the actual ESP32 hardware connected.

## Prerequisites
- Python 3.8+

## Setup & Execution

1. Navigate to the project directory:
   ```bash
   cd smart_parking
   ```

2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

3. Run the application:
   ```bash
   python app.py
   ```

## Using the Application
Open your web browser and go to `http://localhost:5000/`.

- **Dashboard:** `http://localhost:5000/dashboard` - Monitor real-time status.
- **Testing/Simulation:** `http://localhost:5000/testing` - Use to test entry and exit of vehicles without hardware.
- **Admin:** `http://localhost:5000/admin` - View logs and system revenue.
- **Analytics:** `http://localhost:5000/analytics` - Check trends and visual data.

## API Documentation (for IoT/ESP32)
* **POST `/api/entry`** - Payload `{"rfid": "str"}`
* **POST `/api/exit`** - Payload `{"rfid": "str"}`
* **POST `/api/slot_update`** - Payload `{"slot": int, "status": "occupied"|"free"}`
* **GET `/api/status`** - Retrieves parking status summary.
