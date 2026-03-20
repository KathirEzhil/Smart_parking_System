import os

class Config:
    SECRET_KEY = os.environ.get('SECRET_KEY') or 'super-secret-key-parking'
    # Use config inside the app directory
    basedir = os.path.abspath(os.path.dirname(__file__))
    SQLALCHEMY_DATABASE_URI = os.environ.get('DATABASE_URL') or 'sqlite:///' + os.path.join(basedir, 'parking.db')
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    TOTAL_SLOTS = 4
