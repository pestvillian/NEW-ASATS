List of changes to track progress.

4/17/25 9:55pm. Added TMC2226 stepper driver to schematic and followed the datasheet. left questions in the schematic. main concerns are vmotor filtering, do we need diagnostic pins like pdn_uart, diag and index, and what vsense resistors do we use? there are other smaller questions too. -Dorjee

4/17/25 11:42pm. made some small updates. vmotor basic filtering is two 100n decouple caps and one 100u bulk cap. they recommend low ESR caps, which provide faster response time. however, they also recommend voltage spikes to be delayed, which would imply high ESR caps would be good too? need to do more research

4/30/25. schematic rough draft is done. i need to revisions now to make sure it will actually work. there is a main page, buck page, microcontroller page, and a stepper driver page. 

5/2/25. revised schematic with Mishari and Alex. pretty confident in the design, so we started picking footprints. I've finished the microcontroller and stepper driver footprints, working on the buck regulators right now. i want to use the same 50V rated capacitor for both Cin and Cout.

5/3/25. all footprints assigned and every part manufacturer, manufacturer part number, and jcsc part number for BOM. added boot configuration for microcontroller. ready to begin premature layout, but still very open to changes in schematic.

5/3/25. added all 3d models and starting pcb layout. it is tricky

5/4/25. drastically simplified design by using 24V from SMPS for all motors. only one buck regulator for logic components. layout has begun and a general placement for parts has been established by myself without review from team yet. many changes can be made. stepper drivers likely will switch to +3.3V logic, vmotor filtering should be changed to surface mounts for size continuity, copy pasting the setup from one driver to the other two, and more. 

5/5/25. Reviewed schematic with Alex and Mishari. We are confident in the schematic, we will clean up a few last things tonight and then continue layout tomorrow.

5/5/25. placed all components in a rough outline, going to review with teammates tomorrow.

5/6/25. 75% of routing finished, but the routing was very sloppy.

5/7/25. pcb design is almost finished. Petersen was relatively happy with our layout and gave some tips. mishari will implement the tips tonight. no erc or drc errors currently except for the asats logo on the pcb layout. needs a final review tomorrow and then it will be ordered!

5/8/25. Mishari added bottom +24V plane instead of 2mm trace and was reviewed by team. Changed enable pins to be controlled by esp32. added potentiometer. alex, mishari and Dorjee all reviewed footprints and design. we plan to order tomorrow, will review tonight and tomorrow before ordering.

5/8/25. switched board to horizontal text and cleaned it up.

5/9/25. BOARD HAS BEEN ORDERED. it's in "Motor_Board v5". the gerbers are in "Manufacturing_v2". the BOM and placement files are in "Extras_v2". we changed the sense resistors to 500mW because we know up to 2A per coil instead of 1A. this updated the vref resistor value. hopefully the PCB works, but either way, this was an extreme amount of work and we are all very proud of each other and our collective work!