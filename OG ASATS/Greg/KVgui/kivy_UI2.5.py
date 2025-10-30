import kivy
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.gridlayout import GridLayout
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.scrollview import ScrollView
from kivy.properties import StringProperty
import qrcode
import os

class LimitedInput(TextInput):
    __events__ = ('on_shift_enter',) #possible events that may happen during UI runtime
    max_chars = 3

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.multiline = False

    def insert_text(self, substring, from_undo=False):
        s = ''.join([c for c in substring if c.isdigit()])
        if len(self.text + s) > self.max_chars:
            s = s[:self.max_chars - len(self.text)]
        return super().insert_text(s, from_undo=from_undo)

    def keyboard_on_key_down(self, window, keycode, text, modifiers):
        if keycode[1] == 'enter':
            # Let parent widget decide direction
            if 'shift' in modifiers:
                self.dispatch('on_shift_enter')
            else:
                self.dispatch('on_text_validate')
            return True  # Intercept key
        return super().keyboard_on_key_down(window, keycode, text, modifiers)

    def on_shift_enter(self):
        pass  # Placeholder for custom event


class AgitationStep(BoxLayout):
    def __init__(self, controller, **kwargs):
        super(AgitationStep, self).__init__(**kwargs)
        self.orientation = 'vertical'
        self.size_hint_y = None
        self.height = 320
        self.controller = controller

        self.grid = GridLayout(cols=2, size_hint_y=None, height=260)
        self.label_widgets = {}
        self.inputs = []  # Track inputs for Enter navigation

        def add_labeled_input(field_name, max_chars):
            label = Label(text=f"{field_name}")
            self.grid.add_widget(label)
            ti = LimitedInput(hint_text='###', multiline=False)
            ti.max_chars = max_chars
            ti.bind(on_text_validate=self.move_focus_to_next)
            ti.bind(on_shift_enter=self.move_focus_to_previous)
            self.inputs.append(ti)
            self.grid.add_widget(ti)
            self.label_widgets[field_name] = label
            return ti


        self.speed = add_labeled_input("Agitation Speed", 1)
        self.duration = add_labeled_input("Agitation Duration", 2)
        self.totalVolume = add_labeled_input("Total Volume", 3)
        self.percentVolume = add_labeled_input("Percent Volume", 3)
        self.pauseDuration = add_labeled_input("Pause Duration", 2)
        self.numRepeats = add_labeled_input("Number of Repeats", 2)

        self.add_widget(self.grid)

        # Control buttons
        button_bar = BoxLayout(size_hint_y=None, height=40, spacing=10, padding=(10, 0))
        delete_button = Button(text="Delete")
        copy_button = Button(text="Copy")
        paste_button = Button(text="Paste")

        delete_button.bind(on_press=lambda x: controller.delete_step(self))
        copy_button.bind(on_press=lambda x: controller.copy_step(self))
        paste_button.bind(on_press=lambda x: controller.paste_step(self))

        button_bar.add_widget(delete_button)
        button_bar.add_widget(copy_button)
        button_bar.add_widget(paste_button)
        self.add_widget(button_bar)
        
    def validate_ranges(self):#make sure that the percents are within a normal range
        try:
            total = int(self.totalVolume.text)
            percent = int(self.percentVolume.text)
            if not (1 <= total <= 100):
                return False, "Total Volume must be between 001 and 100"
            if not (1 <= percent <= 100):
                return False, "Percent Volume must be between 001 and 100"
        except ValueError:
            return False, "Invalid number in Total Volume or Percent Volume"
        return True, ""


    def move_focus_to_next(self, instance): #moving to next parameter field with Enter
        try:
            idx = self.inputs.index(instance)
            if idx < len(self.inputs) - 1:
                self.inputs[idx + 1].focus = True
            else:
                # Optionally, move to the first field of the next step (not required)
                next_step = self.controller.get_next_step(self)
                if next_step:
                    next_step.inputs[0].focus = True
        except ValueError:
            pass
        
    def move_focus_to_previous(self, instance): # able to move to previous parameter field with shift+Enter
        try:
            idx = self.inputs.index(instance)
            if idx > 0:
                self.inputs[idx - 1].focus = True
            else:
                # Optionally jump to the last field of the previous step
                prev_step = self.controller.get_previous_step(self)
                if prev_step:
                    prev_step.inputs[-1].focus = True
        except ValueError:
            pass


    def get_step_data(self):
        return f"B{self.speed.text}{self.duration.text}{self.totalVolume.text}{self.percentVolume.text}{self.pauseDuration.text}{self.numRepeats.text}"

    def set_step_data(self, data_tuple):
        self.speed.text = data_tuple[0]
        self.duration.text = data_tuple[1]
        self.totalVolume.text = data_tuple[2]
        self.percentVolume.text = data_tuple[3]
        self.pauseDuration.text = data_tuple[4]
        self.numRepeats.text = data_tuple[5]

    def get_data_tuple(self):
        return (
            self.speed.text,
            self.duration.text,
            self.totalVolume.text,
            self.percentVolume.text,
            self.pauseDuration.text,
            self.numRepeats.text,
        )

    def update_labels(self, step_number):
        # Update all parameter labels to reflect current step number
        for field_name, label in self.label_widgets.items():
            label.text = f"{field_name} {step_number}"
            
