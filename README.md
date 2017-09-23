# Uptown-990-HUI-MCU
Bypass old Uptown 990 computer and use as HUI/MCU mixer

This is the arduino project sketch file
The comments are not fully complete and may not all make sense

You should read the "Uptown 990 i2c Information.txt" file for more information about some things

 



## Control Panel Button/LED Information

Auto, Ready LEDs are always on after initializing, unless disconnected
Hold, Manual, Touch, Write LEDs are always off after initializing

Can't be changed since the driver board functionality changes with it and it always has to be in Ready Mode with Touch and Hold off for it to work with the DAW

Buttons work independently from the LEDs

S1, S2 LEDs change with the DAW

## Control Panel Button Mappings for HUI/ MCU
* S1 = Undo
* S2 = Save
* Hold = Global Automation Latch
* Manual = Global Automation Off(HUI), Global Automation Read/Off(MCU)
* Auto = Global Automation Read(HUI), Global Automation Read/Off(MCU)
* Ready = Global Automation Trim
* Touch = Global Automation Touch
* Write = Global Automation Write
