import math
from datetime import datetime

def calculate_duration_and_bill(entry_time, exit_time):
    duration_delta = exit_time - entry_time
    duration_minutes = int(duration_delta.total_seconds() / 60)
    
    # Just in case of any clock drift
    if duration_minutes < 0:
        duration_minutes = 0

    if duration_minutes <= 60:
        bill = 20
    else:
        extra_minutes = duration_minutes - 60
        extra_hours = math.ceil(extra_minutes / 60)
        bill = 20 + (extra_hours * 10)
        
    return duration_minutes, bill
