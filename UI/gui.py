import tkinter as tk
from tkinter import messagebox
import threading
import time
import json
import os
from string_encoder import encode_agitation, encode_moving, encode_pausing
from Send_UART import send_uart_text, send_and_listen

TERRY_PORT = "/dev/cu.usbmodem101"   # TODO: confirm actual port on lab desktop
BAUD_RATE = 9600
EXPECTED_WELL_COUNT = 10  # TODO: confirm with Gamze — not fully confirmed yet
PROTOCOL_DIR = "saved_protocols"

root = tk.Tk()
root.title("ASATS - Protocol Builder")
root.update()

step_type = tk.StringVar(value="AGITATION")

# --- Step type selector ---
type_frame = tk.Frame(root)
type_frame.grid(row=0, column=0, columnspan=3, pady=10)
tk.Radiobutton(type_frame, text="Agitation", variable=step_type, value="AGITATION", command=lambda: show_fields()).pack(side="left")
tk.Radiobutton(type_frame, text="Moving", variable=step_type, value="MOVING", command=lambda: show_fields()).pack(side="left")
tk.Radiobutton(type_frame, text="Pausing", variable=step_type, value="PAUSING", command=lambda: show_fields()).pack(side="left")

field_defs = {
    "AGITATION": [
        ("Speed", "speed", 1, 9),
        ("Duration (sec)", "duration", 0, 99),
        ("Volume (mL)", "volume", 0, 9999),
        ("Percent Volume (%)", "percent_volume", 1, 99),   # 2-digit field — 100 not representable
        ("Pause Time (sec)", "pausetime", 0, 99),
        ("Repeats (0 = skip step)", "repeats", 0, 99),
    ],
    "MOVING": [
        ("Bead Attachment Time (sec)", "initial_surface_time", 0, 999),
        ("Speed", "speed", 0, 9),
        ("Stop At Sequences (unused)", "stop_at_sequences", 0, 9),
        ("Sequence Pause Time (unused)", "sequence_pause_time", 0, 99),
    ],
    "PAUSING": [
        ("Duration (sec)", "duration", 0, 9),
    ],
}

entries = {}
rows = {}
ranges = {}

for stype, fields in field_defs.items():
    entries[stype] = {}
    rows[stype] = []
    ranges[stype] = {}
    for i, (label_text, key, min_val, max_val) in enumerate(fields):
        row = i + 1
        lbl = tk.Label(root, text=label_text)
        lbl.grid(row=row, column=0, sticky="e")
        ent = tk.Entry(root)
        ent.grid(row=row, column=1)
        range_lbl = tk.Label(root, text=f"({min_val}-{max_val})", fg="gray")
        range_lbl.grid(row=row, column=2, sticky="w")
        entries[stype][key] = ent
        rows[stype].append((lbl, ent, range_lbl))
        ranges[stype][key] = (min_val, max_val)

# first grid row below the tallest set of fields — everything after this is
# positioned relative to it, so adding a field won't collide with the buttons
FIRST_FREE_ROW = max(len(f) for f in field_defs.values()) + 1


def show_fields():
    selected = step_type.get()
    for stype, widgets in rows.items():
        for lbl, ent, range_lbl in widgets:
            if stype == selected:
                lbl.grid()
                ent.grid()
                range_lbl.grid()
            else:
                lbl.grid_remove()
                ent.grid_remove()
                range_lbl.grid_remove()


def clear_fields():
    for entry in entries[step_type.get()].values():
        entry.delete(0, tk.END)


def encode_current():
    """Reads the currently visible fields, validates against ranges, and encodes them."""
    selected = step_type.get()
    values = {}
    for key, entry in entries[selected].items():
        val = int(entry.get())
        min_val, max_val = ranges[selected][key]
        if not (min_val <= val <= max_val):
            raise ValueError(f"{key} must be between {min_val} and {max_val}, got {val}")
        values[key] = val

    if selected == "AGITATION":
        return encode_agitation(**values)
    elif selected == "MOVING":
        return encode_moving(**values)
    else:
        return encode_pausing(**values)


