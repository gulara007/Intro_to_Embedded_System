import tkinter as tk
from tkinter import ttk, messagebox
import serial
import sqlite3
import threading
import re

SERIAL_PORT = "COM7"
BAUD_RATE = 9600
DB_NAME = "rfid_tags.db"


def setup_database():
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            uid TEXT UNIQUE,
            scan_count INTEGER DEFAULT 1
        )
    """)
    conn.commit()
    conn.close()


def save_tag(uid):
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()

    cur.execute("SELECT scan_count FROM tags WHERE uid=?", (uid,))
    row = cur.fetchone()

    if row:
        cur.execute("UPDATE tags SET scan_count = scan_count + 1 WHERE uid=?", (uid,))
    else:
        cur.execute("INSERT INTO tags(uid, scan_count) VALUES(?, ?)", (uid, 1))

    conn.commit()
    conn.close()


def get_tags():
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()
    cur.execute("SELECT id, uid, scan_count FROM tags ORDER BY id")
    rows = cur.fetchall()
    conn.close()
    return rows


class RFIDApp:
    def __init__(self, root):
        self.root = root
        self.root.title("RFID Tag Database Viewer")
        self.root.geometry("750x450")

        self.serial_connection = None
        self.running = False

        tk.Button(root, text="Connect to COM7", command=self.connect_serial).pack(pady=8)

        self.status_label = tk.Label(root, text="Status: Not connected", fg="red")
        self.status_label.pack()

        self.last_label = tk.Label(root, text="Last received: none")
        self.last_label.pack(pady=5)

        self.table = ttk.Treeview(root, columns=("ID", "UID", "COUNT"), show="headings")
        self.table.heading("ID", text="Unique ID")
        self.table.heading("UID", text="RFID Tag UID")
        self.table.heading("COUNT", text="Scan Count")

        self.table.column("ID", width=100)
        self.table.column("UID", width=450)
        self.table.column("COUNT", width=150)

        self.table.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        tk.Button(root, text="Refresh Database", command=self.load_data).pack(pady=8)

        self.load_data()

    def connect_serial(self):
        try:
            self.serial_connection = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            self.running = True

            self.status_label.config(text="Status: Connected to COM7", fg="green")

            thread = threading.Thread(target=self.read_serial, daemon=True)
            thread.start()

        except Exception as e:
            messagebox.showerror("Connection Error", str(e))

    def read_serial(self):
        while self.running:
            try:
                line = self.serial_connection.readline().decode(errors="ignore").strip()

                if not line:
                    continue

                print("Received:", line)
                self.root.after(0, lambda text=line: self.last_label.config(text=f"Last received: {text}"))

                uid = None

                if line.startswith("DATA_PACKET:"):
                    uid = line.replace("DATA_PACKET:", "").strip()

                elif "UID" in line.upper():
                    uid = line.split(":")[-1].strip()

                elif re.fullmatch(r"[0-9A-Fa-f ]{8,}", line):
                    uid = line.strip()

                if uid:
                    save_tag(uid)
                    self.root.after(0, self.load_data)

            except Exception as e:
                print("Read Error:", e)

    def load_data(self):
        for row in self.table.get_children():
            self.table.delete(row)

        for row in get_tags():
            self.table.insert("", tk.END, values=row)


setup_database()

root = tk.Tk()
app = RFIDApp(root)
root.mainloop()
