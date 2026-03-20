from flask import Blueprint, request, jsonify
from models import db
from models.parking_model import Slot, SystemState
from datetime import datetime

slot_bp = Blueprint('slot_api', __name__, url_prefix='/api')

@slot_bp.route('/slot_update', methods=['POST'])
def slot_update():
    """
    Accepts real-time IR sensor updates from ESP32.
    Payload: { "slot_id": 1, "occupied": true }
    """
    data = request.get_json() or {}

    # Support both key conventions for robustness
    slot_id    = data.get('slot_id') or data.get('slot')
    occupied   = data.get('occupied')

    # Also allow string "occupied"/"free" as a fallback
    if occupied is None:
        status_str = data.get('status', '')
        occupied   = (status_str.lower() == 'occupied')

    if slot_id is None or occupied is None:
        return jsonify({'error': 'Missing slot_id or occupied field'}), 400

    slot = Slot.query.get(slot_id)
    if not slot:
        return jsonify({'error': f'Slot {slot_id} not found'}), 404

    slot.occupied    = bool(occupied)
    slot.last_update = datetime.now()

    # Keep system-wide counts in sync
    state = SystemState.query.first()
    if state:
        all_slots          = Slot.query.all()
        state.occupied_slots  = sum(1 for s in all_slots if s.occupied)
        state.available_slots = state.total_slots - state.occupied_slots

    db.session.commit()

    return jsonify({
        'status':   'success',
        'slot_id':  slot_id,
        'occupied': slot.occupied
    }), 200


@slot_bp.route('/status', methods=['GET'])
def system_status():
    state = SystemState.query.first()
    if not state:
        return jsonify({'error': 'System uninitialized'}), 500

    return jsonify({
        'total_slots': state.total_slots,
        'occupied':    state.occupied_slots,
        'available':   state.available_slots,
        'revenue':     state.total_revenue
    }), 200


@slot_bp.route('/slots', methods=['GET'])
def get_slots():
    slots = Slot.query.order_by(Slot.slot_id).all()
    return jsonify({
        'slots': [
            {'slot_id': s.slot_id, 'occupied': s.occupied, 'rfid': s.rfid}
            for s in slots
        ]
    }), 200
