import kivy
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.gridlayout import GridLayout
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button

import qrcode
import os

class MyGridLayout(GridLayout):
    #infinite Keywords
    def __init__(self,**kwargs):
        #call grid layout structure
        super(MyGridLayout,self).__init__(**kwargs)
        #set the colums
        self.cols = 1 ## num colums
        #create a second Grid layout
        self.top_grid = GridLayout()#top_grid is name of new grid
        self.top_grid.cols = 2
        
        #putting top_grid in the add_wigget puts the text boxes in the top grid
        
        #Name of protocol as customized by user
        self.top_grid.add_widget(Label(text="Name of protocol ")) #label of the box
        #add input box
        self.name = TextInput(multiline = False) # what input goes into it
        self.top_grid.add_widget(self.name)
        
        #new widget
        self.top_grid.add_widget(Label(text="Agitation Speed ")) #label of the box
        #add input box
        self.speed = TextInput(hint_text='1-9', input_filter = 'float', multiline=False, write_tab=False) # what input goes into it
        self.top_grid.add_widget(self.speed)
        
        #new widget
        self.top_grid.add_widget(Label(text="Agitation Duration ")) #label of the box
        #add input box
        self.duration = TextInput(hint_text='01 - 99', input_filter = 'int', multiline=False, write_tab=False) # what input goes into it
        self.top_grid.add_widget(self.duration) # make sure when you do .add_widget you use the name as self.{whatever name of the var itself}
        
         #new widget
        self.top_grid.add_widget(Label(text="totalVolume")) #label of the box
        #add input box
        self.totalVolume = TextInput(hint_text='001 - 999', input_filter = 'int', multiline=False, write_tab=False) # what input goes into it
        self.top_grid.add_widget(self.totalVolume) # make sure when you do .add_widget you use the name as self.{whatever name of the var itself}
        
        #new widget
        self.top_grid.add_widget(Label(text="percentVolume")) #label of the box
        #add input box
        self.percentVolume = TextInput(hint_text='001 - 999', input_filter = 'int', multiline=False, write_tab=False) # what input goes into it
        self.top_grid.add_widget(self.percentVolume) # make sure when you do .add_widget you use the name as self.{whatever name of the var itself}
        
        #new widget
        self.top_grid.add_widget(Label(text="pauseDuration")) #label of the box
        #add input box
        self.pauseDuration = TextInput(hint_text='01-99', input_filter = 'int', multiline=False, write_tab=False) # what input goes into it
        self.top_grid.add_widget(self.pauseDuration) # make sure when you do .add_widget you use the name as self.{whatever name of the var itself}
        
        #new widget
        self.top_grid.add_widget(Label(text="numRepeats")) #label of the box
        #add input box
        self.numRepeats = TextInput(hint_text='01 - 99', input_filter = 'int', multiline=False, write_tab=False) # what input goes into it
        self.top_grid.add_widget(self.numRepeats) # make sure when you do .add_widget you use the name as self.{whatever name of the var itself}
        
        
        #add the top_grid to the APP 
        self.add_widget(self.top_grid)
        
        #create a button for submitting things
        #create the button itself
        self.submit = Button(   text="Submit Data",
                                font_size=50
                             )
        #bind the button to somthing
        self.submit.bind(on_press=self.press)
        self.add_widget(self.submit)
    
    #function for the pressing the button
    def press(self,instance):
        #create variables from the text in the text boxes
        name = self.name.text
        speed = self.speed.text
        duration = self.duration.text
        totalVolume = self.totalVolume.text
        percentVolume = self.percentVolume.text
        pauseDuration = self.pauseDuration.text
        numRepeats = self.numRepeats.text
        
        
        
        
        protocols = []  # Store protocols for this well
       
        #now we can print this to the app
        self.add_widget(Label(text=f"{name}\nB{speed}{duration}{totalVolume}{percentVolume}{pauseDuration}{numRepeats}\n")) # B is indicative of the type wich the embedded SM needs to read
        protocols.append(f"{name}\nB{speed}{duration}{totalVolume}{percentVolume}{pauseDuration}{numRepeats}\n") # enter data into list
        print(protocols)#debug
        
        #write the text file
        with open(name, "w") as file:
            file.write(f"{name}\nB{speed}{duration}{totalVolume}{percentVolume}{pauseDuration}{numRepeats}\n")
            

              
        qr = qrcode.make("\r\n".join(protocols)) # creats the QR code
        qr_file = os.path.join(f"{name}_Well1.png") #stors / names QR code
        qr.save(qr_file) #save the qr code
                
        #now we nned to clear for the next press so data updates
        #clears the text boxes where you typed but now what outputs on the screen
        self.name.text = ""
        self.speed.text = ""
        self.duration.text = ""
        self.totalVolume.text = ""
        self.percentVolume.text = ""
        self.pauseDuration.text = ""
        self.numRepeats.text = ""
        
        
        
        
        
        
        
#class for main app output. here you call all the classes of widgets and other stuff you have
class MyApp(App):
    def build(self):
        return MyGridLayout()
    


if __name__ == '__main__': # run the app
    MyApp().run()