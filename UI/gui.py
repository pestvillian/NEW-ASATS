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
PROTOCOL_DIR = "saved_protocols"

WELL_NUMBERS = [n for n in range(1, 13) if n != 7]  # 7 is not a real slot

root = tk.Tk()
root.title("ASATS - Protocol Builder")
root.geometry("1000x800")
root.update()

step_type = tk.StringVar(value="AGITATION")

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


# ============================================================
#  Layout: header (always visible) / content (screen swaps) / footer (always visible)
# ============================================================
header_frame = tk.Frame(root)
header_frame.pack(side="top", fill="x", pady=(8, 0))

content_frame = tk.Frame(root)
content_frame.pack(side="top", fill="both", expand=True)

footer_frame = tk.Frame(root)
footer_frame.pack(side="bottom", fill="x")

overview_frame = tk.Frame(content_frame)
editor_frame = tk.Frame(content_frame)


# --- Protocol-building state ---
# wells_data[n] = {"steps": [...], "moving_step": {...} or None}
# each step is {"type": "AGITATION"/"PAUSING"/"MOVING", "values": {...}}
wells_data = {n: {"steps": [], "moving_step": None} for n in WELL_NUMBERS}
active_well = None      # which well the editor screen is focused on
last_well_num = None    # which well is the protocol's terminus (no outgoing move)
editing_step_index = None  # index into active well's steps being edited, or None for "adding new"


def encode_step(step):
    t, v = step["type"], step["values"]
    if t == "AGITATION":
        return encode_agitation(**v)
    elif t == "MOVING":
        return encode_moving(**v)
    else:
        return encode_pausing(**v)


def describe_step(step):
    t, v = step["type"], step["values"]
    if t == "AGITATION":
        return (f"Agitation — speed {v['speed']}, {v['duration']}s, {v['volume']}mL, "
                f"{v['percent_volume']}%, pause {v['pausetime']}s, x{v['repeats']}")
    elif t == "MOVING":
        return f"Moving out — bead attach {v['initial_surface_time']}s, speed {v['speed']}"
    else:
        return f"Pausing — {v['duration']}s"


def label_step(encoded):
    if encoded.startswith("A"):
        return f"[Agitation] {encoded}"
    elif encoded.startswith("M"):
        return f"[Moving]    {encoded}"
    elif encoded.startswith("P"):
        return f"[Pausing]   {encoded}"
    return encoded


# ============================================================
#  Editor screen widgets
# ============================================================
editor_header = tk.Frame(editor_frame)
editor_header.grid(row=0, column=0, columnspan=3, sticky="w", pady=(10, 0), padx=10)
tk.Button(editor_header, text="\u2190 Back to Wells", command=lambda: show_overview()).pack(side="left")
editor_title = tk.Label(editor_header, text="", font=("Helvetica", 14, "bold"))
editor_title.pack(side="left", padx=(15, 0))

steps_list_frame = tk.Frame(editor_frame)
steps_list_frame.grid(row=1, column=0, columnspan=3, sticky="ew", padx=10, pady=(10, 10))

type_frame = tk.Frame(editor_frame)
type_frame.grid(row=2, column=0, columnspan=3, pady=10)
tk.Radiobutton(type_frame, text="Agitation", variable=step_type, value="AGITATION", command=lambda: show_fields()).pack(side="left")
tk.Radiobutton(type_frame, text="Moving", variable=step_type, value="MOVING", command=lambda: show_fields()).pack(side="left")
tk.Radiobutton(type_frame, text="Pausing", variable=step_type, value="PAUSING", command=lambda: show_fields()).pack(side="left")

entries = {}
rows = {}
ranges = {}

for stype, fields in field_defs.items():
    entries[stype] = {}
    rows[stype] = []
    ranges[stype] = {}
    for i, (label_text, key, min_val, max_val) in enumerate(fields):
        row = i + 3
        lbl = tk.Label(editor_frame, text=label_text)
        lbl.grid(row=row, column=0, sticky="e")
        ent = tk.Entry(editor_frame)
        ent.grid(row=row, column=1)
        range_lbl = tk.Label(editor_frame, text=f"({min_val}-{max_val})", fg="gray")
        range_lbl.grid(row=row, column=2, sticky="w")
        entries[stype][key] = ent
        rows[stype].append((lbl, ent, range_lbl))
        ranges[stype][key] = (min_val, max_val)

