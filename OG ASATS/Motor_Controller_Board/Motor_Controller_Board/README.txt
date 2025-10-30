List of changes to track progress.

4/17/25 9:55pm. Added TMC2226 stepper driver to schematic and followed the datasheet. left questions in the schematic. main concerns are vmotor filtering, do we need diagnostic pins like pdn_uart, diag and index, and what vsense resistors do we use? there are other smaller questions too. -Dorjee

4/17/25 11:42pm. made some small updates. vmotor basic filtering is two 100n decouple caps and one 100u bulk cap. they recommend low ESR caps, which provide faster response time. however, they also recommend voltage spikes to be delayed, which would imply high ESR caps would be good too? need to do more research

4/30/25. schematic rough draft is done. i need to revisions now to make sure it will actually work. there is a main page, buck page, microcontroller page, and a stepper driver page. 

5/2/25. revised schematic with Mishari and Alex. pretty confident in the design, so we started picking footprints. I've finished the microcontroller and stepper driver footprints, working on the buck regulators right now. i want to use the same 50V rated capacitor for both Cin and Cout.

5/3/25. all footprints assigned and every part manufacturer, manufacturer part number, and jcsc part number for BOM. added boot configuration for microcontroller. ready to begin premature layout, but still very open to changes in schematic.