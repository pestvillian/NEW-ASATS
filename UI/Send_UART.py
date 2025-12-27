import serial
import time

def send_uart_text(port_name, baud_rate, message):
    """
    Opens a serial port, sends a text message, and closes the port.

    :param port_name: The name of the serial port (e.g., "COM3" on Windows, 
                      "/dev/ttyUSB0" on Linux, "/dev/ttyACM0" on Raspberry Pi).
    :param baud_rate: The baud rate for the communication (must match the device).
    :param message: The text message string to send.
    """
    try:
        # Open the serial port
        # The timeout ensures the program doesn't hang indefinitely if there's no response
        ser = serial.Serial(port=port_name, baudrate=baud_rate, timeout=1)
        time.sleep(2) # Give the port time to connect and initialize

        print(f"Serial port {port_name} opened successfully at {baud_rate} baud.")

        # Encode the message string to bytes and send it
        # You might need to add a newline or carriage return character 
        # depending on what the receiving device expects
        data_to_send = (message + '\r\n').encode('utf-8') 
        ser.write(data_to_send)
        print(f"Sent message: '{message}'")

        # Optional: Read response if the device sends one back
        time.sleep(0.1) # Small delay to allow response
        if ser.in_waiting > 0:
            response_bytes = ser.readline()
            response_string = response_bytes.decode('utf-8').strip()
            print(f"Received response: {response_string}")

    except serial.SerialException as e:
        print(f"Error opening or communicating with serial port: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
    finally:
        # Close the serial port if it was opened
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Serial port {port_name} closed.")

if __name__ == "__main__":
    # --- Configuration ---
    SERIAL_PORT = "COM3"  # Change to your specific port name
    BAUD_RATE = 9600      # Change to match your device's baud rate
    MESSAGE = "Hello, UART!\n"
    # ---------------------

    send_uart_text(SERIAL_PORT, BAUD_RATE, MESSAGE)
