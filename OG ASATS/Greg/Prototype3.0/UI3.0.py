import qrcode
import tkinter as tk
import pickle
from tkinter import ttk, messagebox, filedialog, Canvas, Frame
import os
import copy
import time

SETTINGS_FILE = "settings.txt"


clipboard = {'well': None, 'step': None}
undo_stack = []

y_position = 0  # Add this near the top, outside all functions




#means of scrolling thourgh the UI frame   
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



# copy a whole well
def copy_well(well_name):
    clipboard['well'] = copy.deepcopy(wells_data[well_name])
#paste in a well
def paste_well():
    if clipboard['well']:
        if len(wells_data) >= 12:
            messagebox.showwarning("Limit Reached", "You can only add up to 12 wells.")
            return
        well_name = f"Well {len(wells_data) + 1}"
        wells_data[well_name] = copy.deepcopy(clipboard['well'])
        # Code to refresh UI for the new well

#copy a whole step
def copy_step(well_name, step_index):
    clipboard['step'] = copy.deepcopy(wells_data[well_name]['steps'][step_index])
#paste in step
def paste_step(well_name, steps_frame):
    if clipboard['step']:
        wells_data[well_name]['steps'].append(copy.deepcopy(clipboard['step']))
        # Code to refresh UI for the new step
        
def undo():
    if not undo_stack:
        messagebox.showinfo("Undo", "Nothing to undo.")
        return
    action, name, data = undo_stack.pop()
    if action == 'step':
        wells_data[name]['steps'].append(data)
        # Code to refresh UI
    elif action == 'well':
        wells_data[name] = data
        # Code to refresh UI
        
        

#load the path for saving files
def load_save_path():
    if os.path.exists(SETTINGS_FILE):
        with open(SETTINGS_FILE, "r") as f:
            path = f.read().strip()
            if os.path.isdir(path):
                return path
    return None


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
        
        
#saving directory
SAVE_DIR = load_save_path() or select_save_folder()
if SAVE_DIR:
    os.makedirs(SAVE_DIR, exist_ok=True)

protocols = []

root = tk.Tk()
root.title("ASAT Device - Protocol Generator")
root.geometry("800x600")

notebook = ttk.Notebook(root)
notebook.pack(fill="both", expand=True)

wells_data = {}

file_name_var = tk.StringVar(value="protocols.txt")

def insert_moving_step(well_name):
    steps = wells_data[well_name]["steps"]
    
    # Avoid inserting if last step is already a Moving step
    if steps and steps[-1][0].get() == "Moving":
        return

    well_names = list(wells_data.keys())
    current_index = well_names.index(well_name)
    if current_index >= len(well_names) - 1:
        return  # No next well to move to

    steps_frame = wells_data[well_name]["steps_frame"]
    steps_scrollable = steps_frame.scrollable_frame if hasattr(steps_frame, "scrollable_frame") else steps_frame

    # Create step_type and frame
    step_type = tk.StringVar(value="Moving")

    step_frame = ttk.Frame(steps_scrollable)
    step_frame.pack(fill="x", pady=5)

    step_index = len(steps)
    ttk.Label(step_frame, text=f"Step {step_index + 1}:").pack(side="left", padx=5)

    entries_frame = ttk.Frame(step_frame)
    entries_frame.pack(side="left", padx=5)

    entries = {}
    for label in ["init_surface_time", "speed", "sequences", "sequence_pause_time"]:
        ttk.Label(entries_frame, text=label).pack(side="left", padx=2)
        entry = ttk.Entry(entries_frame, width=5)
        entry.insert(0, "1")
        entry.pack(side="left", padx=2)
        entries[label] = entry

    delete_step_button = ttk.Button(
        step_frame,
        text="🗑️ Delete",
        command=lambda sf=step_frame: delete_step(well_name, sf)
    )
    delete_step_button.pack(side="right", padx=5)

    # Store UI-connected step
    wells_data[well_name]["steps"].append((step_type, entries))







