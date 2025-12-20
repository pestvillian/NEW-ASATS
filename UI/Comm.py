"""This is the file that will loop through the desired text file
then send the protocols to terry via UART"""



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

def main():
    #get te path from the user then send it
    path = selectFile()
    sendProtocol(path)

#define entry poiont
if __name__ == "__main__":
    main()
    