class MovingStep(BoxLayout):
    def __init__(self, **kwargs):
        super(MovingStep, self).__init__(**kwargs)
        self.orientation = 'vertical'
        self.size_hint_y = None
        self.height = 240

        self.grid = GridLayout(cols=2, size_hint_y=None, height=200)
        self.inputs = {}

        def add_input(label_text, max_chars):
            self.grid.add_widget(Label(text=label_text))
            ti = LimitedInput(hint_text="###" if max_chars == 3 else "#", multiline=False)
            ti.max_chars = max_chars
            self.grid.add_widget(ti)
            self.inputs[label_text] = ti

        add_input("Initial Pause (3-digit)", 3)
        add_input("Speed (1-digit)", 1)
        add_input("Number of Segments", 1)
        add_input("Segment Pause", 1)

        self.add_widget(Label(text="Moving Step", size_hint_y=None, height=30))
        self.add_widget(self.grid)

    def get_moving_data(self):
        p = self.inputs["Initial Pause (3-digit)"].text
        s = self.inputs["Speed (1-digit)"].text
        n = self.inputs["Number of Segments"].text
        t = self.inputs["Segment Pause"].text
        return f"M{p}{s}{n}{t}"


class MyGridLayout(BoxLayout):
    def __init__(self, **kwargs):
        super(MyGridLayout, self).__init__(**kwargs)
        self.orientation = 'vertical'

        name_box = BoxLayout(size_hint=(1, 0.1))
        name_box.add_widget(Label(text="Name of Protocol", size_hint=(0.3, 1)))
        self.name_input = TextInput(hint_text='Protocol Name', multiline=False, size_hint=(0.7, 1))
        name_box.add_widget(self.name_input)
        self.add_widget(name_box)

        self.scroll = ScrollView(size_hint=(1, 0.65))
        self.steps_layout = BoxLayout(orientation='vertical', size_hint_y=None, spacing=10, padding=10)
        self.steps_layout.bind(minimum_height=self.steps_layout.setter('height'))
        self.scroll.add_widget(self.steps_layout)
        self.add_widget(self.scroll)

        self.clipboard = None
        self.add_step()

        button_layout = BoxLayout(size_hint=(1, 0.2))
        self.add_step_button = Button(text="Add Step")
        self.add_step_button.bind(on_press=lambda x: self.add_step())
        self.submit_button = Button(text="Submit Data")
        self.submit_button.bind(on_press=self.submit_data)
        button_layout.add_widget(self.add_step_button)
        button_layout.add_widget(self.submit_button)
        self.add_widget(button_layout)
        
        # Add non-deletable Moving Step at the bottom
        self.moving_step = MovingStep()
        self.add_widget(self.moving_step)
        
    #somthing to help track where the curser is in the widgets to add enter button functionality
    def get_next_step(self, current_step):
        children = list(reversed(self.steps_layout.children))
        try:
            idx = children.index(current_step)
            if idx + 1 < len(children):
                return children[idx + 1]
        except ValueError:
            pass
        return None
    #somthing to help track where the curser is in the widgets to add shift+Enter button functionality
    def get_previous_step(self, current_step):
        children = list(reversed(self.steps_layout.children))
        try:
            idx = children.index(current_step)
            if idx - 1 >= 0:
                return children[idx - 1]
        except ValueError:
            pass
        return None


    def add_step(self):
        step = AgitationStep(self)
        self.steps_layout.add_widget(step)
        self.update_all_labels()

    def delete_step(self, step_widget):
        if len(self.steps_layout.children) <= 1:
            # Only one step left — clear its fields instead of deleting
            step_widget.set_step_data(("", "", "", "", "", ""))
        else:
            self.steps_layout.remove_widget(step_widget)
            self.update_all_labels()

    def update_all_labels(self):
        # Re-number steps from top to bottom
        for i, step in enumerate(reversed(self.steps_layout.children), start=1):
            step.update_labels(i)

    def copy_step(self, step_widget):
        self.clipboard = step_widget.get_data_tuple()

    def paste_step(self, step_widget):
        if self.clipboard:
            step_widget.set_step_data(self.clipboard)

    def submit_data(self, instance):
        name = self.name_input.text.strip()
        if not name:
            return  # Add user feedback popup if needed

        steps_data = []
        for step_widget in self.steps_layout.children[::-1]:
            is_valid, message = step_widget.validate_ranges()
            if not is_valid:
                print("Validation error:", message)  # Replace with a popup or label for better UX
                return  # Stop submission

            steps_data.append(step_widget.get_step_data())

        moving_data = self.moving_step.get_moving_data()
        steps_data.append(moving_data)

        text_data = f"{name}\n" + "\n".join(steps_data)

        with open(name + ".txt", "w") as file:
            file.write(text_data)

        qr = qrcode.make(text_data)
        qr.save(f"{name}_QR.png")

        self.name_input.text = ""
        self.steps_layout.clear_widgets()
        self.add_step()



class MyApp(App):
    def build(self):
        return MyGridLayout()

if __name__ == '__main__':
    MyApp().run()
