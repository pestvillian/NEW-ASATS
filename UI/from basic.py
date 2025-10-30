import kivy
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.gridlayout import GridLayout
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.scrollview import ScrollView
from kivy.graphics import Color, Rectangle
from kivy.uix.popup import Popup
from kivy.uix.filechooser import FileChooserListView
from kivy.uix.anchorlayout import AnchorLayout

import qrcode
import os

hiddenimports = ['win32timezone']  # Fix for pyinstaller packaging issues on Windows

# =====================================================================
# Custom TextInput class — used for numeric input fields with character limits
# =====================================================================
class LimitedInput(TextInput):
    __events__ = ('on_shift_enter',)  # Custom event for shift+enter
    max_chars = 3  # Default max character length

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.multiline = False
        # When focus changes (user leaves the input), trigger padding logic
        self.bind(focus=self.on_focus_change)

    def insert_text(self, substring, from_undo=False):
        # Allow only digits
        s = ''.join([c for c in substring if c.isdigit()])
        # Enforce max character limit
        if len(self.text + s) > self.max_chars:
            s = s[:self.max_chars - len(self.text)]
        return super().insert_text(s, from_undo=from_undo)

    def keyboard_on_key_down(self, window, keycode, text, modifiers):
        # Handle Enter vs Shift+Enter navigation
        if keycode[1] == 'enter':
            if 'shift' in modifiers:
                self.dispatch('on_shift_enter')  # Move to previous input
            else:
                self.dispatch('on_text_validate')  # Move to next input
            return True
        return super().keyboard_on_key_down(window, keycode, text, modifiers)

    def on_shift_enter(self):
        pass  # Placeholder event, will be connected externally

    def on_focus_change(self, instance, focused):
        # When focus leaves the field, zero-pad shorter numeric inputs
        if not focused and self.text.isdigit() and len(self.text) < self.max_chars:
            self.text = self.text.zfill(self.max_chars)


