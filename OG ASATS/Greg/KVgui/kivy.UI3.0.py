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

hiddenimports=['win32timezone'] # for pyinstaller crashing


# ----------------------------- #
# Custom TextInput with Char Limit and Keyboard Handling
# ----------------------------- #

class LimitedInput(TextInput):
    __events__ = ('on_shift_enter',)
    max_chars = 3

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.multiline = False
        self.bind(focus=self.on_focus_change)  # Bind focus to detect when user leaves the field

    def insert_text(self, substring, from_undo=False):
        s = ''.join([c for c in substring if c.isdigit()])
        if len(self.text + s) > self.max_chars:
            s = s[:self.max_chars - len(self.text)]
        return super().insert_text(s, from_undo=from_undo)

    def keyboard_on_key_down(self, window, keycode, text, modifiers):
        if keycode[1] == 'enter':
            if 'shift' in modifiers:
                self.dispatch('on_shift_enter')
            else:
                self.dispatch('on_text_validate')
            return True
        return super().keyboard_on_key_down(window, keycode, text, modifiers)

    def on_shift_enter(self):
        pass

    def on_focus_change(self, instance, focused):
        if not focused:
            # Field was exited, apply zero-padding if needed
            if self.text.isdigit() and len(self.text) < self.max_chars:
                self.text = self.text.zfill(self.max_chars)



# ----------------------------- #
# Agitation Step Widget
# ----------------------------- #

class AgitationStep(BoxLayout):
    def __init__(self, well_number, step_number, controller, **kwargs):
        super().__init__(**kwargs)
        self.orientation = 'vertical'
        self.size_hint_y = None
        self.height = 320
        self.controller = controller
        self.well_number = well_number
        self.step_number = step_number

        self.grid = GridLayout(cols=2, size_hint_y=None, height=260)
        self.label_widgets = {}
        self.inputs = []

        def add_labeled_input(field_name, max_chars):
            label = Label()
            self.grid.add_widget(label)
            ti = LimitedInput()
            #determine hint text based on max num inputs
            if max_chars == 3:
                ti.hint_text = "###"
            elif (max_chars != 3 )and (max_chars != 1):
                ti.hint_text = "##"
            else:
                ti.hint_text = "#"
            #ti.hint_text = "###" if max_chars == 3 else "#" # i'm seeing if can make this more precise here
            ti.max_chars = max_chars
            ti.bind(on_text_validate=self.move_focus_to_next)
            ti.bind(on_shift_enter=self.move_focus_to_previous)
            self.grid.add_widget(ti)
            self.label_widgets[field_name] = label
            self.inputs.append(ti)
            return ti

        self.speed = add_labeled_input("Agitation Speed", 1)
        self.duration = add_labeled_input("Agitation Duration", 2)
        self.totalVolume = add_labeled_input("Total Volume", 3)
        self.percentVolume = add_labeled_input("Percent Volume", 3)
        self.pauseDuration = add_labeled_input("Pause Duration", 2)
        self.numRepeats = add_labeled_input("Number of Repeats", 2)

        self.add_widget(self.grid)
        #creating the buttons
        button_bar = BoxLayout(size_hint_y=None, height=40)
        del_btn = Button(text="Delete")
        copy_btn = Button(text="Copy")
        paste_btn = Button (text="Paste")
        #binding the buttons to functions
        del_btn.bind(on_press=lambda x: controller.delete_step(self))
        copy_btn.bind(on_press=lambda x: controller.controller.clipboard_set(self.get_data_tuple()))
        paste_btn.bind(on_press=lambda x: controller.controller.clipboard_paste(self))
        #adding the widgets
        button_bar.add_widget(del_btn)
        button_bar.add_widget(copy_btn)
        button_bar.add_widget(paste_btn)
        self.add_widget(button_bar)


        self.update_labels()
        
    def get_data_tuple(self):
        return (
            self.speed.text,
            self.duration.text,
            self.totalVolume.text,
            self.percentVolume.text,
            self.pauseDuration.text,
            self.numRepeats.text,
        )

    def set_data_tuple(self, data):
        self.speed.text, self.duration.text, self.totalVolume.text, self.percentVolume.text, self.pauseDuration.text, self.numRepeats.text = data


    def update_labels(self):
        for field, label in self.label_widgets.items():
            label.text = f"{field} (Well {self.well_number} - Step {self.step_number})"

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

    def get_step_data(self):
        return f"B{self.speed.text}{self.duration.text}{self.totalVolume.text}{self.percentVolume.text}{self.pauseDuration.text}{self.numRepeats.text}"

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


