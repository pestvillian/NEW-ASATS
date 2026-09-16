import queue
import threading
import time

import serial


active_serial = None
_serial_lock = threading.Lock()
_reader_thread = None
_reader_stop = threading.Event()
_events = queue.Queue()


def connect(port, baud, boot_seconds=2):
    """Open one serial connection for the whole GUI session."""
    global active_serial, _reader_thread

    with _serial_lock:
        if active_serial is not None and active_serial.is_open:
            return True
        active_serial = serial.Serial(port=port, baudrate=baud, timeout=0.2)

    # Opening this board resets it.  Do not send a protocol while it homes.
    time.sleep(boot_seconds)
    _reader_stop.clear()
    _reader_thread = threading.Thread(target=_reader_loop, daemon=True)
    _reader_thread.start()
    _events.put(("connection", "CONNECTED"))
    return True


def disconnect():
    """Close the persistent connection when the GUI exits or Disconnect is used."""
    global active_serial
    _reader_stop.set()
    with _serial_lock:
        ser = active_serial
        active_serial = None
        if ser is not None and ser.is_open:
            ser.close()
    _events.put(("connection", "DISCONNECTED"))


def is_connected():
    with _serial_lock:
        return active_serial is not None and active_serial.is_open


def _reader_loop():
    global active_serial
    try:
        while not _reader_stop.is_set():
            with _serial_lock:
                ser = active_serial
            if ser is None or not ser.is_open:
                return
            line = ser.readline()
            if line:
                _events.put(("line", line.decode(errors="replace").rstrip()))
    except (serial.SerialException, OSError) as exc:
        _events.put(("connection", f"ERROR:{exc}"))
    finally:
        with _serial_lock:
            if active_serial is not None:
                try:
                    active_serial.close()
                except serial.SerialException:
                    pass
                active_serial = None


def get_events():
    """Return all pending reader events; call this only from Tk's main thread."""
    events = []
    while True:
        try:
            events.append(_events.get_nowait())
        except queue.Empty:
            return events


def send_text(message):
    """Send one command or a complete CR/LF-delimited protocol."""
    payload = (message.rstrip("\r\n") + "\r\n").encode("utf-8")
    with _serial_lock:
        if active_serial is None or not active_serial.is_open:
            return False
        try:
            active_serial.write(payload)
            active_serial.flush()
            return True
        except (serial.SerialException, OSError) as exc:
            _events.put(("connection", f"ERROR:{exc}"))
            return False


def send_stop_command():
    return send_text("STOP")


def send_pause_command():
    return send_text("PAUSE")


def send_resume_command():
    return send_text("RESUME")


# Backward-compatible names used by older GUI revisions.  They now send over
# the one persistent connection and intentionally do not open or close a port.
def send_and_listen(port, baud, message, listen_seconds=5):
    if not is_connected():
        connect(port, baud)
    if not send_text(message):
        raise serial.SerialException("Serial port is not connected")


def send_uart_text(port_name, baud_rate, message):
    if not is_connected():
        connect(port_name, baud_rate)
    return send_text(message)

