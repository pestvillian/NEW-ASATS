import serial
import time

def send_and_listen(port, baud, message, listen_seconds=5):
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            print(f"Serial port {port} opened at {baud} baud.")
            time.sleep(2)
            ser.write((message + "\r\n").encode())
            print("Sent. Listening...\n")

            deadline = time.time() + listen_seconds
            while time.time() < deadline:
                line = ser.readline()
                if line:
                    print(line.decode(errors="replace").rstrip())
            print("\n--- done listening ---")
    except serial.SerialException as e:
        print(f"Serial error: {e}")

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