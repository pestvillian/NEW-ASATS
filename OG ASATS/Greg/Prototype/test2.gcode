;FLAVOR:Marlin
;MINX:81.521
;MINY:81.521
;MINZ:0.2
;MAXX:153.479
;MAXY:153.479
;MAXZ:8.4
;TARGET_MACHINE.NAME:Creality Ender-3 Pro
;Generated with Cura_SteamEngine 5.9.0
; Ender 3 Custom Start G-code
G28 ; Home all axes

;start proceudre 

G0 Z50 F3000 ;lift the head and set speed
G0 X50       ;start first step
G0 Z0        ;bring head down
G0 Z50       ;bring head back up
G4 S2        ;how do i shake? using a pause as a placeholder
G0 Z0        ;bring head back down
G0 Z50       ;bread head back up
G0 X100       ;start next step


