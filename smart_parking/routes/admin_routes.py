from flask import Blueprint, render_template, jsonify
from models.parking_model import ParkingLog, SystemState
from utils.auth_decorators import admin_required

admin_bp = Blueprint('admin_routes', __name__, url_prefix='/admin')

@admin_bp.route('/')
@admin_bp.route('/dashboard')
@admin_required
def admin_dashboard():
    state = SystemState.query.first()
    recent_logs = ParkingLog.query.order_by(ParkingLog.id.desc()).limit(10).all()
    return render_template('admin/dashboard.html', state=state, logs=recent_logs)

@admin_bp.route('/logs')
@admin_required
def logs():
    all_logs = ParkingLog.query.order_by(ParkingLog.id.desc()).all()
    return render_template('admin/logs.html', logs=all_logs)

@admin_bp.route('/billing')
@admin_required
def billing():
    state = SystemState.query.first()
    completed_logs = ParkingLog.query.filter_by(status='completed').all()
    
    total_transactions = len(completed_logs)
    total_duration = sum([log.duration_minutes for log in completed_logs if log.duration_minutes])
    
    avg_parking_time = total_duration / total_transactions if total_transactions > 0 else 0
    
    return render_template('admin/billing.html', 
                         revenue=state.total_revenue if state else 0,
                         transactions=total_transactions,
                         avg_time=round(avg_parking_time, 2))

@admin_bp.route('/analytics')
@admin_required
def analytics_view():
    from collections import defaultdict
    from datetime import datetime, timedelta

    all_logs = ParkingLog.query.all()
    completed = [l for l in all_logs if l.status == 'completed']
    active    = [l for l in all_logs if l.status == 'active']

    # ── KPIs ──────────────────────────────────────────────
    total_vehicles    = len(all_logs)
    total_revenue     = sum(l.bill_amount or 0 for l in completed)
    total_completed   = len(completed)
    currently_inside  = len(active)

    durations = [l.duration_minutes for l in completed if l.duration_minutes]
    avg_duration = round(sum(durations) / len(durations)) if durations else 0
    max_duration = max(durations, default=0)
    min_duration = min(durations, default=0)

    # ── Peak hour (which hour had the most entries) ────────
    hour_count = defaultdict(int)
    for l in all_logs:
        if l.entry_time:
            hour_count[l.entry_time.hour] += 1
    if hour_count:
        peak_hour = max(hour_count, key=hour_count.get)
        peak_hour_str = f"{peak_hour:02d}:00 – {(peak_hour+1)%24:02d}:00"
        peak_count    = hour_count[peak_hour]
    else:
        peak_hour_str = "No data yet"
        peak_count    = 0

    # ── Busiest day ────────────────────────────────────────
    day_count = defaultdict(int)
    for l in all_logs:
        if l.entry_time:
            day_count[l.entry_time.strftime('%A')] += 1
    busiest_day       = max(day_count, key=day_count.get) if day_count else 'No data'
    busiest_day_count = day_count.get(busiest_day, 0)

    # ── Turnover rate (completions per day averaged) ───────
    if completed:
        earliest = min(l.entry_time for l in completed if l.entry_time)
        days_active = max((datetime.now() - earliest).days, 1)
        turnover_rate = round(total_completed / days_active, 1)
    else:
        turnover_rate = 0

    stats = dict(
        total_vehicles   = total_vehicles,
        total_revenue    = total_revenue,
        total_completed  = total_completed,
        currently_inside = currently_inside,
        avg_duration     = avg_duration,
        max_duration     = max_duration,
        min_duration     = min_duration,
        peak_hour_str    = peak_hour_str,
        peak_count       = peak_count,
        busiest_day      = busiest_day,
        busiest_day_count= busiest_day_count,
        turnover_rate    = turnover_rate,
    )
    return render_template('admin/analytics.html', **stats)

@admin_bp.route('/testing')
@admin_required
def testing_view():
    return render_template('admin/testing.html')

@admin_bp.route('/api/chart_data')
@admin_required
def chart_data():
    from collections import defaultdict
    logs = ParkingLog.query.all()

    # Count vehicles entered per hour of day (0-23)
    hour_count = defaultdict(int)
    for log in logs:
        if log.entry_time:
            hour_count[log.entry_time.hour] += 1

    labels = [f"{h:02d}:00" for h in range(24)]
    data   = [hour_count[h] for h in range(24)]

    return jsonify({
        'occupancy': {'labels': labels, 'data': data}
    })
