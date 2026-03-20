from flask import Blueprint, render_template, redirect, url_for
from models.parking_model import SystemState, ParkingLog

web_bp = Blueprint('web_routes', __name__)

@web_bp.route('/')
def index():
    return redirect(url_for('auth_routes.login'))

@web_bp.route('/dashboard')
def dashboard():
    state = SystemState.query.first()
    active_logs = ParkingLog.query.filter_by(status='active').all()
    
    return render_template('dashboard.html', state=state, active_logs=active_logs)

@web_bp.route('/analytics')
def analytics():
    # Deprecated: Analytics is now strictly in admin_routes.py
    return redirect(url_for('admin_routes.analytics_view'))

@web_bp.route('/testing')
def testing():
    # Actually wait, let's keep testing accessible if needed, but point to correct path
    return render_template('admin/testing.html')