EDITOR_BTN_ROW = max(len(f) for f in field_defs.values()) + 3

editor_btn_frame = tk.Frame(editor_frame)
editor_btn_frame.grid(row=EDITOR_BTN_ROW, column=0, columnspan=3, pady=10)
tk.Button(editor_btn_frame, text="Add Step", command=lambda: add_step()).pack(side="left", padx=5)
tk.Button(editor_btn_frame, text="Delete Last Step", command=lambda: delete_last_step()).pack(side="left", padx=5)
tk.Button(editor_btn_frame, text="Cancel Edit", command=lambda: cancel_edit()).pack(side="left", padx=5)
tk.Button(editor_btn_frame, text="Finish Well", command=lambda: finish_well()).pack(side="left", padx=5)
tk.Button(editor_btn_frame, text="Set As Last Well", command=lambda: finish_last_well()).pack(side="left", padx=5)


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


def read_current_values():
    """Reads the currently visible fields, validates against ranges.
    Returns (type, values_dict). Raises ValueError."""
    selected = step_type.get()
    values = {}
    for key, entry in entries[selected].items():
        val = int(entry.get())
        min_val, max_val = ranges[selected][key]
        if not (min_val <= val <= max_val):
            raise ValueError(f"{key} must be between {min_val} and {max_val}, got {val}")
        values[key] = val
    return selected, values


# ============================================================
#  Overview screen widgets
# ============================================================
well_buttons = {}
well_grid = tk.Frame(overview_frame)
well_grid.pack(pady=20)

WELL_ROWS = [[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]]
for r, row_nums in enumerate(WELL_ROWS):
    for c, n in enumerate(row_nums):
        if n == 7:
            b = tk.Button(well_grid, text="7", width=6, height=2, state="disabled", disabledforeground="#555555")
        else:
            b = tk.Button(well_grid, text=str(n), width=6, height=2,
                          font=("Helvetica", 13), command=lambda n=n: show_editor(n))
            well_buttons[n] = b
        b.grid(row=r, column=c, padx=4, pady=4)

tk.Label(overview_frame, text="\u2713 = has steps or moving step   \u2691 = marked as last well",
         fg="gray").pack(pady=(0, 10))


# ============================================================
#  Header (always visible)
# ============================================================
status_label = tk.Label(header_frame, text="", fg="gray")
status_label.pack()
validation_label = tk.Label(header_frame, text="", fg="pink")
validation_label.pack()
timer_label = tk.Label(header_frame, text="", fg="pink", font=("Menlo", 18, "bold"))
timer_label.pack()


# ============================================================
#  Footer (always visible): preview, save/load, send/reset
# ============================================================
tk.Label(footer_frame, text="Current protocol:", fg="gray").pack(anchor="w", padx=10)
preview_text = tk.Text(footer_frame, height=8, fg="white", bg="black", state="disabled")
preview_text.pack(fill="x", padx=10, pady=(0, 5))

save_row = tk.Frame(footer_frame)
save_row.pack(pady=(0, 5))
tk.Label(save_row, text="Name:").pack(side="left")
name_entry = tk.Entry(save_row, width=18)
name_entry.pack(side="left", padx=(2, 8))
tk.Button(save_row, text="Save", command=lambda: save_protocol()).pack(side="left", padx=2)

selected_protocol = tk.StringVar(value="(none saved)")
load_menu = tk.OptionMenu(save_row, selected_protocol, "(none saved)")
load_menu.config(width=14)
load_menu.pack(side="left", padx=(12, 2))
load_button = tk.Button(save_row, text="Load", command=lambda: load_protocol(),
                        state="disabled", disabledforeground="#777777")
load_button.pack(side="left", padx=2)
delete_button = tk.Button(save_row, text="\u2715", command=lambda: delete_saved_protocol(),
                          width=2, state="disabled", disabledforeground="#777777")
delete_button.pack(side="left", padx=2)

action_row = tk.Frame(footer_frame)
action_row.pack(pady=(0, 10))
send_button = tk.Button(action_row, text="Send Protocol", command=lambda: send_protocol(),
                        state="disabled", disabledforeground="#777777")
