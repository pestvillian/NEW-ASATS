"""This is the file that will loop through the desired text file
then send the protocols to terry via UART"""


import time
import serial
import tkinter as tk
from tkinter import filedialog

def selectFile():
    """
    Opens a file selection dialog box using Tkinter and returns the path of the selected file.
    """
    # Create a hidden root window (to prevent an empty Tkinter window from appearing)
    root = tk.Tk()
    root.withdraw()
    
    # Open the file selection dialog
    file_path = filedialog.askopenfilename(
        title="Select a file",
        initialdir="/", # Start directory (e.g., user's home directory or the root)
        filetypes=(("All files", "*.*"), ("Text files", "*.txt"), ("CSV files", "*.csv"))
    )
    
    # Destroy the hidden root window
    root.destroy()
    
    return file_path





# Source - https://stackoverflow.com/a
# Posted by Jared Mackey, modified by community. See post 'Timeline' for change history
# Retrieved 2025-12-20, License - CC BY-SA 3.0

#path = '../test_dir/urmom.txt'
def sendProtocol(path):

    try:
        # Open the file in read mode ('r' is the default)
        with open(path, 'r') as file:
            # Loop through each line in the file
            for line in file:
                # Process the line here
                # print(line) 

                # Often, you'll want to strip leading/trailing whitespace,
                # especially the newline character ('\n') at the end of each line
                cleaned_line = line.strip()
                print(cleaned_line)

    except FileNotFoundError:
        print(f"Error: The file '{path}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

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


def main():
    #get te path from the user then send it
    path = selectFile()
    sendProtocol(path)

#define entry poiont
if __name__ == "__main__":
    main()
    