#updated parameter fields to ensure proper digit entries
def create_parameter_fields(step_type, entries_frame, entries):
    for widget in entries_frame.winfo_children():
        widget.destroy()
    entries.clear()

    # Define step-specific parameters and constraints
    step_config = {
        "Agitation": {
            "labels": [ "Speed", "Duration (s)","Volume (µL)", "% of Volume"],
            "keys": ["speed", "duration","volume", "percent_volume"],
            "limits": [1, 2, 3, 3]  # Example: Max digits per field
        },
        "Pausing": {
            "labels": ["Pause Time (s)"],
            "keys": ["pause_time"],
            "limits": [1]  
        },
        "Moving": {
            "labels": ["Init Surface Time (s)", "Speed", "Sequences", "Sequence Pause Time (s)"],
            "keys": ["init_surface_time", "speed", "sequences", "sequence_pause_time"],
            "limits": [3, 1, 1, 1]  # Enforces 3-digit limit for Init Surface Time
        }
    }

    if step_type not in step_config:
        return  # Invalid step type, do nothing

    config = step_config[step_type]
    labels, keys, limits = config["labels"], config["keys"], config["limits"]

    def validate_input(entry_text, max_digits):
        return entry_text == "" or (entry_text.isdigit() and len(entry_text) <= max_digits)

    for label, key, max_digits in zip(labels, keys, limits):
        ttk.Label(entries_frame, text=label).pack(side="left", padx=2)

        # Validate user input (allow empty input for deletion)
        vcmd = (entries_frame.register(lambda text, md=max_digits: validate_input(text, md)), "%P")

        entry = ttk.Entry(entries_frame, width=8, validate="key", validatecommand=vcmd)
        entry.pack(side="left", padx=2)
        entries[key] = entry
        
        
        



def add_step(well_name, steps_frame):
    steps_scrollable = steps_frame.scrollable_frame if hasattr(steps_frame, "scrollable_frame") else steps_frame
    steps = wells_data[well_name]["steps"]

    # Prevent adding steps after a Moving step
    if steps and steps[-1][0].get() == "Moving":
        messagebox.showwarning("Invalid Operation", "You cannot add a step after a Moving step.")
        return

    step_type = tk.StringVar(value="Agitation")
    step_frame = ttk.Frame(steps_scrollable)
    step_frame.pack(fill="x", pady=5)

    step_index = len(steps)
    ttk.Label(step_frame, text=f"Step {step_index + 1}:").pack(side="left", padx=5)

    entries_frame = ttk.Frame(step_frame)
    entries_frame.pack(side="left", padx=5)

    entries = {}
    # Populate Agitation-specific parameters by default
    for label in ["volume_ul", "duration_s", "speed"]:
        ttk.Label(entries_frame, text=label).pack(side="left", padx=2)
        entry = ttk.Entry(entries_frame, width=5)
        entry.insert(0, "1")
        entry.pack(side="left", padx=2)
        entries[label] = entry

    delete_step_button = ttk.Button(
        step_frame,
        text="🗑️ Delete",
        command=lambda sf=step_frame: delete_step(well_name, sf)
    )
    delete_step_button.pack(side="right", padx=5)

    wells_data[well_name]["steps"].append((step_type, entries))


    
#delete wells
def delete_well(well_name, well_frame):
    undo_stack.append(('well', well_name, copy.deepcopy(wells_data[well_name])))
    del wells_data[well_name]
    well_frame.destroy()
    
#delete steps  
def delete_step(well_name, step_frame):
    steps = wells_data[well_name]["steps"]
    steps_scrollable = wells_data[well_name]["steps_frame"].scrollable_frame

    # Find index of this frame
    for idx, (stype_var, entries) in enumerate(steps):
        if entries and entries[next(iter(entries))].master.master == step_frame:
            del steps[idx]
            break

    step_frame.destroy()

    # If Moving step was deleted and this is not the last well, reinsert
    well_names = list(wells_data.keys())
    current_index = well_names.index(well_name)
    if current_index < len(well_names) - 1:
        insert_moving_step(well_name)

    

