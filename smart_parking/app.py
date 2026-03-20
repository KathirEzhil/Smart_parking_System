import os
from flask import Flask, session
from config import Config
from models import db
from models.parking_model import Slot, SystemState
from models.user_model import User
from datetime import datetime

# Import blueprints
from api.parking_api import parking_bp
from api.slot_api import slot_bp
from routes.web_routes import web_bp
from routes.admin_routes import admin_bp
from routes.simulation_routes import simulation_bp
from routes.auth_routes import auth_bp
from routes.user_routes import user_bp

def create_app():
    app = Flask(__name__)
    app.config.from_object(Config)

    # Initialize extensions
    db.init_app(app)

    # Register blueprints
    app.register_blueprint(parking_bp)
    app.register_blueprint(slot_bp)
    app.register_blueprint(web_bp)
    app.register_blueprint(admin_bp)
    app.register_blueprint(simulation_bp)
    app.register_blueprint(auth_bp)
    app.register_blueprint(user_bp)

    return app

def setup_database(app):
    with app.app_context():
        # Create tables
        db.create_all()

        # Check if SystemState exists
        state = SystemState.query.first()
        if not state:
            state = SystemState(
                total_slots=app.config['TOTAL_SLOTS'],
                occupied_slots=0,
                available_slots=app.config['TOTAL_SLOTS'],
                total_revenue=0
            )
            db.session.add(state)
        
        # Check if Slots exist
        existing_slots = Slot.query.count()
        if existing_slots == 0:
            for i in range(1, app.config['TOTAL_SLOTS'] + 1):
                slot = Slot(
                    slot_id=i,
                    occupied=False,
                    rfid=None,
                    last_update=datetime.now()
                )
                db.session.add(slot)
                
        # Seed Admin and Demo Users
        if not User.query.filter_by(username='admin').first():
            admin_user = User(username='admin', role='admin')
            admin_user.set_password('adminpassword')
            db.session.add(admin_user)
            print("Seed: Admin user created.")

        if not User.query.filter_by(username='user1').first():
            user1 = User(username='user1', role='user', rfid_tag='CAR101')
            user1.set_password('password')
            db.session.add(user1)
            print("Seed: Demo User created.")
                
        db.session.commit()
        print("Database initialized successfully.")

app = create_app()

if __name__ == '__main__':
    setup_database(app)
    # Run server
    app.run(host='0.0.0.0', port=5000, debug=True)
