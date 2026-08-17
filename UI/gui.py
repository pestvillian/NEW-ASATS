import tkinter as tk
from string_encoder import encode_agitation, encode_moving, encode_pausing
from Send_UART import send_uart_text, send_and_listen

TERRY_PORT = "/dev/cu.usbmodem2101"   # TODO: confirm actual port on lab desktop
BAUD_RATE = 9600
EXPECTED_WELL_COUNT = 10  # TODO: confirm with Gamze/Greg — not fully confirmed yet

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
        ("Duration", "duration", 0, 99),
        ("Volume", "volume", 0, 9999),          # 4-digit field
        ("Percent Volume", "percent_volume", 1, 99),   # 2-digit field — see note
        ("Pause Time", "pausetime", 0, 99),
        ("Repeats", "repeats", 0, 99),
    ],
    "MOVING": [
        ("Initial Surface Time", "initial_surface_time", 0, 999),
        ("Speed", "speed", 0, 9),
        ("Stop At Sequences", "stop_at_sequences", 0, 9),
        ("Sequence Pause Time", "sequence_pause_time", 0, 99),
    ],
    "PAUSING": [
        ("Duration", "duration", 0, 9),
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
current_well = []    # steps added to the well currently being built
full_protocol = []   # all completed wells' steps, in final send order
well_count = 0

def update_ui():
    current_well_number = well_count + 1
    status_label.config(
        text=f"Well {current_well_number} of {EXPECTED_WELL_COUNT} | Steps in current well: {len(current_well)} | Total steps: {len(full_protocol)}"
    )
    preview_text.config(state="normal")
    preview_text.delete("1.0", tk.END)
    preview = full_protocol + (["--- current well ---"] + current_well if current_well else [])
    preview_text.insert("1.0", "\n".join(preview))
    preview_text.config(state="disabled")


def add_step():
    selected = step_type.get()
    if selected == "MOVING":
        result_label.config(text="Use 'Finish Well' to add the moving step, not 'Add Step'")
        return
    try:
        protocol_string = encode_current()
    except ValueError as e:
        result_label.config(text=str(e))
        return
    current_well.append(protocol_string)
    result_label.config(text=f"Added to well: {protocol_string}")
    clear_fields()
    update_ui()
    send_button.config(state="disabled")

def delete_last_step():
    global protocol_finished
    if not current_well:
        if full_protocol:
            result_label.config(text="Nothing to delete in the current well — finished wells can't be edited, use Reset")
        else:
            result_label.config(text="Nothing to delete")
        return
    removed = current_well.pop()
    protocol_finished = False
    send_button.config(state="disabled")
    result_label.config(text=f"Removed: {removed}")
    update_ui()

def finish_well():
    global well_count
    if step_type.get() != "MOVING":
        result_label.config(text="Select 'Moving' and fill in the transition fields to finish this well")
        return
    if not current_well:
        result_label.config(text="Add at least one step before finishing the well")
        return
    try:
        moving_string = encode_current()
    except ValueError as e:
        result_label.config(text=str(e))
        return
    full_protocol.extend(current_well)
    full_protocol.append(moving_string)
    current_well.clear()
    well_count += 1
    result_label.config(text=f"Well {well_count} finished with moving step: {moving_string}")
    clear_fields()
    update_ui()
    send_button.config(state="disabled")

def finish_last_well():
    global well_count
    if not current_well:
        result_label.config(text="Add at least one step before finishing the protocol")
        return
    if step_type.get() == "MOVING":
        result_label.config(text="The last well doesn't take a moving step — there's nowhere to move to")
        return
    full_protocol.extend(current_well)
    current_well.clear()
    well_count += 1
    result_label.config(text=f"Well {well_count} finished — protocol complete, Send is now enabled")
    clear_fields()
    update_ui()
    send_button.config(state="normal")

def send_protocol():
    if current_well:
        result_label.config(text="Finish the current well before sending")
        return
    if not full_protocol:
        result_label.config(text="Nothing to send yet")
        return
    if well_count != EXPECTED_WELL_COUNT:
        result_label.config(text=f"Warning: {well_count} wells built, expected {EXPECTED_WELL_COUNT} — sending anyway")
    full_text = "\r\n".join(full_protocol) + "\r\nEND"
    print("Sending full protocol:\n" + full_text)
    send_and_listen(TERRY_PORT, BAUD_RATE, full_text, listen_seconds=300)

def reset_protocol():
    global well_count
    current_well.clear()
    full_protocol.clear()
    well_count = 0
    clear_fields()
    result_label.config(text="Protocol reset")
    update_ui()
    preview_text.config(state="normal")
    preview_text.delete("1.0", tk.END)
    preview_text.config(state="disabled")
    send_button.config(state="disabled")

btn_frame = tk.Frame(root)
btn_frame.grid(row=7, column=0, columnspan=3, pady=10)
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
result_label.grid(row=8, column=0, columnspan=3)

status_label = tk.Label(root, text="", fg="gray")
status_label.grid(row=9, column=0, columnspan=3)

tk.Label(root, text="Current protocol:", fg="gray").grid(row=10, column=0, columnspan=3, sticky="w", padx=10)
preview_text = tk.Text(root, height=6, width=40, state="disabled", fg="white", bg="black")
preview_text.grid(row=11, column=0, columnspan=3, padx=10, pady=(0, 10))

show_fields()
update_ui()
root.mainloop()