# ----------------------------- #
# Moving Step Widget
# ----------------------------- #

class MovingStep(BoxLayout):
    def __init__(self, well_number, controller, **kwargs):
        super().__init__(**kwargs)
        self.controller = controller  # <-- Add this line

        self.orientation = 'vertical'

        self.size_hint_y = None
        self.height = 200
        self.inputs = {}


        grid = GridLayout( cols=2, size_hint_y=None, height=160)
        self.add_widget(Label(text=f"Moving Step for Well {well_number}", size_hint_y=None, height=20))
        self.add_widget(BoxLayout(size_hint_y=20, height=60))  # Adds visual spacing below the label


        def add_input(label_text, max_chars):
            grid.add_widget(Label(text=label_text, size_hint_y = 40, height=20))
  
            ti = LimitedInput(hint_text="###" if max_chars == 3 else "#", multiline=False)

            ti.max_chars = max_chars
            grid.add_widget(ti)
            self.inputs[label_text] = ti

        add_input("Initial Pause", 3)
        add_input("Speed", 1)
        add_input("Number of Segments", 1)
        add_input("Segment Pause", 1)

        self.add_widget(grid)
        
                #CP
        button_bar = BoxLayout(size_hint_y=None, height=40)
        copy_btn = Button(text="Copy")
        paste_btn = Button(text="Paste")

        # Use controller reference to call clipboard methods
        copy_btn.bind(on_press=lambda x: self.controller.clipboard_set_moving(self.get_data_dict()))
        paste_btn.bind(on_press=lambda x: self.controller.clipboard_paste_moving(self))
        
        button_bar.add_widget(copy_btn)
        button_bar.add_widget(paste_btn)
        self.add_widget(button_bar)
        
        
        
    def get_data_dict(self):
        return {key: ti.text for key, ti in self.inputs.items()}

    def set_data_dict(self, data_dict):
        for key in self.inputs:
            self.inputs[key].text = data_dict.get(key, "")

    def update_label(self, well_number):
        self.children[-1].text = f"Moving Step for Well {well_number}"  # assuming label is last


    def get_moving_data(self):
        p = self.inputs["Initial Pause"].text
        s = self.inputs["Speed"].text
        n = self.inputs["Number of Segments"].text
        t = self.inputs["Segment Pause"].text
        return f"M{p}{s}{n}{t}"


# ----------------------------- #
# MyGridLayout: Main App Layout
# ----------------------------- #