send_button.pack(side="left", padx=5)
tk.Button(action_row, text="Reset Protocol", command=lambda: reset_protocol()).pack(side="left", padx=5)


# ============================================================
#  Screen switching
# ============================================================
def show_overview():
    editor_frame.pack_forget()
    overview_frame.pack(fill="both", expand=True)
    update_ui()


def show_editor(n):
    global active_well, editing_step_index
    active_well = n
    editing_step_index = None
    overview_frame.pack_forget()
    editor_frame.pack(fill="both", expand=True)
    step_type.set("AGITATION")
    show_fields()
    clear_fields()
    update_ui()


# ============================================================
#  Well/step editing logic
# ============================================================
def render_step_rows():
    for widget in steps_list_frame.winfo_children():
        widget.destroy()
    if active_well is None:
        return
    d = wells_data[active_well]
    for idx, step in enumerate(d["steps"]):
        row = tk.Frame(steps_list_frame)
        row.pack(fill="x", pady=1)
        prefix = "\u25b6 " if idx == editing_step_index else "   "  # ▶ marks the one being edited
        tk.Button(row, text=prefix + describe_step(step), anchor="w",
                  command=lambda i=idx: edit_step(i)).pack(side="left", fill="x", expand=True)
        tk.Button(row, text="\u2715", width=2, command=lambda i=idx: delete_step(i)).pack(side="left")
    if d["moving_step"] is not None:
        row = tk.Frame(steps_list_frame)
        row.pack(fill="x", pady=1)
        tk.Button(row, text="[Moving out] " + describe_step(d["moving_step"]), anchor="w",
                  command=lambda: edit_moving_step()).pack(side="left", fill="x", expand=True)
        tk.Button(row, text="\u2715", width=2, command=lambda: clear_moving_step()).pack(side="left")
    elif active_well == last_well_num:
        tk.Label(steps_list_frame, text="(terminal well \u2014 no outgoing move)", fg="gray").pack(anchor="w")
    if not d["steps"] and d["moving_step"] is None:
        tk.Label(steps_list_frame, text="(no steps yet)", fg="gray").pack(anchor="w")


def add_step():
    global editing_step_index
    if active_well is None:
        return
    selected = step_type.get()
    if selected == "MOVING":
        result_label_set("Use 'Finish Well' to set the moving step, not 'Add Step'")
        return
    try:
        t, values = read_current_values()
    except ValueError as e:
        result_label_set(str(e))
        return
    step = {"type": t, "values": values}
    steps = wells_data[active_well]["steps"]
    if editing_step_index is not None:
        steps[editing_step_index] = step
        result_label_set(f"Updated step {editing_step_index + 1} in well {active_well}")
        editing_step_index = None
    else:
        steps.append(step)
        result_label_set(f"Added to well {active_well}: {encode_step(step)}")
    clear_fields()
    update_ui()


def delete_last_step():
    if active_well is None:
        return
    steps = wells_data[active_well]["steps"]
    if not steps:
        result_label_set(f"No steps in well {active_well} to delete")
        return
    delete_step(len(steps) - 1)


def delete_step(idx):
    global editing_step_index
    steps = wells_data[active_well]["steps"]
    if not (0 <= idx < len(steps)):
        return
    removed = steps.pop(idx)
    if editing_step_index == idx:
        editing_step_index = None
        clear_fields()
    elif editing_step_index is not None and idx < editing_step_index:
        editing_step_index -= 1
    result_label_set(f"Removed step {idx + 1} from well {active_well}: {encode_step(removed)}")
    update_ui()


def edit_step(idx):
    global editing_step_index
    steps = wells_data[active_well]["steps"]
    if not (0 <= idx < len(steps)):
        return
    step = steps[idx]
    step_type.set(step["type"])
    show_fields()
    clear_fields()
    for key, val in step["values"].items():
        entries[step["type"]][key].insert(0, str(val))
    editing_step_index = idx
    result_label_set(f"Editing step {idx + 1} of well {active_well} \u2014 change values, then press Add Step")
    update_ui()


def cancel_edit():
    global editing_step_index
    editing_step_index = None
    clear_fields()
    result_label_set("Edit cancelled")
    update_ui()


