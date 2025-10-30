import kivy
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.gridlayout import GridLayout
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button

file_path = "my_file.txt" # file name of new file


class MyGridLayout(GridLayout):
    #infinite Keywords
    def __init__(self,**kwargs):
        #call grid layout structure
        super(MyGridLayout,self).__init__(**kwargs)
        #set the colums
        self.cols = 2 ## num colums
        

        
        #add widgets - things in app
        self.add_widget(Label(text="Name:> ")) #label of the box
        #add input box
        self.name = TextInput(multiline = False) # what input goes into it
        self.add_widget(self.name)
        
        #new widget
        self.add_widget(Label(text="Favorite Pizza> ")) #label of the box
        #add input box
        self.pizza = TextInput(multiline = False) # what input goes into it
        self.add_widget(self.pizza)
        
        #new widget
        self.add_widget(Label(text="Favorite COlor? ")) #label of the box
        #add input box
        self.color = TextInput(multiline = False) # what input goes into it
        self.add_widget(self.color) # make sure when you do .add_widget you use the name as self.{whatever name of the var itself}
        
        #create a button for submitting things
        
        self.submit = Button(text="Submit Data",font_size=32)#create the button itself
        #bind the button to somthing
        self.submit.bind(on_press=self.press)
        self.add_widget(self.submit)
    
    #function for the pressing the button
    def press(self,instance):
        #create variables from the text in the text boxes
        name = self.name.text
        pizza = self.pizza.text
        color = self.color.text
        
        print(f"Hello {name}, you like {pizza}, and you favorite color is {color} !!")
        #now we can print this to the app
        self.add_widget(Label(text=f"Hello {name}, you like {pizza}, and you favorite color is {color} !!"))
        #now we nned to clear for the next press so data updates
        #clears the text boxes where you typed but now what outputs on the screen
        self.name.text = ""
        self.pizza.text = ""
        self.color.text = ""
        with open(file_path, "w") as file:
            file.write(f"Hello {name}, you like {pizza}, and you favorite color is {color} !!")
        
        
        
        
        
        
#class for main app output. here you call all the classes of widgets and other stuff you have
class MyApp(App):
    def build(self):
        return MyGridLayout()
    


if __name__ == '__main__': # run the app
    MyApp().run()