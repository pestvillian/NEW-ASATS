Hello, I'm Dorjee. I worked on the touchscreen firmware by myself and the motor controller firmware with Greg. I led the software architecture of the motor controller firmware by handling UART reception and parsing, and I set up the state machine to call the motor movement functions.

Here is a breakdown of the file structure for this directory:

	- "bigger_touchscreen" contains the stm32cubeide files for the touchscreen PCB working on its own
	- "faster_prototype_with_microstepping" contains working firmware for our first prototype that had all three motors moving, but did not have precise motor movements
	- "MOTOR_SCRIPTS" contains a plethora of motor testing files over the course of the past few months 
	- "premade_protocol_testing_clean" contains working firmware for moving the motors without the touchscreen
	- "usb_AND_touchscreen" contains the stm32cubeide files for the stm32 development board

Lastly, I'll clarify this folder represents my work for my own subsystem earlier in the project. Our final scripts are located in the "Motor-Controller-Firmware" folder because it contains the combination of both Greg and my work.
