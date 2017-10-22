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



# Wiring Information

Microcontroller: [Teensy LC](https://www.adafruit.com/product/2419)

Level Shifter: [4-channel I2C-safe Bi-directional Logic Level Converter](https://www.adafruit.com/product/757)

1 Microcontroller per driver board

1 Level Shifter per DE-9 cable

### Due to wire length another pullup resistor is required on level shifter B1-B4
#### You may have to test a couple of values but mine seems stable with a 4.7k resistor, but don't go too low or high or it won't work 

## Pinout
Each DE-9 cable can have 2 driver boards: A & B

#### This is the standard pinout for an uptown system, but it has come to my attention that not all of the systems followed this pinout
DE-9 Pin | Function | To
---------|----------|---
5        | Clock A  | Level Shifter B2
9        | Data A   | Level Shifter B1
4        | Ground A | Level Shifter Ground B
6        | Clock B  | Level Shifter B4
1        | Data B   | Level Shifter B3
2        | Ground B | Level Shifter Ground B

1st Teensy LC Pin | Function | To
------------------|----------|---
19                | Clock A  | Level Shifter A2
18                | Data A   | Level Shifter A1
Vin               | 5V       | Level Shifter HV
3.3V              | 3.3V     | Level Shifter LV
GND               | Ground   | Level Shifter Ground A

2nd Teensy LC Pin | Function | To
------------------|----------|---
19                | Clock B  | Level Shifter A4
18                | Data B   | Level Shifter A3
GND               | Ground   | Level Shifter Ground A

Level Shifter Pin | Function                       | From
------------------|--------------------------------|-----
LV                | Reference Voltage for 3.3V i2c | 1st Teensy 3.3V
HV                | Reference Voltage for 5V i2c   | 1st Teensy Vin
Ground on A side  | Ground for 3.3V i2c            | Both Teensy LC GND
Ground on B side  | Ground for 5V i2c              | DE-9 Pin 4 & 2
A1                | Data A                         | 1st Teensy LC Pin 18
A2                | Clock A                        | 1st Teensy LC Pin 19
A3                | Data B                         | 2nd Teensy LC Pin 18
A4                | Clock B                        | 2nd Teensy LC Pin 19
B1                | Data A                         | DE-9 Pin 9
B2                | Clock A                        | DE-9 Pin 5
B3                | Data B                         | DE-9 Pin 1
B4                | Clock B                        | DE-9 Pin 6
