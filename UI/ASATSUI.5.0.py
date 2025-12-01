import kivy
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.gridlayout import GridLayout
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.scrollview import ScrollView
from kivy.uix.widget import Widget
from kivy.uix.floatlayout import FloatLayout
from kivy.graphics import Color, Rectangle
from kivy.uix.popup import Popup
from kivy.uix.filechooser import FileChooserListView
from kivy.uix.anchorlayout import AnchorLayout


import os
#make scrolling more robust
from kivy.config import Config
Config.set('kivy', 'exit_on_escape', '0')
Config.set('graphics', 'multisamples', '2')
Config.set('widgets', 'scroll_timeout', '250')
Config.set('widgets', 'scroll_distance', '10')

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
        self.height = 220 #making full step taller

        self.bind(minimum_height=self.setter('height'))
        self.controller = controller # initialze controller object
        self.well_number = well_number
        self.step_number = step_number
         # Border background (optional for clarity)
        with self.canvas.before:
            Color(0.2, 0.2, 0.2, 0.9)
            self.rect = Rectangle(pos=self.pos, size=self.size)
        self.bind(pos=self.update_rect, size=self.update_rect)



        # Title
        self.add_widget(Label(
            text="Agitation Step",
            size_hint_y=None,
            height=30,
            bold=True
        ))

        # Grid for fields
        grid = GridLayout(
            cols=2,
            size_hint_y=None,
            spacing=[15, 10],
            padding=[10, 5],
        )
        grid.bind(minimum_height=grid.setter('height'))


        self.label_widgets = {}
        self.inputs = []
        fields = [
            ("Agitation Speed", TextInput(multiline=False)),
            ("Agitation Duration", TextInput(multiline=False)),
            ("Total Volume", TextInput(multiline=False)),
            ("Percent Volume", TextInput(multiline=False)),
            ("Pause Duration", TextInput(multiline=False)),
            ("Number of Repeats", TextInput(multiline=False))
        ]

        self.speed = fields[0]
        self.duration = fields[1]
        self.totalVolume = fields[2]
        self.percentVolume = fields[3]
        self.pauseDuration = fields[4]
        self.numRepeats = fields[5]
        
        # Grid for fields (same structure as MovingStep)
        grid = GridLayout(cols=2, size_hint_y=None, height=250, padding=[20, 0], spacing=10)

        self.inputs = {}
        field_definitions = [
            ("Agitation Speed", 1),
            ("Agitation Duration", 2),
            ("Total Volume", 3),
            ("Percent Volume", 3),
            ("Pause Duration", 2),
            ("Number of Repeats", 2)
        ]


        for label_text, max_chars in field_definitions:
            grid.add_widget(Label(text=label_text, halign="right", valign="middle"))
            ti = LimitedInput(hint_text="#" * max_chars, multiline=False)
            ti.max_chars = max_chars
            ti.bind(on_text_validate=self.move_focus_to_next)
            ti.bind(on_shift_enter=self.move_focus_to_previous)
            grid.add_widget(ti)
            self.inputs[label_text] = ti

        self.add_widget(grid)

        # Buttons for managing the step
        button_bar = BoxLayout(size_hint_y=None, height=30,padding=5)
        del_btn = Button(text="Delete")
        copy_btn = Button(text="Copy")
        paste_btn = Button(text="Paste")

        # Hook buttons to controller logic
        del_btn.bind(on_press=lambda x: controller.delete_step(self))

        # NOTE: controller here is the WellBlock; its .controller is MyGridLayout
        # correct bindings for copying/pasting a single agitation step
        # `controller` here is the WellBlock; `controller.controller` is the MyGridLayout controller
        copy_btn.bind(on_press=lambda *_: controller.controller.clipboard_set_agitation(self.get_data_tuple()))
        paste_btn.bind(on_press=lambda *_: controller.controller.clipboard_paste_agitation(self))


        button_bar.add_widget(del_btn)
        button_bar.add_widget(copy_btn)
        button_bar.add_widget(paste_btn)
        self.add_widget(button_bar)

        self.update_labels()
    #update sizes
    def update_rect(self, *args):
        self.rect.pos = self.pos
        self.rect.size = self.size


    # Returns all field data as a tuple
    def get_data_tuple(self):
        return tuple(ti.text for ti in self.inputs.values())

    def set_data_tuple(self, data):
        for (key, ti), value in zip(self.inputs.items(), data):
            ti.text = value


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
        speed = self.inputs["Agitation Speed"].text
        duration = self.inputs["Agitation Duration"].text
        total = self.inputs["Total Volume"].text
        percent = self.inputs["Percent Volume"].text
        pause = self.inputs["Pause Duration"].text
        repeats = self.inputs["Number of Repeats"].text
        return f"B{speed}{duration}{total}{percent}{pause}{repeats}"

    # Validation logic to ensure reasonable input ranges
    def validate_ranges(self):
        try:
            total = int(self.inputs["Total Volume"].text)
            percent = int(self.inputs["Percent Volume"].text)
            if not (1 <= total <= 100):
                return False, "Total Volume must be between 001 and 100"
            if not (1 <= percent <= 100):
                return False, "Percent Volume must be between 001 and 100"
        except ValueError:
            return False, "Invalid input in Total or Percent Volume"
        return True, ""


