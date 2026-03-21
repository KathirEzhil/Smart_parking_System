# Smart IoT Parking Management System

![Dashboard Preview](https://via.placeholder.com/1200x600?text=Smart+IoT+Parking+Dashboard) *(Optional: Replace with actual project screenshot)*

## Overview
The **Smart IoT Parking Management System** is a robust, production-quality web application built with Flask to manage, monitor, and analyze a real-time IoT-based smart parking ecosystem. By bridging physical hardware (like ESP32 microcontrollers and RFID readers) with a dynamic web dashboard, this system provides administrators and users with a seamless parking experience, including live tracking, revenue generation, and historical analytics.

Currently, the system supports built-in simulation routes, allowing for end-to-end testing and demonstration without requiring the physical hardware components to be connected.

## Key Features
* **Live Parking Layout:** A dynamically responsive grid view that reflects the real-time occupancy status of individual parking slots.
* **Role-Based Access Control:** Separate dashboards and permissions for System Administrators (monitoring, revenue, logs) and Users (personal history, active session tracking).
* **Comprehensive Analytics:** Visual representations of parking demand by the hour, peak hours, turnover rates, and total revenue metrics.
* **Hardware Integration Ready:** RESTful APIs designed to communicate seamlessly with ESP32-based hardware for physical entry/exit gates and slot updates.
* **Simulation Mode:** Built-in web routes to test vehicle entries and exits programmatically.

---

## Architecture & Technology Stack
* **Backend Framework:** Python / Flask
* **Database:** SQLite (via SQLAlchemy ORM)
* **Frontend:** HTML5, CSS3, Vanilla JavaScript, Chart.js (for analytics visuals)
* **IoT Hardware Core (Optional):** ESP32, RFID Readers (RC522), IR/Ultrasonic Sensors

---

## Prerequisites
* Python 3.8+
* `pip` package manager

---

## Installation & Setup

1. **Clone the repository and navigate to the project directory:**
   ```bash
   git clone https://github.com/KathirEzhil/Smart_parking_System.git
   cd Smart_parking_System/smart_parking
   ```

2. **Create a virtual environment (optional but recommended):**
   ```bash
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\\Scripts\\activate
   ```

3. **Install the required dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

4. **Initialize the application and database:**
   Upon first execution, the system automatically provisions the SQLite database, configures the predefined slots based on the `config.py` settings, and seeds initial demo data (including user accounts).
   ```bash
   python app.py
   ```

---

## Usage Guide

Once the server is running, the application will be accessible at `http://localhost:5000/`.

### Default Credentials
* **Admin Login:** Username: `admin` | Password: `adminpassword`
* **User Login:** Username: `user1` | Password: `password` (Associated test RFID: `CAR101`)

### Application Routes
* `/dashboard` - Overview of real-time slot occupancy and active parking session details.
* `/analytics` - Data visualization covering entry trends, turnover, and revenue metrics.
* `/admin` - Administrative interface for viewing detailed session logs and aggregated system revenue.
* `/testing` - Interface for simulating vehicle entries and exits without requiring hardware.

---

## Hardware Integration API

For connecting ESP32 or other IoT endpoints, the application provides the following RESTful API endpoints. Ensure that your microcontroller sends data with the `Content-Type: application/json` header.

| Endpoint             | Method | Payload Example                         | Description |
| :------------------- | :----- | :-------------------------------------- | :---------- |
| `/api/entry`         | `POST` | `{"rfid": "CAR101"}`                    | Registers a vehicle entry, initiates billing timeline. |
| `/api/exit`          | `POST` | `{"rfid": "CAR101"}`                    | Registers a vehicle exit, concludes billing and updates logs. |
| `/api/slot_update`   | `POST` | `{"slot": 1, "status": "occupied"}`     | Updates the real-time physical status of a specific slot. |
| `/api/status`        | `GET`  | *None*                                  | Retrieves a summary of total slots, occupied, and available counts. |
| `/api/slots`         | `GET`  | *None*                                  | Used by the frontend to poll and render the live parking layout grid. |

---

## Developer Project Structure

```text
smart_parking/
├── app.py                 # Core application entry point & setup logic
├── config.py              # Environment and system configurations
├── requirements.txt       # Python dependency list
├── serial_bridge.py       # (Optional) Serial USB to Flask HTTP bridge
├── models/                # SQLAlchemy database schema models
├── routes/                # Blueprint routing (Admin, User, Views, Auth)
├── api/                   # REST APIs for direct hardware & frontend communication
├── static/                # CSS styling, JavaScript mapping, and Chart.js code
└── templates/             # Jinja2 HTML rendering templates
```