# --- Protocol-building state ---
wells = []                 # list of well dicts: {"steps": [...], "moving_step": str or None}
current_well_steps = []    # steps for the well currently being built
protocol_finished = False  # True only after Finish Last Well, or loading a finished protocol


def label_step(s):
    if s.startswith("A"):
        return f"[Agitation] {s}"
    elif s.startswith("M"):
        return f"[Moving]    {s}"
    elif s.startswith("P"):
        return f"[Pausing]   {s}"
    return s

def update_ui():
    total_steps = sum(len(w["steps"]) + (1 if w["moving_step"] else 0) for w in wells)
    current_well_number = len(wells) + 1
    status_label.config(
        text=f"Well {current_well_number} of {EXPECTED_WELL_COUNT} | Steps in current well: {len(current_well_steps)} | Total steps: {total_steps}"
    )
    preview_text.config(state="normal")
    preview_text.delete("1.0", tk.END)

    lines = []
    for i, w in enumerate(wells):
        lines.append(f"--- well {i+1} ---")
        lines.extend(label_step(s) for s in w["steps"])
        if w["moving_step"]:
            lines.append(label_step(w["moving_step"]))
    if current_well_steps:
        lines.append("--- current well ---")
        lines.extend(label_step(s) for s in current_well_steps)

    preview_text.insert("1.0", "\n".join(lines))
    preview_text.config(state="disabled")


def add_step():
    global protocol_finished
    selected = step_type.get()
    if selected == "MOVING":
        result_label.config(text="Use 'Finish Well' to add the moving step, not 'Add Step'")
        return
    try:
        protocol_string = encode_current()
    except ValueError as e:
        result_label.config(text=str(e))
        return
    current_well_steps.append(protocol_string)
    protocol_finished = False
    result_label.config(text=f"Added to well: {protocol_string}")
    clear_fields()
    update_ui()
    send_button.config(state="disabled")


def delete_last_step():
    global protocol_finished
    if not current_well_steps:
        if wells:
            result_label.config(text="Nothing to delete in the current well — click a well below to edit a finished one")
        else:
            result_label.config(text="Nothing to delete")
        return
    removed = current_well_steps.pop()
    protocol_finished = False
    result_label.config(text=f"Removed: {removed}")
    update_ui()
    send_button.config(state="disabled")


def finish_well():
    global protocol_finished
    if step_type.get() != "MOVING":
        result_label.config(text="Select 'Moving' and fill in the transition fields to finish this well")
        return
    if not current_well_steps:
        result_label.config(text="Add at least one step before finishing the well")
        return
    try:
        moving_string = encode_current()
    except ValueError as e:
        result_label.config(text=str(e))
        return
    wells.append({"steps": list(current_well_steps), "moving_step": moving_string})
    current_well_steps.clear()
    protocol_finished = False
    result_label.config(text=f"Well {len(wells)} finished with moving step: {moving_string}")
    clear_fields()
    update_ui()
    send_button.config(state="disabled")


def finish_last_well():
    global protocol_finished
    if not current_well_steps:
        result_label.config(text="Add at least one step before finishing the protocol")
        return
    if step_type.get() == "MOVING":
        result_label.config(text="The last well doesn't take a moving step — there's nowhere to move to")
        return
    wells.append({"steps": list(current_well_steps), "moving_step": None})
    current_well_steps.clear()
    protocol_finished = True
    result_label.config(text=f"Well {len(wells)} finished — protocol complete, Send is now enabled")
    clear_fields()
    update_ui()
    send_button.config(state="normal")

def estimate_total_seconds(steps):
    """Rough estimate of run duration from encoded steps. Undercounts homing,
    motion, and clamping overhead — treat as a floor, not exact."""
    total = 0
    for s in steps: 
        if s.startswith("A"):
            duration = int(s[2:4])
            pausetime = int(s[10:12])
            repeats = int(s[12:14])
            total += repeats * (duration + pausetime)
        elif s.startswith("M"):
            initial_surface_time = int(s[1:4])
            total += initial_surface_time + 10  # rough buffer for the physical move itself
        elif s.startswith("P"):
            duration = int(s[1:2])
            total += duration
    return total