class MyGridLayout(BoxLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
        # Directory selector
        dir_layout = BoxLayout(size_hint=(1, 0.1))
        self.path_input = TextInput(hint_text="Save Directory", readonly=True)
        browse_btn = Button(text="Browse", size_hint_x=0.3)
        browse_btn.bind(on_press=self.open_file_chooser)
        dir_layout.add_widget(self.path_input)
        dir_layout.add_widget(browse_btn)
        self.add_widget(dir_layout)

        self.orientation = 'vertical'
        self.wells = []
        self.clipboard = None
        self.max_wells = 12
        #clipboards to add copy and paste
        self.clipboard_agitation = None
        self.clipboard_moving = None
        self.clipboard_well = None



        self.name_input = TextInput(hint_text="Name of Protocol", size_hint=(1, 0.1), multiline=False)
        self.add_widget(self.name_input)

        self.scroll = ScrollView(size_hint=(1, 0.75))
        
        self.container = BoxLayout(orientation='vertical', size_hint_y=None, spacing=15, padding=10)
        self.container.bind(minimum_height=self.container.setter('height'))
        
        self.scroll.add_widget(self.container)
        self.add_widget(self.scroll)

        btns = BoxLayout(size_hint=(1, 0.15))
        add_well = Button(text="Add Well")
        add_well.bind(on_press=self.add_well)
        submit = Button(text="Submit")
        submit.bind(on_press=self.submit)
        btns.add_widget(add_well)
        btns.add_widget(submit)
        self.add_widget(btns)

        self.add_well()  # Start with first well
        
        separator = BoxLayout(size_hint_y=None, height=5, padding=0)
        separator.canvas.before.clear()
        with separator.canvas.before:

            Color(0.6, 0.6, 0.6, 1)  # light gray
            self.rect = Rectangle(size=separator.size, pos=separator.pos)
            separator.bind(size=lambda w, s: setattr(self.rect, 'size', s))
            separator.bind(pos=lambda w, p: setattr(self.rect, 'pos', p))

        self.container.add_widget(separator)
        
    def open_file_chooser(self, instance):
        content = BoxLayout(orientation='vertical')
        filechooser = FileChooserListView(path=os.getcwd(), dirselect=True)
        content.add_widget(filechooser)

        buttons = BoxLayout(size_hint_y=None, height=40)
        select_btn = Button(text="Select")
        cancel_btn = Button(text="Cancel")
        buttons.add_widget(select_btn)
        buttons.add_widget(cancel_btn)
        content.add_widget(buttons)

        popup = Popup(title="Select Directory", content=content,
                    size_hint=(0.9, 0.9), auto_dismiss=False)

        def select_path(instance):
            if filechooser.selection:
                self.path_input.text = filechooser.selection[0]
            popup.dismiss()

        select_btn.bind(on_press=select_path)
        cancel_btn.bind(on_press=popup.dismiss)

        popup.open()

        
    
    def clipboard_paste_well(self, well_block):
        if not self.clipboard_well:
            return
        steps_data, moving_data = self.clipboard_well

        # Clear existing steps
        for step in well_block.steps:
            well_block.step_container.remove_widget(step)
        well_block.steps.clear()

        # Add copied steps
        for data in steps_data:
            step = AgitationStep(well_block.well_number, len(well_block.steps) + 1, well_block)
            step.set_data_tuple(data)
            well_block.steps.append(step)
            well_block.step_container.add_widget(step)

        # Update moving step
        if hasattr(well_block, 'moving_step') and moving_data:
            well_block.moving_step.set_data_dict(moving_data)

        well_block.update_labels()
        well_block.update_height()


    def clipboard_set_well(self, well_block):
        steps_data = [step.get_data_tuple() for step in well_block.steps]
        moving_data = None
        if hasattr(well_block, 'moving_step'):
            moving_data = well_block.moving_step.get_data_dict()
        self.clipboard_well = (steps_data, moving_data)

    def delete_well(self, well_block):
        if len(self.wells) <= 1:
            return  # Don't delete the only well
        self.wells.remove(well_block)
        self.container.remove_widget(well_block)
        self.update_all_labels()
    #updating labels
    def update_all_labels(self):
        for i, well in enumerate(self.wells, start=1):
            well.well_number = i
            well.update_labels()

    def clipboard_set_agitation(self, data_tuple):
       self.clipboard_agitation = data_tuple

    def clipboard_paste_agitation(self, step_widget):
        if self.clipboard_agitation:
            step_widget.set_data_tuple(self.clipboard_agitation)

    def clipboard_set_moving(self, data_dict):
        self.clipboard_moving = data_dict

    def clipboard_paste_moving(self, moving_widget):
        if self.clipboard_moving:
            moving_widget.set_data_dict(self.clipboard_moving)

    def clipboard_set(self, data_tuple):
       self.clipboard = data_tuple

    def clipboard_paste(self, step_widget):
        if self.clipboard:
            step_widget.set_data_tuple(self.clipboard)


    def add_well(self, *args):
        if len(self.wells) >= self.max_wells:
            return

        well = WellBlock(len(self.wells) + 1, self)
        self.wells.append(well)
        self.container.add_widget(well)

    def submit(self, *args):
        #extract the file name and the file path from the text input boxes
        name = self.name_input.text.strip()
        path_input = self.path_input.text.strip()
        slash = "/"
        pname = path_input + slash + name 
        if not name:
            return

        full_protocol = [name]
        for i, well in enumerate(self.wells):
            steps_data, valid, message = well.get_data()
            if not valid:
                print("Validation Error:", message)
                return
            full_protocol.extend(steps_data)
            #logic for when to add the moving step
            if (i < len(self.wells) - 1 ) or  ( hasattr(well, 'moving_step') and (len(self.wells) == 1 or i < len(self.wells) - 1)):  # Only add M-line if not last well
                full_protocol.append(well.moving_step.get_moving_data())
                
            # # Include moving step if it's not the last well,
            # # OR if it's the only well (so we don't skip it)
            # if hasattr(well, 'moving_step') and (len(self.wells) == 1 or i < len(self.wells) - 1):
            #     full_protocol.append(well.moving_step.get_moving_data())

        
        full_text = "\r\n".join(full_protocol)
        with open(pname+ ".txt", "w") as file:
            file.write(full_text)
        file.close()

        qr = qrcode.make(full_text)
        qr.save(f"{pname}_QR.png")


# ----------------------------- #
# WellBlock: Holds Steps + MovingStep
# ----------------------------- #

class WellBlock(BoxLayout):
    def __init__(self, well_number, controller, **kwargs):
        super().__init__(**kwargs)
        self.orientation = 'vertical'
        self.size_hint_y = None
        self.padding = 10
        self.spacing = 15
        self.controller = controller
        self.well_number = well_number
        self.steps = []

        self.step_container = BoxLayout(orientation='vertical', size_hint_y=None, spacing=90)
        self.step_container.bind(minimum_height=self.step_container.setter('height'))
        self.step_container.height = 0
        self.add_widget(self.step_container)
        
        #buttons that configure wells
        button_bar = BoxLayout(size_hint_y=None, height=40)
        copy_btn = Button(text=f"Copy Well {well_number}")
        paste_btn = Button(text=f"Paste into Well {well_number}")
        delete_btn = Button(text=f"Delete Well {well_number}")


        
        #button that adds new steps in current Well
        self.step_controls = BoxLayout(size_hint_y=None, height=40)
        add_step = Button(text=f"Add Step to Well {well_number}")
        add_step.bind(on_press=self.add_step)
        self.step_controls.add_widget(add_step)
        self.add_widget(self.step_controls)
        # Add spacing between button and next section
        self.add_widget(BoxLayout(size_hint_y=None, height=5))
        
        


        copy_btn.bind(on_press=lambda x: controller.clipboard_set_well(self))
        paste_btn.bind(on_press=lambda x: controller.clipboard_paste_well(self)) # problem child
        delete_btn.bind(on_press=lambda x: controller.delete_well(self))

        button_bar.add_widget(copy_btn)
        button_bar.add_widget(paste_btn)
        button_bar.add_widget(delete_btn)
        self.add_widget(button_bar)
        self.delete_bar = button_bar  # Track for height



        # Only add MovingStep if not the last well
        if well_number < 12:
            self.moving_step = MovingStep(well_number, controller)
            self.add_widget(self.moving_step)
            
        self.add_step()  # Add the first step
        self.update_height()
    
    def get_data(self):
        data = []
        for step in self.steps:
            valid, msg = step.validate_ranges()
            if not valid:
                return [], False, msg
            data.append(step.get_step_data())
        return data, True, ""


    def add_step(self, *args):
        step = AgitationStep(self.well_number, len(self.steps) + 1, self)
        self.steps.append(step)
        self.step_container.add_widget(step)
        self.update_height()

    def update_labels(self):
        for i, step in enumerate(self.steps, start=1):
            step.well_number = self.well_number
            step.step_number = i
            step.update_labels()
        if hasattr(self, "moving_step"):
            self.moving_step.update_label(self.well_number)

    def get_next_step(self, current_step):
        idx = self.steps.index(current_step)
        if idx < len(self.steps) - 1:
            return self.steps[idx + 1]
        return None

    def get_previous_step(self, current_step):
        idx = self.steps.index(current_step)
        if idx > 0:
            return self.steps[idx - 1]
        return None


    def delete_step(self, step_widget):
        if len(self.steps) == 1:
            # Clear fields if only one step remains
            for field in step_widget.inputs:
                field.text = ""
        else:
            self.steps.remove(step_widget)
            self.step_container.remove_widget(step_widget)
            self.update_labels()
        self.update_height()

    def update_height(self):
        total_height = 0

        # Add height for all steps
        total_height += sum([step.height for step in self.steps])

        # Add the height of the "Add Step" button row
        total_height += self.step_controls.height

        # Add extra spacing after button (if you added it)
        total_height += 5

        # Add moving step if present
        if hasattr(self, 'moving_step'):
            total_height += self.moving_step.height

        # Add delete well button if present
        if hasattr(self, 'delete_bar'):
            total_height += self.delete_bar.height

        # Final padding for visual breathing room
        total_height += 60

        self.height = total_height




# ----------------------------- #
# Main App
# ----------------------------- #

class MyApp(App):
    def build(self):
        return MyGridLayout()


if __name__ == '__main__':
    MyApp().run()
