import serial
import serial.tools.list_ports
import threading
import os
import time
import customtkinter as ctk
from tkinter import messagebox
import matplotlib.pyplot as plt

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

class ReactionGameGUI(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("Professional Reaction Dashboard")
        self.geometry("1000x700")
        self.ser = None
        self.setup_ui()
        self.connect_serial()

    def setup_ui(self):
        # Ввод имен
        self.top = ctk.CTkFrame(self)
        self.top.pack(pady=20, fill="x", padx=20)
        
        self.p1_entry = ctk.CTkEntry(self.top, placeholder_text="Player 1")
        self.p1_entry.pack(side="left", padx=10); self.p1_entry.insert(0, "gulara")
        
        self.p2_entry = ctk.CTkEntry(self.top, placeholder_text="Player 2")
        self.p2_entry.pack(side="left", padx=10); self.p2_entry.insert(0, "dr.alex")
        
        ctk.CTkButton(self.top, text="Start", command=self.send_start, fg_color="green").pack(side="right", padx=10)

        self.score_label = ctk.CTkLabel(self, text="0 : 0", font=("Arial", 140, "bold"))
        self.score_label.pack(pady=30)
        
        self.status = ctk.CTkLabel(self, text="Система готова", font=("Arial", 20))
        self.status.pack(pady=10)

        self.bot_frame = ctk.CTkFrame(self)
        self.bot_frame.pack(pady=20, fill="x", padx=20)
        
        ctk.CTkButton(self.bot_frame, text="Player 1 Progress", command=lambda: self.plot_performance(self.p1_entry.get())).pack(side="left", expand=True, padx=10)
        ctk.CTkButton(self.bot_frame, text="Head-to-head Components", command=self.plot_head_to_head, fg_color="purple").pack(side="left", expand=True, padx=10)
        ctk.CTkButton(self.bot_frame, text="Player 2 Progress", command=lambda: self.plot_performance(self.p2_entry.get())).pack(side="left", expand=True, padx=10)

    def connect_serial(self):
        for p in serial.tools.list_ports.comports():
            try:
                self.ser = serial.Serial(p.device, 9600, timeout=0.1)
                threading.Thread(target=self.reader, daemon=True).start()
                return
            except: continue

    def send_start(self):
        if self.ser: self.ser.write(b"START\n")

    def reader(self):
        while True:
            if self.ser and self.ser.in_waiting:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if line: self.after(10, self.parse, line)
            time.sleep(0.01)

    def parse(self, data):
        clean = data.replace("Arduino:", "").strip()
        
        if "SCORE:" in clean:
            s = clean.split("SCORE:")[1].split(",")
            self.score_label.configure(text=f"{s[0]} : {s[1]}")
            
        elif "FALSE_START:" in clean:
            p = self.p1_entry.get() if "P1" in clean else self.p2_entry.get()
            self.status.configure(text=f"False start! Too early!", text_color="red")
            
        elif "REACTION:" in clean:
            p_code, r_time = clean.split(":")[1], clean.split(":")[2]
            name = self.p1_entry.get() if "P1" in p_code else self.p2_entry.get()
            opp = self.p2_entry.get() if "P1" in p_code else self.p1_entry.get()
            self.save_data(name, opp, r_time, "WIN")
            self.status.configure(text=f"{name}: {r_time} ms", text_color="white")
            
        elif "WINNER:" in clean:
            winner = self.p1_entry.get() if "P1" in clean else self.p2_entry.get()
            messagebox.showinfo("VICTORY!", f"The player {winner} wins the match!")

    def save_data(self, player, opp, reaction, res):
        file = f"{player}.txt"
        with open(file, "a", encoding="utf-8") as f:
            f.write(f"{time.strftime('%Y-%m-%d')}|{opp}|{reaction}|{res}\n")

    def plot_performance(self, name):
        if not os.path.exists(f"{name}.txt"): return
        times = []
        with open(f"{name}.txt", "r") as f:
            for l in f: times.append(int(l.split("|")[2]))
        
        plt.figure(f"Performance: {name}")
        plt.plot(times, marker='o', label='Reaction Time (ms)')
        plt.axhline(y=sum(times)/len(times), color='r', linestyle='--', label='Average')
        plt.title(f"Reaction Progress for {name}")
        plt.legend(); plt.grid(True); plt.show()

    def plot_head_to_head(self):
        p1, p2 = self.p1_entry.get(), self.p2_entry.get()
        if not os.path.exists(f"{p1}.txt"): return
        
        p1_wins = 0
        with open(f"{p1}.txt", "r") as f:
            for l in f:
                if l.split("|")[1] == p2: p1_wins += 1
        
        plt.figure("Head to Head Analysis")
        plt.bar([f"{p1} wins vs {p2}"], [p1_wins], color='blue')
        plt.title("Competitive History")
        plt.show()

if __name__ == "__main__":
    app = ReactionGameGUI()
    app.mainloop()