def send_protocol():
    if current_well_steps:
        result_label.config(text="Finish the current well before sending")
        return
    if not wells:
        result_label.config(text="Nothing to send yet")
        return
    if len(wells) != EXPECTED_WELL_COUNT:
        result_label.config(text=f"Warning: {len(wells)} wells built, expected {EXPECTED_WELL_COUNT} — sending anyway")

    flat_steps = []
    for w in wells:
        flat_steps.extend(w["steps"])
        if w["moving_step"]:
            flat_steps.append(w["moving_step"])

    full_text = "\r\n".join(flat_steps) + "\r\nEND"
    print("Sending full protocol:\n" + full_text)

    total_seconds = estimate_total_seconds(flat_steps)
    listen_seconds = max(60, total_seconds + 60)  # buffer so listening doesn't cut off a long run early

    start_time = time.time()
    running = {"active": True}

    def tick():
        if running["active"]:
            elapsed = int(time.time() - start_time)
            remaining = max(0, total_seconds - elapsed)
            timer_label.config(text=f"Running: {remaining // 60}:{remaining % 60:02d}")
            if remaining > 0:
                root.after(1000, tick)

    def worker():
        # time.sleep(5) #temporary, test visually
        try:
            send_and_listen(TERRY_PORT, BAUD_RATE, full_text, listen_seconds=listen_seconds)
        except Exception as e:
            result_label.config(text=f"Could not reach the board on {TERRY_PORT}")
            print(f"Serial error: {e}")
        finally:
            running["active"] = False
            timer_label.config(text="")

    tick()
    threading.Thread(target=worker, daemon=True).start()


def reset_protocol():
    global protocol_finished
    current_well_steps.clear()
    wells.clear()
    protocol_finished = False
    clear_fields()
    result_label.config(text="Protocol reset")
    update_ui()
    preview_text.config(state="normal")
    preview_text.delete("1.0", tk.END)
    preview_text.config(state="disabled")
    send_button.config(state="disabled")


# --- Save / load ---

def save_protocol():
    name = name_entry.get().strip()
    if not name:
        result_label.config(text="Enter a name to save under")
        return
    if not wells:
        result_label.config(text="Nothing to save yet")
        return

    os.makedirs(PROTOCOL_DIR, exist_ok=True)
    path = os.path.join(PROTOCOL_DIR, name + ".json")

    if os.path.exists(path):
        if not messagebox.askyesno("Overwrite?", f"'{name}' already exists. Overwrite it?"):
            result_label.config(text="Save cancelled")
            return

    data = {
        "name": name,
        "wells": wells,
        "finished": protocol_finished,
    }
    with open(path, "w") as f:
        json.dump(data, f, indent=2)

    total_steps = sum(len(w["steps"]) + (1 if w["moving_step"] else 0) for w in wells)
    result_label.config(text=f"Saved as '{name}' ({total_steps} steps)")
    refresh_protocol_list()
    name_entry.delete(0, tk.END)


def refresh_protocol_list():
    if not os.path.isdir(PROTOCOL_DIR):
        names = []
    else:
        names = sorted(f[:-5] for f in os.listdir(PROTOCOL_DIR) if f.endswith(".json"))
    menu = load_menu["menu"]
    menu.delete(0, "end")
    if not names:
        selected_protocol.set("(none saved)")
        load_menu.config(fg="#777777")
        load_button.config(state="disabled")
        delete_button.config(state="disabled")
        return
    for n in names:
        menu.add_command(label=n, command=lambda v=n: selected_protocol.set(v))
    if selected_protocol.get() not in names:
        selected_protocol.set(names[0])
    load_menu.config(fg="black")
    load_button.config(state="normal")
    delete_button.config(state="normal")