def add_well():
    if len(wells_data) >= 12:
        messagebox.showwarning("Limit Reached", "You can only add up to 12 wells.")
        return

    well_name = f"Well {len(wells_data) + 1}"
    wells_data[well_name] = {"steps": []}

    # Create a new tab for the well
    well_frame = ttk.Frame(notebook)
    notebook.add(well_frame, text=well_name)

    # Steps container inside the well tab
    steps_frame = ScrollableFrame(well_frame)
    steps_frame.pack(fill="both", expand=True, pady=10)

    # Action buttons
    ttk.Button(well_frame, text="Add Step", command=lambda: add_step(well_name, steps_frame)).pack(pady=5)
    ttk.Button(well_frame, text="🛑 Delete Well", command=lambda: delete_well(well_name, well_frame)).pack(pady=5)
    ttk.Button(well_frame, text="Copy Well", command=lambda: copy_well(selected_well_name)).pack(pady=5)
    ttk.Button(well_frame, text="Paste Well", command=paste_well).pack(pady=5)

    # Store frame refs
    wells_data[well_name]["frame"] = well_frame
    wells_data[well_name]["steps_frame"] = steps_frame

    # Add initial Agitation step
    add_step(well_name, steps_frame)

    # Auto-insert Moving step into the *previous well*, if one exists
    well_names = list(wells_data.keys())
    if len(well_names) > 1:
        previous_well = well_names[-2]
        insert_moving_step(previous_well)



def generate_protocol_files():
    if not os.path.exists(SAVE_DIR):
        os.makedirs(SAVE_DIR)

    base_file_name = file_name_var.get().strip()
    if not base_file_name:
        messagebox.showerror("Error", "Please enter a valid file name.")
        return

    if not base_file_name.endswith(".txt"):
        base_file_name += ".txt"

    protocol_data = []
    well_names = list(wells_data.keys())

    for i, well_name in enumerate(well_names):
        well_info = wells_data[well_name]
        steps = []

        for step_type_var, entries in well_info["steps"]:
            step_type = step_type_var.get()
            step_dict = {"type": step_type}
            for key, entry in entries.items():
                value = entry.get().strip()
                if not value.isdigit():
                    messagebox.showerror("Error", f"Invalid input in {well_name} for {key}.")
                    return
                step_dict[key] = int(value)

            # If Moving step, attach target well
            if step_type == "Moving" and i < len(well_names) - 1:
                step_dict["target"] = well_names[i + 1]

            steps.append(step_dict)

        # Append auto-Moving step if not last well and none is present
        if i < len(well_names) - 1 and not any(s["type"] == "Moving" for s in steps):
            steps.append({
                "type": "Moving",
                "init_surface_time": 1,
                "speed": 1,
                "sequences": 1,
                "sequence_pause_time": 1,
                "target": well_names[i + 1]
            })

        protocol_data.append({"well": well_name, "steps": steps})

    try:
        with open(os.path.join(SAVE_DIR, base_file_name), "w") as f:
            for item in protocol_data:
                f.write(f"{item['well']}\n")
                for step in item["steps"]:
                    f.write(f"  Step: {step['type']}\n")
                    for k, v in step.items():
                        if k != "type":
                            f.write(f"    {k}: {v}\n")
        messagebox.showinfo("Success", f"Protocol saved to {base_file_name}")
    except Exception as e:
        messagebox.showerror("Error", f"Failed to save protocol: {e}")





#UI structure


top_frame = ttk.Frame(root)
top_frame.pack(fill="x", pady=5)

file_frame = ttk.Frame(root)
file_frame.pack(fill="x", pady=5)

save_path_label = ttk.Label(root, text=f"Save Folder: {SAVE_DIR}", wraplength=500)
save_path_label.pack(pady=5)

ttk.Label(file_frame, text="File Name:").pack(side="left", padx=5)
file_name_entry = ttk.Entry(file_frame, textvariable=file_name_var)
file_name_entry.pack(side="left", padx=5)

ttk.Button(top_frame, text="Add Well", command=add_well).pack(side="left", padx=5)
ttk.Button(top_frame, text="Generate Protocol File", command=generate_protocol_files).pack(side="left", padx=5)
ttk.Button(file_frame, text="Select Folder", command=change_save_folder).pack(side="left", padx=5)

# Buttons to add in the UI:
ttk.Button(root, text="Copy Well", command=lambda: copy_well(selected_well_name)).pack()
ttk.Button(root, text="Paste Well", command=paste_well).pack()
ttk.Button(root, text="Copy Step", command=lambda: copy_step(well_name, step_index)).pack()
ttk.Button(root, text="Paste Step", command=lambda: paste_step(well_name, steps_frame)).pack()
ttk.Button(root, text="Undo", command=undo).pack()

root.mainloop()