# =====================================================================
# Moving Step Widget — defines transition parameters between wells
# =====================================================================
class MovingStep(BoxLayout):
    def __init__(self, controller, well_number, **kwargs):
        super().__init__(**kwargs)
        self.controller = controller
        self.well_number = well_number
        self.orientation = 'vertical'
        self.size_hint_y = None
        self.height = 180
        self.inputs = {}

        # Title and top spacing
        self.add_widget(Label(
            text=f"Moving Step for Well {well_number}",
            size_hint_y=None,
            height=25
        ))
        self.add_widget(BoxLayout(size_hint_y=None, height=10))  # Spacer for aesthetics

        # Grid for inputs (same style as agitation)
        grid = GridLayout(cols=2, size_hint_y=None, height=120, padding=[40, 0], spacing=10)

        def add_input(label_text, max_chars):
            grid.add_widget(Label(text=label_text))
            ti = LimitedInput(hint_text="###" if max_chars == 3 else "#", multiline=False)
            ti.max_chars = max_chars
            grid.add_widget(ti)
            self.inputs[label_text] = ti

        # Movement parameters (matching structure)
        add_input("Initial Pause", 3)
        add_input("Speed", 1)
        add_input("Number of Segments", 1)
        add_input("Segment Pause", 1)
        self.add_widget(grid)

        # Copy / Paste buttons (same alignment)
        button_bar = BoxLayout(size_hint_y=None, height=40, spacing=20, padding=[100, 0])
        copy_btn = Button(text="Copy")
        paste_btn = Button(text="Paste")
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
    #if error again check here
    def update_label(self, well_number):
        self.children[-1].text = f"Moving Step for Well {well_number}"

    # Combine movement data into a single line for protocol export
    def get_moving_data(self):
        p = self.inputs["Initial Pause"].text
        s = self.inputs["Speed"].text
        n = self.inputs["Number of Segments"].text
        t = self.inputs["Segment Pause"].text
        return f"M{p}{s}{n}{t}"