def edit_moving_step():
    d = wells_data[active_well]["moving_step"]
    if d is None:
        return
    step_type.set("MOVING")
    show_fields()
    clear_fields()
    for key, val in d["values"].items():
        entries["MOVING"][key].insert(0, str(val))
    result_label_set(f"Editing moving step of well {active_well} \u2014 change values, then press Finish Well")
    update_ui()


def clear_moving_step():
    wells_data[active_well]["moving_step"] = None
    result_label_set(f"Cleared moving step for well {active_well}")
    update_ui()


def finish_well():
    global last_well_num
    if active_well is None:
        return
    if step_type.get() != "MOVING":
        result_label_set("Select 'Moving' and fill in the transition fields to set this well's outgoing move")
        return
    try:
        t, values = read_current_values()
    except ValueError as e:
        result_label_set(str(e))
        return
    wells_data[active_well]["moving_step"] = {"type": "MOVING", "values": values}
    if active_well == last_well_num:
        last_well_num = None
    result_label_set(f"Well {active_well} moving step set")
    clear_fields()
    update_ui()


def finish_last_well():
    global last_well_num
    if active_well is None:
        return
    wells_data[active_well]["moving_step"] = None
    last_well_num = active_well
    result_label_set(f"Well {active_well} marked as the last well \u2014 protocol terminates here")
    clear_fields()
    update_ui()


# ============================================================
#  Validation / building the flat protocol
# ============================================================
def is_protocol_valid():
    if last_well_num is None:
        return False
    for n in WELL_NUMBERS:
        if n > last_well_num or n == last_well_num:
            continue
        if wells_data[n]["moving_step"] is None:
            return False
    return True


def build_flat_steps():
    """Walk wells 1..last_well_num, collecting encoded steps.
    Returns (flat_steps, missing_well)."""
    flat = []
    for n in WELL_NUMBERS:
        if last_well_num is None or n > last_well_num:
            break
        d = wells_data[n]
        for step in d["steps"]:
            flat.append(encode_step(step))
        if n == last_well_num:
            continue
        if d["moving_step"] is None:
            return flat, n
        flat.append(encode_step(d["moving_step"]))
    return flat, None


def validation_message():
    if last_well_num is None:
        return "Mark a well as the last well to complete the protocol"
    _, missing = build_flat_steps()
    if missing is not None:
        return f"Well {missing} needs a moving step before you can send"
    return "Protocol complete \u2014 ready to send"


# ============================================================
#  Result label helper (shows message wherever visible)
# ============================================================
result_label = tk.Label(header_frame, text="")
result_label.pack()


def result_label_set(text):
    result_label.config(text=text)


# ============================================================
#  Main UI refresh
# ============================================================
def update_ui():
    send_button.config(state="normal" if is_protocol_valid() else "disabled")
    validation_label.config(text=validation_message())

    for n, b in well_buttons.items():
        d = wells_data[n]
        has_data = bool(d["steps"]) or d["moving_step"] is not None or n == last_well_num
        label = str(n)
        if n == last_well_num:
            label += " \u2691"
        elif has_data:
            label += " \u2713"
        b.config(text=label)

    if active_well is not None:
        d = wells_data[active_well]
        editor_title.config(text=f"Well {active_well}")
        status_label.config(
            text=f"Well {active_well} \u2014 {len(d['steps'])} step(s), "
                 f"moving step: {'set' if d['moving_step'] else 'not set'}"
        )
    else:
        status_label.config(text="Click a well to begin")

    render_step_rows()

    flat, _ = build_flat_steps()
    preview_text.config(state="normal")
    preview_text.delete("1.0", tk.END)
    lines = []
    for n in WELL_NUMBERS:
        d = wells_data[n]
        if not d["steps"] and d["moving_step"] is None and n != last_well_num:
            continue
        marker = "  <-- editing" if n == active_well else ""
        lines.append(f"--- well {n}{marker} ---")
        lines.extend(label_step(encode_step(s)) for s in d["steps"])
        if d["moving_step"]:
            lines.append(label_step(encode_step(d["moving_step"])))
        elif n == last_well_num:
            lines.append("(terminal well \u2014 no outgoing move)")
    preview_text.insert("1.0", "\n".join(lines))
    preview_text.config(state="disabled")


