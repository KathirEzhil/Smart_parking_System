from flask import Blueprint, request, jsonify
from datetime import datetime
from models import db
from models.parking_model import ParkingLog, Slot, SystemState
from pricing import calculate_duration_and_bill

parking_bp = Blueprint('parking_api', __name__, url_prefix='/api')

@parking_bp.route('/scan', methods=['POST'])
def scan_rfid():
    """
    Single Endpoint for ESP32 RFID Scans.
    If RFID has an active session -> Handle as EXIT.
    If RFID has no active session -> Handle as ENTRY.
    """
    data = request.get_json()
    if not data or 'rfid' not in data:
        return jsonify({'error': 'Missing RFID data'}), 400
        
    rfid = data['rfid']
    
    # Check for active session
    active_log = ParkingLog.query.filter_by(rfid=rfid, status='active').first()
    
    state = SystemState.query.first()
    if not state:
        state = SystemState()
        db.session.add(state)
        
    if active_log:
        # HANDLING EXIT
        exit_time = datetime.now()
        duration_minutes, bill_amount = calculate_duration_and_bill(active_log.entry_time, exit_time)
        
        active_log.exit_time = exit_time
        active_log.duration_minutes = duration_minutes
        active_log.bill_amount = bill_amount
        active_log.status = 'completed'
        
        # Increase revenue
        state.total_revenue += bill_amount
        
        db.session.commit()
        
        return jsonify({
            'message': 'Exit recorded successfully.',
            'action': 'EXIT',
            'rfid': rfid,
            'duration_minutes': duration_minutes,
            'bill_amount': bill_amount
        }), 200

    else:
        # HANDLING ENTRY
        # Note: We decouple the slot_id because the hardware IR sensor will handle grid detection natively.
        new_log = ParkingLog(
            rfid=rfid,
            entry_time=datetime.now(),
            status='active',
            slot_id=None 
        )
        
        db.session.add(new_log)
        db.session.commit()
        
        return jsonify({
            'message': 'Entry recorded successfully.',
            'action': 'ENTRY',
            'rfid': rfid,
            'entry_time': new_log.entry_time.isoformat()
        }), 201
