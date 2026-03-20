from flask import Blueprint, render_template, request, session, redirect, url_for, flash
from models.user_model import User

auth_bp = Blueprint('auth_routes', __name__, url_prefix='/auth')

@auth_bp.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')
        
        user = User.query.filter_by(username=username).first()
        if user and user.check_password(password):
            session['user_id'] = user.id
            session['role'] = user.role
            session['username'] = user.username
            
            if user.role == 'admin':
                return redirect(url_for('admin_routes.admin_dashboard'))
            elif user.role == 'user':
                # Bind rfid to session for user routes to query fast
                session['rfid'] = user.rfid_tag 
                return redirect(url_for('user_routes.dashboard'))
        else:
            flash('Invalid username or password', 'error')

    return render_template('auth/login.html')

@auth_bp.route('/logout')
def logout():
    session.clear()
    flash('Successfully logged out.', 'success')
    return redirect(url_for('auth_routes.login'))

@auth_bp.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')
        rfid_tag = request.form.get('rfid_tag')
        
        from models import db
        
        # Check if user exists
        if User.query.filter_by(username=username).first():
            flash('Username already exists.', 'error')
            return redirect(url_for('auth_routes.register'))
            
        new_user = User(username=username, role='user', rfid_tag=rfid_tag)
        new_user.set_password(password)
        db.session.add(new_user)
        db.session.commit()
        
        flash('Registration successful! You can now log in.', 'success')
        return redirect(url_for('auth_routes.login'))
        
    return render_template('auth/register.html')
