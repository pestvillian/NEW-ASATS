import serial
import time

active_serial = None
stop_flag = {"stopped": False}

def send_and_listen(port, baud, message, listen_seconds=5):
    global active_serial
    stop_flag["stopped"] = False
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            active_serial = ser
            print(f"Serial port {port} opened at {baud} baud.")
            time.sleep(2)
            ser.write((message + "\r\n").encode())
            print("Sent. Listening...\n")

            deadline = time.time() + listen_seconds
            board_paused = False
            while time.time() < deadline or board_paused:
                if stop_flag["stopped"]:
                    break
                line = ser.readline()
                if line:
                    decoded = line.decode(errors="replace").rstrip()
                    print(decoded)
                    if "PAUSED" in decoded:
                        board_paused = True
                    elif "RESUMING" in decoded:
                        board_paused = False
                        deadline = time.time() + listen_seconds  # give it a fresh window post-resume
            print("\n--- done listening ---")
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    finally:
        active_serial = None


def send_stop_command():
    global active_serial
    if active_serial is not None and active_serial.is_open:
        try:
            active_serial.write(b"STOP\r\n")
            stop_flag["stopped"] = True
            return True
        except Exception as e:
            print(f"Failed to send STOP: {e}")
            return False
    return False

def send_pause_command():
    global active_serial
    if active_serial is not None and active_serial.is_open:
        try:
            active_serial.write(b"PAUSE\r\n")
            return True
        except Exception as e:
            print(f"Failed to send PAUSE: {e}")
            return False
    return False


def send_resume_command():
    global active_serial
    if active_serial is not None and active_serial.is_open:
        try:
            active_serial.write(b"RESUME\r\n")
            return True
        except Exception as e:
            print(f"Failed to send RESUME: {e}")
            return False
    return False

def send_uart_text(port_name, baud_rate, message):
    """
    Opens a serial port, sends a text message, and closes the port.

    :param port_name: The name of the serial port (e.g., "COM3" on Windows, 
                      "/dev/ttyUSB0" on Linux, "/dev/ttyACM0" on Raspberry Pi).
    :param baud_rate: The baud rate for the communication (must match the device).
    :param message: The text message string to send.
    """
    try:
        ser = serial.Serial(port=port_name, baudrate=baud_rate, timeout=1)
        time.sleep(2)

        print(f"Serial port {port_name} opened successfully at {baud_rate} baud.")

        data_to_send = (message + '\r\n').encode('utf-8') 
        ser.write(data_to_send)
        print(f"Sent message: '{message}'")

        time.sleep(0.1)
        if ser.in_waiting > 0:
            response_bytes = ser.readline()
            response_string = response_bytes.decode('utf-8').strip()
            print(f"Received response: {response_string}")

    except serial.SerialException as e:
        print(f"Error opening or communicating with serial port: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Serial port {port_name} closed.")