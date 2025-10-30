# ;FLAVOR:Marlin
# ;MINX:81.521
# ;MINY:81.521
# ;MINZ:0.2
# ;MAXX:153.479
# ;MAXY:153.479
# ;MAXZ:8.4
# ;TARGET_MACHINE.NAME:Creality Ender-3 Pro
# ;Generated with Cura_SteamEngine 5.9.0
# ; Ender 3 Custom Start G-code
"""# img = qrcode.make('Some data here')
# type(img)  # qrcode.image.pil.PilImage
# img.save("some_file.png")


# qr = qrcode.QRCode(
#     version=1,
#     error_correction=qrcode.constants.ERROR_CORRECT_L,
#     box_size=10,
#     border=4,
# )
# qr.add_data('Some data')
# qr.make(fit=True)

# img = qr.make_image(fill_color="black", back_color="white")


"""
#QR code library testing

import qrcode


#attempt at doing it with the Gcode File
#global vars

gcode = ["G28"]
step = 0
steps = 0


def generate_gcode(x, z,speed):
    
    #gcode.append("G28")  # Absolute coordinates Move home
    gcode.append("G0 Z{} F{}".format(z,speed))  # Move to starting point
    #move over to next step
    gcode.append("G0 X{}".format(x)) 
    #agitation
    gcode.append("G0 Z{}".format(0))
    gcode.append("G0 Z{}".format(z))
    gcode.append("G0 Z{}".format(0))
    gcode.append("G0 Z{}".format(z))



 
    #format the list
    return "\n".join(gcode)






def text_to_qr(text_file, output_file):
    """Converts a text file to a QR code image."""

    with open(text_file, 'r') as f:
        text_data = f.read()

    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_H,
        box_size=10,
        border=4,
    )
    
    qr.add_data(text_data)
    qr.make(fit=True)

    img = qr.make_image(fill_color="black", back_color="white")
    img.save(output_file)



def main():
    #attempt at making Gcade
    
    print("hello user! welcome to the ASAT device. Prepare your testing information:\n\n\n")
    steps = input("how many steps would you like for this test? ==> ")
    

    for step in range(0,int(steps)):
        
        
        speed = input("how fast are we agitating => ")
        depth = input("how deep are we going => ")
        increments = input("how far along => ")

        new_gcode = generate_gcode(increments, depth,speed) #generate the Gcode
        
        
        
    new_gcode = new_gcode + "\nG28" #ending go home line

    print(new_gcode)
    
    with open('E:/NEW.gcode', 'w') as f:
        f.write(new_gcode)
    
    text_file = "NEW.gcode"  # Replace with your text file path
    output_file = "output_qr.png"  # Replace with desired output QR code image path
    text_to_qr(text_file, output_file)
    
    







if __name__ == '__main__':
    main()
    