import sqlite3
import csv
from sys import argv

if len(argv) != 3:
    print("Запускать: <файл БД> <выходной файл>")
    exit(-1)

conn = sqlite3.connect(argv[1])
cursor = conn.cursor()

cursor.execute("""
    SELECT DISTINCT memory_size, state
    FROM iosets
""")
rows = cursor.fetchall()

with open(argv[2], 'w', newline='', encoding='utf-8') as file:

    writer = csv.writer(file)

    for row in rows:
        memory_size, state = row
        cursor.execute("""
            SELECT value 
            FROM iosets 
            WHERE memory_size = ? AND state = ?
        """, (memory_size, state))
        io_rows = cursor.fetchall()
        io_values = [row[0] for row in io_rows]
        writer.writerow([f"Memory Size: {memory_size}, State: {state}, IOs: {', '.join(io_values)}"])

conn.close()