# ============================================================
#  Send
# ============================================================
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
    if not is_protocol_valid():
        result_label_set(validation_message())
        return

    flat_steps, _ = build_flat_steps()
    if not flat_steps:
        result_label_set("Nothing to send yet")
        return

    full_text = "\r\n".join(flat_steps) + "\r\nEND"
    print("Sending full protocol:\n" + full_text)

    total_seconds = estimate_total_seconds(flat_steps)
    listen_seconds = max(60, total_seconds + 60)

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
        time.sleep(5) # test timer
        try:
            send_and_listen(TERRY_PORT, BAUD_RATE, full_text, listen_seconds=listen_seconds)
            result_label_set("Protocol sent and completed")
        except Exception as e:
            result_label_set(f"Could not reach the board on {TERRY_PORT}")
            print(f"Serial error: {e}")
        finally:
            running["active"] = False
            timer_label.config(text="")

    tick()
    threading.Thread(target=worker, daemon=True).start()


def reset_protocol():
    global active_well, last_well_num, editing_step_index
    for n in WELL_NUMBERS:
        wells_data[n] = {"steps": [], "moving_step": None}
    active_well = None
    last_well_num = None
    editing_step_index = None
    show_overview()
    result_label_set("Protocol reset")
    update_ui()


# ============================================================
#  Save / load
# ============================================================
def save_protocol():
    name = name_entry.get().strip()
    if not name:
        result_label_set("Enter a name to save under")
        return
    has_any = any(wells_data[n]["steps"] or wells_data[n]["moving_step"] for n in WELL_NUMBERS)
    if not has_any and last_well_num is None:
        result_label_set("Nothing to save yet")
        return

    os.makedirs(PROTOCOL_DIR, exist_ok=True)
    path = os.path.join(PROTOCOL_DIR, name + ".json")

    if os.path.exists(path):
        if not messagebox.askyesno("Overwrite?", f"'{name}' already exists. Overwrite it?"):
            result_label_set("Save cancelled")
            return

    data = {
        "name": name,
        "wells": {str(n): wells_data[n] for n in WELL_NUMBERS},
        "last_well_num": last_well_num,
    }
    with open(path, "w") as f:
        json.dump(data, f, indent=2)

    flat, _ = build_flat_steps()
    result_label_set(f"Saved as '{name}' ({len(flat)} steps)")
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
    global active_well, last_well_num, editing_step_index
    name = selected_protocol.get()
    path = os.path.join(PROTOCOL_DIR, name + ".json")
    if not os.path.exists(path):
        result_label_set(f"No saved protocol named '{name}'")
        return

    has_any = any(wells_data[n]["steps"] or wells_data[n]["moving_step"] for n in WELL_NUMBERS)
    if has_any or last_well_num is not None:
        if not messagebox.askyesno("Discard current?", "Loading will discard the protocol you're building. Continue?"):
            result_label_set("Load cancelled")
            return

    with open(path) as f:
        data = json.load(f)

    for n in WELL_NUMBERS:
        loaded = data["wells"].get(str(n))
        wells_data[n] = loaded if loaded is not None else {"steps": [], "moving_step": None}
    last_well_num = data.get("last_well_num")
    active_well = None
    editing_step_index = None

    name_entry.delete(0, tk.END)
    name_entry.insert(0, data.get("name", name))

    show_overview()
    flat, missing = build_flat_steps()
    if missing is not None:
        result_label_set(f"Loaded '{name}' \u2014 {len(flat)} steps, well {missing} still needs a moving step")
    else:
        state_note = "ready to send" if is_protocol_valid() else "not finished"
        result_label_set(f"Loaded '{name}' \u2014 {len(flat)} steps, {state_note}")


def delete_saved_protocol():
    name = selected_protocol.get()
    path = os.path.join(PROTOCOL_DIR, name + ".json")
    if not os.path.exists(path):
        result_label_set(f"No saved protocol named '{name}'")
        return
    if not messagebox.askyesno("Delete?", f"Permanently delete '{name}'? This can't be undone."):
        result_label_set("Delete cancelled")
        return
    os.remove(path)
    result_label_set(f"Deleted '{name}'")
    refresh_protocol_list()


# ============================================================
#  Start on the overview screen
# ============================================================
show_fields()
show_overview()
refresh_protocol_list()
update_ui()
root.mainloop()