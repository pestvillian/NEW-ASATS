import kivy
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.boxlayout import BoxLayout

class MyApp(App):
    def build(self):
        layout = BoxLayout(orientation='vertical')
        label = Label(text="Hello, Kivy!")
        button = Button(text="Click Me")
        layout.add_widget(label)
        layout.add_widget(button)
        return layout

if __name__ == '__main__':
    MyApp().run()