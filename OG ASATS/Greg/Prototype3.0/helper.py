import qrcode
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, Canvas, Frame
import os
import time
import copy

SETTINGS_FILE = "settings.txt"

clipboard = {'well': None, 'step': None}
undo_stack = []
wells_data = {}
SAVE_DIR = "protocols"

# Ensure SAVE_DIR exists
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)



#load the path for saving files
def load_save_path():
    if os.path.exists(SETTINGS_FILE):
        with open(SETTINGS_FILE, "r") as f:
            path = f.read().strip()
            if os.path.isdir(path):
                return path
    return None

#saving directory
SAVE_DIR = load_save_path() or select_save_folder()
if SAVE_DIR:
    os.makedirs(SAVE_DIR, exist_ok=True)

#selecting the save folder
def select_save_folder():
    folder_selected = filedialog.askdirectory()
    if folder_selected:
        with open(SETTINGS_FILE, "w") as f:
            f.write(folder_selected)
        save_path_label.config(text=f"Save Folder: {folder_selected}")
        return folder_selected
    return None
#change the save folder funciton
def change_save_folder():
    global SAVE_DIR
    new_folder = select_save_folder()
    if new_folder:
        SAVE_DIR = new_folder
        
# Scrollable Frame Class
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

# Create Parameter Fields with Input Validation
def create_parameter_fields(step_type, entries_frame, entries):
    for widget in entries_frame.winfo_children():
        widget.destroy()
    entries.clear()

    step_config = {
        "Agitation": {
            "labels": ["Volume (µL)", "% of Volume", "Speed", "Duration (s)"],
            "keys": ["volume", "percent_volume", "speed", "duration"],
            "limits": [1, 2, 1, 2]  
        },
        "Pausing": {
            "labels": ["Pause Time (s)"],
            "keys": ["pause_time"],
            "limits": [1]  
        },
        "Moving": {
            "labels": ["Init Surface Time (s)", "Speed", "Sequences", "Sequence Pause Time (s)"],
            "keys": ["init_surface_time", "speed", "sequences", "sequence_pause_time"],
            "limits": [3, 1, 1, 1]  
        }
    }

    if step_type not in step_config:
        return  

    config = step_config[step_type]
    labels, keys, limits = config["labels"], config["keys"], config["limits"]

    def validate_input(entry_text, max_digits):
        return entry_text == "" or (entry_text.isdigit() and len(entry_text) <= max_digits)

    for label, key, max_digits in zip(labels, keys, limits):
        ttk.Label(entries_frame, text=label).pack(side="left", padx=2)
        vcmd = (entries_frame.register(lambda text, md=max_digits: validate_input(text, md)), "%P")
        entry = ttk.Entry(entries_frame, width=8, validate="key", validatecommand=vcmd)
        entry.pack(side="left", padx=2)
        entries[key] = entry

def delete_step(well_name, step_frame):
    steps = wells_data[well_name]["steps"]

    # Find the step object that matches this frame
    step_to_delete = next((step for step in steps if step["frame"] == step_frame), None)

    if not step_to_delete:
        messagebox.showerror("Error", "Step not found!")
        return

    # Save step data for undo (store only serializable data)
    undo_stack.append(('step', well_name, {
        "type": step_to_delete["type"].get(),
        "entries": {k: v.get() for k, v in step_to_delete["entries"].items()}
    }))

    # Remove the step from UI
    step_frame.destroy()

    # Remove step from data
    steps.remove(step_to_delete)

    # Reassign step numbers dynamically
    for i, step in enumerate(steps):
        if step["label"].winfo_exists():  # Ensure label still exists before updating
            step["label"].config(text=f"Step {i+1}:")



def add_step(well_name, steps_frame):
    step_type = tk.StringVar(value="Agitation")

    step_frame = ttk.Frame(steps_frame.scrollable_frame)
    step_frame.pack(fill="x", pady=5)

    step_label = ttk.Label(step_frame, text=f"Step {len(wells_data[well_name]['steps']) + 1}:")
    step_label.pack(side="left", padx=5)

    step_menu = ttk.Combobox(step_frame, textvariable=step_type, 
                             values=["Agitation", "Pausing", "Moving"], width=10)
    step_menu.pack(side="left", padx=5)

    entries_frame = ttk.Frame(step_frame)
    entries_frame.pack(side="left", padx=5)
    entries = {}

    create_parameter_fields(step_type.get(), entries_frame, entries)

    step_menu.bind("<<ComboboxSelected>>", lambda e: create_parameter_fields(step_type.get(), entries_frame, entries))

    delete_button = ttk.Button(step_frame, text="🗑️ Delete", 
                               command=lambda: delete_step(well_name, step_frame))
    delete_button.pack(side="right", padx=5)

    wells_data[well_name]["steps"].append({
        "frame": step_frame,
        "label": step_label,
        "type": step_type,
        "entries": entries
    })






# Delete Well
def delete_well(well_name, well_frame):
    undo_stack.append(('well', well_name, copy.deepcopy(wells_data[well_name])))
    del wells_data[well_name]
    notebook.forget(notebook.index(well_frame))

# Add Well
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

    name_var = tk.StringVar(value=well_name)
    ttk.Entry(well_frame, textvariable=name_var).pack(pady=5)

    ttk.Button(well_frame, text="Add Step", command=lambda: add_step(well_name, steps_frame)).pack(pady=5)
    ttk.Button(well_frame, text="Generate Protocol", command=lambda: generate_protocol_file(name_var.get())).pack(pady=5)
    ttk.Button(well_frame, text="🛑 Delete Well", command=lambda: delete_well(well_name, well_frame)).pack(pady=5)








# Generate Protocol File per Well
def generate_protocol_file(well_name):
    if not os.path.exists(SAVE_DIR):
        os.makedirs(SAVE_DIR)

    protocols = []
    for step in wells_data[well_name]["steps"]:
        step_type = step.children["!combobox"].get()
        entries = {label: entry.get() for label, entry in step.children["!frame"].children.items() if isinstance(entry, ttk.Entry)}

        if step_type == "Agitation":
            protocol_string = f"B{''.join(entries.values())}"
        elif step_type == "Pausing":
            protocol_string = f"P{entries['pause_time']}"
        elif step_type == "Moving":
            protocol_string = f"M{''.join(entries.values())}"
        else:
            continue

        protocols.append(protocol_string)

    if protocols:
        protocol_file = os.path.join(SAVE_DIR, f"{well_name}.txt")
        with open(protocol_file, 'w') as f:
            f.write("\n".join(protocols))

        qr = qrcode.make("\r\n".join(protocols))
        qr_file = os.path.join(SAVE_DIR, f"{well_name}.png")
        qr.save(qr_file)

        messagebox.showinfo("Success", f"Protocol and QR code saved for {well_name}!")

# UI Setup
root = tk.Tk()
root.title("ASAT Device - Protocol Generator")
root.geometry("800x600")

file_frame = ttk.Frame(root)
file_frame.pack(fill="x", pady=5)
save_path_label = ttk.Label(root, text=f"Save Folder: {SAVE_DIR}", wraplength=500)
save_path_label.pack(pady=5)

notebook = ttk.Notebook(root)
notebook.pack(fill="both", expand=True)

ttk.Button(root, text="Add Well", command=add_well).pack(pady=10)
ttk.Button(file_frame, text="Select Folder", command=change_save_folder).pack(side="left", padx=5)

root.mainloop()