def load_protocol():
    global protocol_finished
    name = selected_protocol.get()
    path = os.path.join(PROTOCOL_DIR, name + ".json")
    if not os.path.exists(path):
        result_label.config(text=f"No saved protocol named '{name}'")
        return

    if wells or current_well_steps:
        if not messagebox.askyesno("Discard current?", "Loading will discard the protocol you're building. Continue?"):
            result_label.config(text="Load cancelled")
            return

    with open(path) as f:
        data = json.load(f)

    current_well_steps.clear()
    wells.clear()
    wells.extend(data["wells"])
    protocol_finished = data.get("finished", False)

    name_entry.delete(0, tk.END)
    name_entry.insert(0, data.get("name", name))

    clear_fields()
    update_ui()
    send_button.config(state="normal" if protocol_finished else "disabled")
    total_steps = sum(len(w["steps"]) + (1 if w["moving_step"] else 0) for w in wells)
    state_note = "ready to send" if protocol_finished else "not finished — press Finish Last Well"
    result_label.config(text=f"Loaded '{name}' — {total_steps} steps, {state_note}")


def delete_saved_protocol():
    name = selected_protocol.get()
    path = os.path.join(PROTOCOL_DIR, name + ".json")
    if not os.path.exists(path):
        result_label.config(text=f"No saved protocol named '{name}'")
        return
    if not messagebox.askyesno("Delete?", f"Permanently delete '{name}'? This can't be undone."):
        result_label.config(text="Delete cancelled")
        return
    os.remove(path)
    result_label.config(text=f"Deleted '{name}'")
    refresh_protocol_list()


# --- Save / load widgets ---
save_frame = tk.Frame(root)
save_frame.grid(row=FIRST_FREE_ROW + 5, column=0, columnspan=3, pady=(0, 10))

tk.Label(save_frame, text="Name:").pack(side="left")
name_entry = tk.Entry(save_frame, width=18)
name_entry.pack(side="left", padx=(2, 8))
tk.Button(save_frame, text="Save", command=save_protocol).pack(side="left", padx=2)

selected_protocol = tk.StringVar(value="(none saved)")
load_menu = tk.OptionMenu(save_frame, selected_protocol, "(none saved)")
load_menu.config(width=14)
load_menu.pack(side="left", padx=(12, 2))
load_button = tk.Button(save_frame, text="Load", command=load_protocol,
                        state="disabled", disabledforeground="#777777")
load_button.pack(side="left", padx=2)
delete_button = tk.Button(save_frame, text="✕", command=delete_saved_protocol,
                          width=2, state="disabled", disabledforeground="#777777")
delete_button.pack(side="left", padx=2)


# --- Action buttons ---
btn_frame = tk.Frame(root)
btn_frame.grid(row=FIRST_FREE_ROW, column=0, columnspan=3, pady=10)
tk.Button(btn_frame, text="Add Step", command=add_step).pack(side="left", padx=5)
tk.Button(btn_frame, text="Delete Last Step", command=delete_last_step).pack(side="left", padx=5)
tk.Button(btn_frame, text="Finish Well", command=finish_well).pack(side="left", padx=5)
tk.Button(btn_frame, text="Finish Last Well", command=finish_last_well).pack(side="left", padx=5)

send_button = tk.Button(
    btn_frame,
    text="Send Protocol",
    command=send_protocol,
    state="disabled",
    disabledforeground="#777777",
)
send_button.pack(side="left", padx=5)

tk.Button(btn_frame, text="Reset Protocol", command=lambda: reset_protocol()).pack(side="left", padx=5)


result_label = tk.Label(root, text="")
result_label.grid(row=FIRST_FREE_ROW + 1, column=0, columnspan=3)

status_label = tk.Label(root, text="", fg="gray")
status_label.grid(row=FIRST_FREE_ROW + 2, column=0, columnspan=3)

timer_label = tk.Label(root, text="", fg="gray")
timer_label.grid(row=FIRST_FREE_ROW + 3, column=0, columnspan=3)

tk.Label(root, text="Current protocol:", fg="gray").grid(row=FIRST_FREE_ROW + 4, column=0, columnspan=3, sticky="w", padx=10)

preview_text = tk.Text(root, height=6, width=40, state="disabled", fg="white", bg="black")
preview_text.grid(row=FIRST_FREE_ROW + 5, column=0, columnspan=3, padx=10, pady=(0, 10))
save_frame.grid(row=FIRST_FREE_ROW + 6, column=0, columnspan=3, pady=(0, 10))

show_fields()
update_ui()
refresh_protocol_list()
root.mainloop()