# =====================================================================
# Main Layout (MyGridLayout) — holds wells, toolbar, save location, etc.
# =====================================================================
class MyGridLayout(BoxLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.orientation = 'vertical'

        # --- Directory selector UI ---
        dir_layout = BoxLayout(size_hint=(1, 0.1))
        self.path_input = TextInput(hint_text="Save Directory", readonly=True)
        browse_btn = Button(text="Browse", size_hint_x=0.3)
        browse_btn.bind(on_press=self.open_file_chooser)
        dir_layout.add_widget(self.path_input)
        dir_layout.add_widget(browse_btn)
        self.add_widget(dir_layout)

        # Initialize state and clipboard holders
        self.wells = []
        self.clipboard = None
        self.max_wells = 12
        self.clipboard_agitation = None
        self.clipboard_moving = None
        self.clipboard_well = None

        # --- Scroll container for wells ---

        self.scroll = ScrollView(size_hint=(1, 1), do_scroll_x=True, do_scroll_y=False)

        # Horizontal container for wells
        self.container = BoxLayout(
            orientation='horizontal',
            spacing=60,
            padding=[60, 40],
            size_hint=(None, None),
            height=400
        )
        self.container.bind(minimum_width=self.container.setter('width'))

        # Anchor layout to keep the wells bottom-aligned
        anchor = AnchorLayout(anchor_y='bottom', size_hint=(None, 1))
        anchor.add_widget(self.container)

        # The key line: make the anchor's width track the container width
        self.container.bind(width=lambda inst, val: setattr(anchor, 'width', val))

        # Add anchor to scroll view
        self.scroll.add_widget(anchor)

        # Add scroll view to your root layout
        self.add_widget(self.scroll)


        # --- Protocol naming input ---
        self.name_input = TextInput(hint_text="Name of Protocol", size_hint=(1, 0.1), multiline=False)
        self.add_widget(self.name_input)

        # --- Buttons: Add well, Submit ---
        btns = BoxLayout(size_hint=(1, 0.15))
        add_well = Button(text="Add Well")
        add_well.bind(on_press=self.add_well)
        submit = Button(text="Submit")
        submit.bind(on_press=self.submit)
        btns.add_widget(add_well)
        btns.add_widget(submit)
        self.add_widget(btns)

        # Add first well at start
        self.add_well()

    # --- Directory chooser popup ---
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

        popup = Popup(title="Select Directory", content=content, size_hint=(0.9, 0.9), auto_dismiss=False)

        def select_path(instance):
            if filechooser.selection:
                self.path_input.text = filechooser.selection[0]
            popup.dismiss()

        select_btn.bind(on_press=select_path)
        cancel_btn.bind(on_press=popup.dismiss)
        popup.open()
        #gives each well a light boarder
    def _update_rect(self, *args):
        self.canvas.before.children[0].pos = self.pos
        self.canvas.before.children[0].size = self.size
    # --- Clipboard and well management ---
    def clipboard_set_well(self, well_block):
        steps_data = [step.get_data_tuple() for step in well_block.steps]
        moving_data = None
        if hasattr(well_block, 'moving_step'):
            moving_data = well_block.moving_step.get_data_dict()
        self.clipboard_well = (steps_data, moving_data)

    def clipboard_paste_well(self, well_block):
        if not self.clipboard_well:
            return
        steps_data, moving_data = self.clipboard_well

        # Remove existing agitation step widgets
        for step in list(well_block.steps):  # make a copy of the list
            if step in well_block.step_container.children:
                well_block.step_container.remove_widget(step)
        well_block.steps.clear()

        # If there's a moving_step in the UI, temporarily remove it so we can append agitation steps above it
        had_moving = hasattr(well_block, 'moving_step') and (well_block.moving_step in well_block.step_container.children)
        if had_moving:
            well_block.step_container.remove_widget(well_block.moving_step)

        # Recreate agitation steps from clipboard data, preserving order
        for data in steps_data:
            step = AgitationStep(well_block.well_number, len(well_block.steps) + 1, well_block)
            step.set_data_tuple(data)
            well_block.steps.append(step)
            well_block.step_container.add_widget(step)

        # Re-add moving_step at the bottom if it was present
        if had_moving:
            well_block.step_container.add_widget(well_block.moving_step)
            if moving_data:
                well_block.moving_step.set_data_dict(moving_data)

        # Refresh UI bookkeeping
        well_block.update_labels()
        # no need to resize the whole well - scrolling handles overflow


    def delete_well(self, well_block):
        if len(self.wells) <= 1:
            return
        self.wells.remove(well_block)
        self.container.remove_widget(well_block)
        self.update_all_labels()

    def update_all_labels(self):
        for i, well in enumerate(self.wells, start=1):
            well.well_number = i
            well.update_labels()

    # --- Copy/paste helpers for steps ---
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

    # --- Add well button ---
    def add_well(self, *args):
        if len(self.wells) >= self.max_wells:
            return
        well = WellBlock(len(self.wells) + 1,controller=self)
        self.wells.append(well)
        self.container.add_widget(well)

    # --- Submit button logic ---
    def submit(self, *args):
        name = self.name_input.text.strip()
        path_input = self.path_input.text.strip()
        if not name or not path_input:
            return

        pname = os.path.join(path_input, name)
        full_protocol = []
        for i, well in enumerate(self.wells):
            steps_data, valid, message = well.get_data()
            if not valid:
                print("Validation Error:", message)
                return
            full_protocol.extend(steps_data)
            # Add moving step for all but last well
            if (i < len(self.wells) - 1) or (len(self.wells) == 1):
                full_protocol.append(well.moving_step.get_moving_data())

        # Write to text file
        full_text = "\r\n".join(full_protocol)
        with open(pname + ".txt", "w") as file:
            file.write(full_text)



# =====================================================================
# WellBlock — container for multiple agitation steps and a moving step
# =====================================================================
class WellBlock(BoxLayout):
    def __init__(self, well_number, controller, **kwargs):
        super().__init__(**kwargs)
        self.well_number = well_number
        self.controller = controller
        self.orientation = 'vertical'
        self.size_hint = (None, None)
        self.size = (350, 650) #problem affecting scrolling in well blocks
        self.spacing = 10
        self.padding = 10
        self.steps = []

        with self.canvas.before:
            Color(0.3, 0.3, 0.3, 1)
            self.rect = Rectangle(pos=self.pos, size=self.size)
            self.bind(pos=self.update_rect, size=self.update_rect)

        # --- Header ---
        self.add_widget(Label(text=f"Well {well_number}", size_hint_y=None, height=30))

        # --- Scrollable container for steps ---
        # Give it a proportional height instead of (1,1)
        self.scroll = ScrollView(size_hint=(1, 0.85), bar_width=8)
        self.scroll.do_scroll_y = True
        self.scroll.scroll_y = 1 #start at top
        self.scroll.effect_y.bind(
            on_overscroll=lambda *args: setattr(self.scroll, 'scroll_y', self.scroll.scroll_y)
        )
        self.step_container = BoxLayout(
                orientation='vertical',
                size_hint_y=None,
                spacing=12,
                padding=[10, 10]
            )
        #auto expand container vertically
        self.step_container.bind(minimum_height=self.step_container.setter('height'))
        self.scroll.add_widget(self.step_container)
        self.add_widget(self.scroll)

        # --- Buttons at bottom ---
        btn_container = BoxLayout(size_hint_y=None, height=40, spacing=5)
        btn_container.add_widget(Button(text="Add Step", on_press=self.add_step))
        btn_container.add_widget(Button(text="Copy Well", on_press=lambda *_: self.controller.clipboard_set_well(self)))
        btn_container.add_widget(Button(text="Paste Well", on_press=lambda *_: self.controller.clipboard_paste_well(self)))
        btn_container.add_widget(Button(text="Delete Well", on_press=self.delete_well))
        self.add_widget(btn_container)

        # --- Populate first step + moving step ---
        self.add_step()
        self.moving_step = MovingStep(self.well_number, self.controller)
        self.step_container.add_widget(self.moving_step)


    def add_step(self, *args):
        step = AgitationStep(self.well_number, len(self.steps) + 1, controller=self)
        self.steps.append(step)
        # Insert agitation steps above the moving step if it already exists
        insert_index = len(self.step_container.children)
        if hasattr(self, "moving_step"):
            # children list is reversed in Kivy; safe way is to add above moving_step visually:
            self.step_container.remove_widget(self.moving_step)
            self.step_container.add_widget(step)
            self.step_container.add_widget(self.moving_step)
        else:
            self.step_container.add_widget(step)
        self.update_labels()
        #self.update_height() # this line is a problem with scrolling

    def delete_step(self, step):
        if step in self.steps:
            self.steps.remove(step)
            self.step_container.remove_widget(step)
            # Renumber steps
            self.update_labels()
            #self.update_height() #this one is causing problems with scrolling

    def get_next_step(self, current_step):
        if current_step not in self.steps:
            return None
        idx = self.steps.index(current_step)
        return self.steps[idx + 1] if idx + 1 < len(self.steps) else None

    def get_previous_step(self, current_step):
        if current_step not in self.steps:
            return None
        idx = self.steps.index(current_step)
        return self.steps[idx - 1] if idx - 1 >= 0 else None

    def update_labels(self):
    
        # Propagate well/step numbers to each AgitationStep
        for i, s in enumerate(self.steps, start=1):
            s.well_number = self.well_number
            s.step_number = i
            s.update_labels()
        if hasattr(self, "moving_step"):
            self.moving_step.update_label(self.well_number)

    def update_height(self):
        # Basic heuristic: base + each step height + moving step
        base = 120
        step_h = 180
        moving_h = 180 if hasattr(self, "moving_step") else 0
        new_h = base + step_h * len(self.steps) + moving_h
        self.size = (self.size[0], max(600, new_h))
        self.update_rect()

    def get_data(self):
        # """Return (list_of_lines, valid, message) for this well."""
        lines = []
        for s in self.steps:
            ok, msg = s.validate_ranges()
            if not ok:
                return [], False, f"Well {self.well_number}, Step {s.step_number}: {msg}"
            lines.append(s.get_step_data())
        return lines, True, ""

    def delete_well(self, *args):
        # Use controller to keep indices in sync
        self.controller.delete_well(self)

    def update_rect(self, *args):
        self.rect.pos = self.pos
        self.rect.size = self.size



# =====================================================================
# Main App Entry Point
# =====================================================================
class MyApp(App):
    def build(self):
        return MyGridLayout()

#run the app
if __name__ == '__main__':
    MyApp().run()
