// To give your project a unique name, this code must be
// placed into a .c file (its own tab).  It can not be in
// a .cpp file or your main sketch (the .ino file).

#include "usb_names.h"

// Edit these lines to create your own name.  The length must
// match the number of characters in your custom name.

#define MIDI_NAME   {'U','P','T','O','W','N',' ','9','9','0'}
#define MIDI_NAME_LEN  10
#define MANUFACTURER_NAME    {'U','P','T','O','W','N',' ','A','U','T','O','M','A','T','I','O','N',' ','S','Y','S','T','E','M',' ','I','N','C'}
#define MANUFACTURER_NAME_LEN   28
// Do not change this part.  This exact format is required by USB.

struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
        2 + MANUFACTURER_NAME_LEN * 2,
        3,
        MANUFACTURER_NAME
};

struct usb_string_descriptor_struct usb_string_serial_number = {
        12,
        3,
        {2,9,6,',','R','E','V',9,3,5}
};
