from models import db
from datetime import datetime

class ParkingLog(db.Model):
    __tablename__ = 'parking_logs'
    id = db.Column(db.Integer, primary_key=True)
    rfid = db.Column(db.String(50), nullable=False)
    entry_time = db.Column(db.DateTime, nullable=False, default=datetime.now)
    exit_time = db.Column(db.DateTime, nullable=True)
    duration_minutes = db.Column(db.Integer, nullable=True)
    bill_amount = db.Column(db.Integer, nullable=True)
    status = db.Column(db.String(20), nullable=False, default='active') # active, completed
    slot_id = db.Column(db.Integer, nullable=True)

    def to_dict(self):
        return {
            'id': self.id,
            'rfid': self.rfid,
            'entry_time': self.entry_time.isoformat() if self.entry_time else None,
            'exit_time': self.exit_time.isoformat() if self.exit_time else None,
            'duration_minutes': self.duration_minutes,
            'bill_amount': self.bill_amount,
            'status': self.status,
            'slot_id': self.slot_id
        }

class Slot(db.Model):
    __tablename__ = 'slots'
    slot_id = db.Column(db.Integer, primary_key=True)
    occupied = db.Column(db.Boolean, default=False, nullable=False)
    rfid = db.Column(db.String(50), nullable=True)
    last_update = db.Column(db.DateTime, nullable=False, default=datetime.now, onupdate=datetime.now)

    def to_dict(self):
        return {
            'slot_id': self.slot_id,
            'occupied': self.occupied,
            'rfid': self.rfid,
            'last_update': self.last_update.isoformat() if self.last_update else None
        }

class SystemState(db.Model):
    __tablename__ = 'system_state'
    id = db.Column(db.Integer, primary_key=True)
    total_slots = db.Column(db.Integer, nullable=False, default=4)
    occupied_slots = db.Column(db.Integer, nullable=False, default=0)
    available_slots = db.Column(db.Integer, nullable=False, default=4)
    total_revenue = db.Column(db.Integer, nullable=False, default=0)

    def to_dict(self):
        return {
            'total_slots': self.total_slots,
            'occupied_slots': self.occupied_slots,
            'available_slots': self.available_slots,
            'total_revenue': self.total_revenue
        }
