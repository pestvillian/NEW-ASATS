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
    max_chars = 3  # Default, can be overridden

    def insert_text(self, substring, from_undo=False):
        # Filter digits only
        s = ''.join([c for c in substring if c.isdigit() or c == '.'])
        if len(self.text + s) > self.max_chars:
            s = s[:self.max_chars - len(self.text)]
        return super().insert_text(s, from_undo=from_undo)

class AgitationStep(GridLayout):
    def __init__(self, step_number, **kwargs):
        super(AgitationStep, self).__init__(**kwargs)
        self.cols = 2
        self.step_number = step_number

        def add_labeled_input(label_text, max_chars):
            self.add_widget(Label(text=f"{label_text} {step_number}"))
            ti = LimitedInput(hint_text='###', multiline=False)
            ti.max_chars = max_chars
            self.add_widget(ti)
            return ti

        self.speed = add_labeled_input("Agitation Speed", 1)
        self.duration = add_labeled_input("Agitation Duration", 2)
        self.totalVolume = add_labeled_input("Total Volume", 3)
        self.percentVolume = add_labeled_input("Percent Volume", 3)
        self.pauseDuration = add_labeled_input("Pause Duration", 2)
        self.numRepeats = add_labeled_input("Number of Repeats", 2)

    def get_step_data(self):
        return f"B{self.speed.text}{self.duration.text}{self.totalVolume.text}{self.percentVolume.text}{self.pauseDuration.text}{self.numRepeats.text}"


class MyGridLayout(BoxLayout):
    def __init__(self, **kwargs):
        super(MyGridLayout, self).__init__(**kwargs)
        self.orientation = 'vertical'

        name_box = BoxLayout(size_hint=(1, 0.1))
        name_box.add_widget(Label(text="Name of Protocol", size_hint=(0.3, 1)))
        self.name_input = TextInput(hint_text='Protocol Name', multiline=False, size_hint=(0.7, 1))
        name_box.add_widget(self.name_input)
        self.add_widget(name_box)

        # Scrollable area for steps
        self.scroll = ScrollView(size_hint=(1, 0.65))
        self.steps_layout = BoxLayout(orientation='vertical', size_hint_y=None, spacing=10, padding=10)
        self.steps_layout.bind(minimum_height=self.steps_layout.setter('height'))
        self.scroll.add_widget(self.steps_layout)
        self.add_widget(self.scroll)

        self.step_count = 0
        self.add_step()

        # Buttons
        button_layout = BoxLayout(size_hint=(1, 0.2))
        self.add_step_button = Button(text="Add Step")
        self.add_step_button.bind(on_press=lambda x: self.add_step())
        self.submit_button = Button(text="Submit Data")
        self.submit_button.bind(on_press=self.submit_data)
        button_layout.add_widget(self.add_step_button)
        button_layout.add_widget(self.submit_button)
        self.add_widget(button_layout)

    def add_step(self):
        self.step_count += 1
        step = AgitationStep(self.step_count, size_hint_y=None, height=300)
        self.steps_layout.add_widget(step)

    def submit_data(self, instance):
        name = self.name_input.text.strip()
        if not name:
            return  # You can add feedback popup here

        steps_data = []
        for step_widget in self.steps_layout.children[::-1]:
            steps_data.append(step_widget.get_step_data())

        text_data = f"{name}\n" + "\n".join(steps_data)

        with open(name + ".txt", "w") as file:
            file.write(text_data)

        qr = qrcode.make(text_data)
        qr.save(f"{name}_QR.png")

        # Clear inputs
        self.name_input.text = ""
        self.steps_layout.clear_widgets()
        self.step_count = 0
        self.add_step()


class MyApp(App):
    def build(self):
        return MyGridLayout()


if __name__ == '__main__':
    MyApp().run()
