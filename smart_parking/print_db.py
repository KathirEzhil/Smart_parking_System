import sqlite3
import pandas as pd

def print_db():
    conn = None
    try:
        conn = sqlite3.connect('C:/Users/Kathir Ezhil A/Downloads/Smart_parking1/smart_parking/parking.db')
        
        # Get all tables
        query = "SELECT name FROM sqlite_master WHERE type='table'"
        tables = pd.read_sql_query(query, conn)
        
        if tables.empty:
            print("Database is completely empty.")
            return

        for index, row in tables.iterrows():
            table_name = row['name']
            print(f"\n{'='*50}")
            print(f" TABLE: {table_name.upper()}")
            print(f"{'='*50}")
            
            df = pd.read_sql_query(f"SELECT * FROM {table_name}", conn)
            
            if df.empty:
                print("  (Empty Table)")
            else:
                print(df.to_string(index=False))
                
    except Exception as e:
        print(f"Error reading database: {e}")
    finally:
        if conn:
            conn.close()

if __name__ == '__main__':
    print_db()
