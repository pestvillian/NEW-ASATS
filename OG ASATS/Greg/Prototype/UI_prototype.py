import qrcode
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, Canvas, Frame
import os
import time
# File to store last used save path
SETTINGS_FILE = "settings.txt"



class ScrollableFrame(ttk.Frame):
    def __init__(self, container):
        super().__init__(container)
        canvas = Canvas(self)
        scrollbar = ttk.Scrollbar(self, orient="vertical", command=canvas.yview)
        self.scrollable_frame = ttk.Frame(canvas)

        self.scrollable_frame.bind(
            "<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )

        window = canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        self.scrollable_frame.bind("<Enter>", lambda _: self.bind_scroll(canvas))
        self.scrollable_frame.bind("<Leave>", lambda _: self.unbind_scroll(canvas))

    def bind_scroll(self, canvas):
        canvas.bind_all("<MouseWheel>", lambda e: canvas.yview_scroll(-1*(e.delta//120), "units"))

    def unbind_scroll(self, canvas):
        canvas.unbind_all("<MouseWheel>")

def update_step_inputs(frame, step_type, entries):
    for widget in frame.winfo_children():
        widget.destroy()

    ttk.Label(frame, text="Step Type:").grid(row=0, column=0)
    ttk.Label(frame, text=step_type.get()).grid(row=0, column=1)

    if step_type.get() == "Agitation":
        ttk.Label(frame, text="Depth:").grid(row=1, column=0)
        entries["depth"] = ttk.Entry(frame)
        entries["depth"].grid(row=1, column=1)

        ttk.Label(frame, text="Speed:").grid(row=2, column=0)
        entries["speed"] = ttk.Entry(frame)
        entries["speed"].grid(row=2, column=1)

        ttk.Label(frame, text="Duration:").grid(row=3, column=0)
        entries["duration"] = ttk.Entry(frame)
        entries["duration"].grid(row=3, column=1)

    elif step_type.get() in ["Rest (Above)", "Rest (Inside)"]:
        ttk.Label(frame, text="Duration:").grid(row=1, column=0)
        entries["duration"] = ttk.Entry(frame)
        entries["duration"].grid(row=1, column=1)

    elif step_type.get() == "Custom":
        ttk.Label(frame, text="Depth:").grid(row=1, column=0)
        entries["depth"] = ttk.Entry(frame)
        entries["depth"].grid(row=1, column=1)

        ttk.Label(frame, text="Speed:").grid(row=2, column=0)
        entries["speed"] = ttk.Entry(frame)
        entries["speed"].grid(row=2, column=1)

        ttk.Label(frame, text="Duration:").grid(row=3, column=0)
        entries["duration"] = ttk.Entry(frame)
        entries["duration"].grid(row=3, column=1)

        ttk.Label(frame, text="Break Time:").grid(row=4, column=0)
        entries["break_time"] = ttk.Entry(frame)
        entries["break_time"].grid(row=4, column=1)

def load_save_path():
    if os.path.exists(SETTINGS_FILE):
        with open(SETTINGS_FILE, "r") as f:
            path = f.read().strip()
            if os.path.isdir(path):
                return path
    return None

def select_save_folder():
    folder_selected = filedialog.askdirectory()
    if folder_selected:
        with open(SETTINGS_FILE, "w") as f:
            f.write(folder_selected)
        save_path_label.config(text=f"Save Folder: {folder_selected}")
        return folder_selected
    return None

SAVE_DIR = load_save_path() or select_save_folder()
if SAVE_DIR:
    os.makedirs(SAVE_DIR, exist_ok=True)

gcode = ["G28"]

root = tk.Tk()
root.title("ASAT Device - G-code Generator")
root.geometry("800x600")

notebook = ttk.Notebook(root)
notebook.pack(fill="both", expand=True)

wells_data = {}

# File name entry field
file_name_var = tk.StringVar(value="output.gcode")


def delete_step(well_name, step_frame, step_index):
    step_frame.destroy()
    wells_data[well_name]["steps"].pop(step_index)

def add_step(well_name, steps_frame):
    step_type = tk.StringVar()
    step_type.set("Agitation")

    step_frame = ttk.Frame(steps_frame.scrollable_frame)
    step_frame.pack(fill="x", pady=5)

    step_index = len(wells_data[well_name]["steps"])

    ttk.Label(step_frame, text=f"Step {step_index+1}:").pack(side="left", padx=5)

    step_menu = ttk.Combobox(step_frame, textvariable=step_type, values=["Agitation", "Rest (Above)", "Rest (Inside)", "Custom"])
    step_menu.pack(side="left", padx=5)

    entries = {}
    input_frame = ttk.Frame(step_frame)
    input_frame.pack(side="left", padx=5)

    step_menu.bind("<<ComboboxSelected>>", lambda event: update_step_inputs(input_frame, step_type, entries))
    update_step_inputs(input_frame, step_type, entries)

    delete_step_button = ttk.Button(step_frame, text="🗑️ Delete", command=lambda: delete_step(well_name, step_frame, step_index))
    delete_step_button.pack(side="right", padx=5)

    wells_data[well_name]["steps"].append((step_type, entries))

def delete_well(well_name, well_frame):
    del wells_data[well_name]
    well_frame.destroy()
    notebook.forget(notebook.index(well_frame))

def add_well():
    if len(wells_data) >= 12:
        messagebox.showwarning("Limit Reached", "You can only add up to 12 wells.")
        return

    well_name = f"Well {len(wells_data) + 1}"
    wells_data[well_name] = {"steps": []}

    well_frame = ttk.Frame(notebook)
    notebook.add(well_frame, text=well_name)

    steps_frame = ScrollableFrame(well_frame)
    steps_frame.pack(fill="both", expand=True, pady=10)

    ttk.Button(well_frame, text="Add Step", command=lambda: add_step(well_name, steps_frame)).pack(pady=5)

    delete_well_button = ttk.Button(well_frame, text="🛑 Delete Well", command=lambda: delete_well(well_name, well_frame))
    delete_well_button.pack(pady=5)

def generate_gcode():
    global gcode
    gcode = ["G28"]

    for well_name, data in wells_data.items():
        for step_type, entries in data["steps"]:
            step_type_val = step_type.get()
            if step_type_val == "Agitation":
                depth = entries["depth"].get()
                speed = entries["speed"].get()
                duration = entries["duration"].get()
                while time.wait(duration) != 0: #attempt at timing the agitation
                    gcode.append(f"G0 Z{depth} F{speed} ; Move down")
                    gcode.append(f"G4 P{duration} ; Wait")
                    gcode.append(f"G0 Z0 ; Move up")

            elif step_type_val in ["Rest (Above)", "Rest (Inside)"]:
                duration = entries["duration"].get()
                gcode.append(f"G4 P{duration} ; Rest")

            elif step_type_val == "Custom":
                depth = entries["depth"].get()
                speed = entries["speed"].get()
                duration = entries["duration"].get()
                break_time = entries["break_time"].get()
                gcode.append(f"G0 Z{depth} F{speed} ; Move down")
                gcode.append(f"G4 P{duration} ; Agitate")
                gcode.append(f"G4 P{break_time} ; Break")
                gcode.append(f"G0 Z0 ; Move up")

    gcode.append("G28 ; Move home at the end")

    file_name = file_name_var.get().strip()
    if not file_name.endswith(".gcode"):
        file_name += ".gcode"

    gcode_file = os.path.join(SAVE_DIR, file_name)
    with open(gcode_file, 'w') as f:
        f.write("\n".join(gcode))

    messagebox.showinfo("Success", f"G-code saved as: {gcode_file}")

# UI Setup
top_frame = ttk.Frame(root)
top_frame.pack(fill="x", pady=5)


# File name input
file_frame = ttk.Frame(root)
file_frame.pack(fill="x", pady=5)

ttk.Label(file_frame, text="File Name:").pack(side="left", padx=5)
file_name_entry = ttk.Entry(file_frame, textvariable=file_name_var)
file_name_entry.pack(side="left", padx=5)

# Directory selection button
def change_save_folder():
    global SAVE_DIR
    new_folder = select_save_folder()
    if new_folder:
        SAVE_DIR = new_folder



top_frame = ttk.Frame(root)
top_frame.pack(fill="x", pady=5)

ttk.Button(top_frame, text="Add Well", command=add_well).pack(side="left", padx=5)
ttk.Button(top_frame, text="Generate G-code", command=generate_gcode).pack(side="left", padx=5)
ttk.Button(file_frame, text="Select Folder", command=change_save_folder).pack(side="left", padx=5)


save_path_label = ttk.Label(root, text=f"Save Folder: {SAVE_DIR}", wraplength=500)
save_path_label.pack(pady=5)

root.mainloop()
