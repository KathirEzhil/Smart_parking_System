from functools import wraps
from flask import session, redirect, url_for, flash

def admin_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'role' not in session or session['role'] != 'admin':
            flash('Admin access required!', 'error')
            return redirect(url_for('auth_routes.login'))
        return f(*args, **kwargs)
    return decorated_function

def user_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'role' not in session or session['role'] != 'user':
            flash('User access required!', 'error')
            return redirect(url_for('auth_routes.login'))
        return f(*args, **kwargs)
    return decorated_function