# =====================================================================
# Agitation Step Widget — represents one agitation step inside a well
# =====================================================================
class AgitationStep(BoxLayout):
    def __init__(self, well_number, step_number, controller, **kwargs):
        super().__init__(**kwargs)
        self.orientation = 'vertical'
        self.size_hint_y = None
        self.bind(minimum_height=self.setter('height'))
        self.controller = controller
        self.well_number = well_number
        self.step_number = step_number

        # Grid for label/input pairs
        self.grid = GridLayout(cols=2, size_hint_y=40, padding=10,spacing=90)
        self.grid.bind(minimum_height=self.grid.setter('height'))
        self.label_widgets = {}
        self.inputs = []

        # Helper function to create labeled LimitedInput fields
        def add_labeled_input(field_name, max_chars):
            label = Label()
            self.grid.add_widget(label)
            ti = LimitedInput()
            # Dynamically choose placeholder based on expected input width
            if max_chars == 3:
                ti.hint_text = "###"
            elif (max_chars != 3) and (max_chars != 1):
                ti.hint_text = "##"
            else:
                ti.hint_text = "#"
            ti.max_chars = max_chars
            # Keyboard navigation between fields
            ti.bind(on_text_validate=self.move_focus_to_next)
            ti.bind(on_shift_enter=self.move_focus_to_previous)
            self.grid.add_widget(ti)
            self.label_widgets[field_name] = label
            self.inputs.append(ti)
            return ti

        # Input fields for agitation parameters
        self.speed = add_labeled_input("Agitation Speed", 1)
        self.duration = add_labeled_input("Agitation Duration", 2)
        self.totalVolume = add_labeled_input("Total Volume", 3)
        self.percentVolume = add_labeled_input("Percent Volume", 3)
        self.pauseDuration = add_labeled_input("Pause Duration", 2)
        self.numRepeats = add_labeled_input("Number of Repeats", 2)

        self.add_widget(self.grid)

        # Buttons for managing the step
        button_bar = BoxLayout(size_hint_y=None, height=30,padding=5)
        del_btn = Button(text="Delete")
        copy_btn = Button(text="Copy")
        paste_btn = Button(text="Paste")

        # Hook buttons to controller logic
        del_btn.bind(on_press=lambda x: controller.delete_step(self))
        # NOTE: controller here is the WellBlock; its .controller is MyGridLayout
        copy_btn.bind(on_press=lambda x: controller.controller.clipboard_well(self.get_data_tuple()))
        paste_btn.bind(on_press=lambda x: controller.controller.clipboard_paste(self))

        button_bar.add_widget(del_btn)
        button_bar.add_widget(copy_btn)
        button_bar.add_widget(paste_btn)
        self.add_widget(button_bar)

        self.update_labels()

    # Returns all field data as a tuple
    def get_data_tuple(self):
        return (
            self.speed.text,
            self.duration.text,
            self.totalVolume.text,
            self.percentVolume.text,
            self.pauseDuration.text,
            self.numRepeats.text,
        )

    # Fills the inputs from a tuple (used in paste)
    def set_data_tuple(self, data):
        self.speed.text, self.duration.text, self.totalVolume.text, self.percentVolume.text, self.pauseDuration.text, self.numRepeats.text = data

    # Refresh label text to show which well/step each belongs to
    def update_labels(self):
        for field, label in self.label_widgets.items():
            label.text = f"{field} (Well {self.well_number} - Step {self.step_number})"

    # Keyboard navigation helpers
    def move_focus_to_next(self, instance):
        idx = self.inputs.index(instance)
        if idx < len(self.inputs) - 1:
            self.inputs[idx + 1].focus = True
        else:
            next_step = self.controller.get_next_step(self)
            if next_step:
                next_step.inputs[0].focus = True

    def move_focus_to_previous(self, instance):
        idx = self.inputs.index(instance)
        if idx > 0:
            self.inputs[idx - 1].focus = True
        else:
            prev_step = self.controller.get_previous_step(self)
            if prev_step:
                prev_step.inputs[-1].focus = True

    # Converts this step into a single protocol line
    def get_step_data(self):
        return f"B{self.speed.text}{self.duration.text}{self.totalVolume.text}{self.percentVolume.text}{self.pauseDuration.text}{self.numRepeats.text}"

    # Validation logic to ensure reasonable input ranges
    def validate_ranges(self):
        try:
            total = int(self.totalVolume.text)
            percent = int(self.percentVolume.text)
            if not (1 <= total <= 100):
                return False, "Total Volume must be between 001 and 100"
            if not (1 <= percent <= 100):
                return False, "Percent Volume must be between 001 and 100"
        except ValueError:
            return False, "Invalid input in Total or Percent Volume"
        return True, ""




class InputSaverApp(App):
    def build(self):
        # Create the main layout
        main_layout = BoxLayout(orientation='vertical', padding=10, spacing=10)

        # Create a TextInput widget for user input
        self.text_input = TextInput(hint_text="Enter text here...", multiline=True)
        main_layout.add_widget(self.text_input)

        # Create a Button to save the input
        save_button = Button(text="Save to File")
        save_button.bind(on_release=self.save_to_file)
        main_layout.add_widget(save_button)

        # Create a Label to display status messages
        self.status_label = Label(text="")
        main_layout.add_widget(self.status_label)

        return main_layout

    def save_to_file(self, instance):
        input_text = self.text_input.text
        if input_text:
            try:
                with open("output.txt", "a") as f: # "a" for append mode
                    f.write(input_text + "\n")
                self.status_label.text = "Text saved successfully to output.txt!"
                self.text_input.text = "" # Clear the input field
            except Exception as e:
                self.status_label.text = f"Error saving file: {e}"
        else:
            self.status_label.text = "Please enter some text to save."

if __name__ == '__main__':
    InputSaverApp().run()