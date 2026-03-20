from flask import Blueprint, render_template, request, session, redirect, url_for, flash
from models.parking_model import ParkingLog
from pricing import calculate_duration_and_bill
from datetime import datetime
from utils.auth_decorators import user_required

user_bp = Blueprint('user_routes', __name__, url_prefix='/user')

@user_bp.route('/dashboard')
@user_required
def dashboard():
    rfid = session.get('rfid')
    
    # Get active session
    active_log = ParkingLog.query.filter_by(rfid=rfid, status='active').first()
    
    # Calculate current dummy bill if active
    current_bill = 0
    current_duration = 0
    if active_log:
        current_duration, current_bill = calculate_duration_and_bill(active_log.entry_time, datetime.now())

    # Get history
    history_logs = ParkingLog.query.filter_by(rfid=rfid, status='completed').order_by(ParkingLog.id.desc()).all()

    return render_template('user/dashboard.html', 
                            rfid=rfid, 
                            active_log=active_log, 
                            current_bill=current_bill,
                            current_duration=current_duration,
                            history=history_logs)
