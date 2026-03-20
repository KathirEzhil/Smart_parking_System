from flask import Blueprint, current_app, redirect, flash, request, jsonify, url_for

simulation_bp = Blueprint('simulation_routes', __name__)

@simulation_bp.route('/simulate_scan/<rfid>')
def simulate_scan(rfid):
    """
    Simulates a scan by sending a mock POST request to the /api/scan endpoint internally.
    """
    with current_app.test_client() as client:
        response = client.post('/api/scan', json={'rfid': rfid})
        data = response.get_json()
        
        if response.status_code in [200, 201]:
            action = data.get('action')
            if action == 'ENTRY':
                flash(f"Simulation: Car {rfid} Entered.", 'success')
            elif action == 'EXIT':
                flash(f"Simulation: Car {rfid} Exited. Bill: ₹{data.get('bill_amount')}", 'success')
        else:
            flash(f"Simulation Error: {data.get('error', 'Unknown Error')}", 'error')
            
    return redirect(url_for('admin_routes.testing_view'))

@simulation_bp.route('/simulate_slot/<int:slot_id>/<status>')
def simulate_slot(slot_id, status):
    """
    Simulates an IR Sensor turning ON or OFF 
    status parameter must be 'occupied' or 'free'.
    """
    is_occupied = (status.lower() == 'occupied')
    
    with current_app.test_client() as client:
        response = client.post('/api/slot_update', json={
            'slot_id': slot_id,
            'occupied': is_occupied,
            'rfid': None # Because discrete IR sensors do not read RFIDs
        })
        
        if response.status_code == 200:
            flash(f"Simulation: Slot {slot_id} marked as {status.upper()}.", 'info')
        else:
            data = response.get_json()
            flash(f"Simulation Error: {data.get('error', 'Unknown Error')}", 'error')
            
    return redirect(url_for('admin_routes.testing_view'))
