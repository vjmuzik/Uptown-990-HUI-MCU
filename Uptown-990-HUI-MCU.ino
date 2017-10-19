#include <i2c_t3.h>

#define uptown 0x40            //Uptown 990 Driver Board i2c address (Might be different for others, all mine are 0x40)
#define allCall 0x00           //i2c all call address  (probably not needed but it seems to be apart of the initialization process)

#define uptReadLen 20          //Number of bytes to read from uptown driver board
#define uptWriteLen 21         //Number of bytes to write to uptown driver board
uint8_t uptCurrentMode;         //0 = manual, 1 = auto, 2 = ready, 3 = write  Not used
uint8_t uptRead[uptReadLen];      //Buffer for data read from uptown driver board
uint8_t uptWrite[uptWriteLen];    //Buffer for data written to uptown driver board
uint16_t uptFaderRead[8];         //Buffer for read fader positions
uint8_t uptButtonRead[8];         //Buffer for read button states
uint8_t uptControlPanelRead[8];   //Buffer for read control panel button states
uint8_t uptTouchRead[8];          //Buffer for read touch states
uint16_t uptFaderWrite[8];        //Buffer for written fader positions
uint8_t uptButtonWrite[8];        //Buffer for written button states
uint8_t zoneSelect = 0;          //Changes with HUI/MCU message zone select
const uint8_t muteAddress[8] = {17, 16, 15, 14, 13, 12, 11, 10};      //Address for mute change in the write message
const uint8_t faderMSBAddress[8] = {9, 8, 7, 6, 5, 4, 3, 2};         //Address for fader MSB change
const uint8_t faderLSBPosition[8] = {6, 4, 2, 0, 6, 4, 2, 0};
const uint8_t faderLSBAddress[8] = {20, 20, 20, 20, 19, 19, 19, 19};
uint8_t muteLastState[8];          //Used to check button state, toggle on change from 0x10 to 0x08, toggle off on change from 0x08 to 0x10
uint8_t controlPanelLastState[8];
uint8_t touchLastState[8];
uint16_t faderLastValue[8];
const uint8_t controlPanelZone[8] = {0x08, 0x08, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
const uint8_t controlPanelPort[8] = {0x03, 0x07, 0x03, 0x02, 0x00, 0x04, 0x05, 0x01};
const uint8_t controlPanelNote[8] = {0x51, 0x50, 0x4A, 0x4A, 0x4C, 0x4B, 0x4D, 0x4E};
const uint8_t addressConverter[8] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
elapsedMillis driverIO;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);   // LED
  digitalWrite(LED_BUILTIN, LOW); // LED off

  // Setup for Master mode, pins 18/19, external pullups, 100kHz, 200ms default timeout
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 100000);
  Wire.setDefaultTimeout(200000); // 200ms

  Serial.begin(115200);
  hwInit();
  uptRunMode();
  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleVelocityChange(OnVelocityChange);
  usbMIDI.setHandleProgramChange(OnProgramChange);
  usbMIDI.setHandleAfterTouch(OnAfterTouch);
  usbMIDI.setHandlePitchChange(OnPitchChange);
//  usbMIDI.setHandleSysEx(OnSysEx);
}

void loop() {
  if (Serial.available() > 0) {
    serialRead();
  }
  while(usbMIDI.read()){
  
  }

  uptSetupWriteButtons();
  uptSetupWriteFaders();
  
  if (driverIO >= 30) {
    driverIO = driverIO - 30;
    if(driverIO > 0) {
      Serial.print("Latency: ");
      Serial.println(driverIO);
    }
    uptReadFrom();
    uptWriteTo();
  }
  
  uptSetupReadButtons();
  uptSetupReadFaders();
  
}

void serialRead() {
  String serial = Serial.readString();
  if (serial == "read") {
    Serial.println("Reading...");
    uptSendStart();
    uptSendStop();
  }
  else if (serial == "readButtons") {
    Serial.println("Reading Buttons...");
    uptSendStartButtons();

    uptSendStop();
  }
  else if (serial == "init") {
    hwInit();
  }
  else if (serial == "bottoms") {
    uptBottoms();
  }
  else if (serial == "zeroDB") {
    uptZeroDB();
  }
  else if (serial == "tops") {
    uptTops();
  }
  else if (serial == "rocket") {
    uptRocket();
  }
  else if (serial == "position") {
    uptPosition();
  }
  else if (serial == "runMode") {
    uptRunMode();
  }
  else if (serial == "allOff") {
    uptWrite[0] = 0x00;
  }
  else if (serial == "autoOn") {
    uptAutoOn();
  }
  else if (serial == "manualOn") {
    uptManualOn();
  }
  else if (serial == "readyOn") {
    uptReadyOn();
  }
  else if (serial == "writeOn") {
    uptWriteOn();
  }
  else if (serial == "touchOn") {
    uptTouchOn();
  }
  else if (serial == "touchOff") {
    uptTouchOff();
  }
  else if (serial == "holdOn") {
    uptHoldOn();
  }
  else if (serial == "holdOff") {
    uptHoldOff();
  }
  else if (serial == "S1On") {
    uptS1On();
  }
  else if (serial == "S1Off") {
    uptS1Off();
  }
  else if (serial == "S2On") {
    uptS2On();
  }
  else if (serial == "S2Off") {
    uptS2Off();
  }
  else {
    Serial.println("Not a valid command");
  }

}

void hwInit() {
  uint8_t revNum = 0;
  uint8_t fadNum = 0;
  Serial.println("Initializing...");
  Wire.beginTransmission(allCall);      //Start i2c command for all call address
  Wire.write(0x01);                     //As far as I can tell this probably isn't needed
  Wire.endTransmission();               //being that the command is normally to ignore it

  delay(1901);                          //All delays are based off i2c readings and might not be needed at all

  Wire.requestFrom(uptown, 2);          //Request 2 bytes, 1st byte is revision number, 2nd is number of faders
  if (Wire.getError() != 0) {           //See if there's an error
    Serial.println("Failed to find system");
    delay(1000);
    Serial.println("Searching again...");
    hwInit();
  }
  else {
    revNum = Wire.readByte();           //Read revision number from buffer
    fadNum = Wire.readByte();           //Read number of faders from buffer
    Serial.printf("Revision Number: %u    Number of Faders: %u \n", revNum, fadNum);

    delay(73);

    uptMode(0x00, 0x50);                //Replaces individual recallings of 2 byte transfer

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x00);
    //    Wire.write(0x50);
    //    Wire.endTransmission();

    delay(4047);

    uptMode(0x72, 0x72);

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x72);
    //    Wire.write(0x72);
    //    Wire.endTransmission();

    delay(118);

    uptMode(0x12, 0x12);

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x12);
    //    Wire.write(0x12);
    //    Wire.endTransmission();

    delay(103);

    uptMode(0x23, 0x23);

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x23);
    //    Wire.write(0x23);
    //    Wire.endTransmission();

    delay(39);

    uptMode(0x64, 0x64);

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x64);
    //    Wire.write(0x64);
    //    Wire.endTransmission();

    delay(238);

    uptMode(0x50, 0x50);

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x50);
    //    Wire.write(0x50);
    //    Wire.endTransmission();

    delay(55);

    uptMode(0x21, 0x21);

    //    Wire.beginTransmission(uptown);     //Initialization sequence as far as I can tell
    //    Wire.write(0x21);
    //    Wire.write(0x21);
    //    Wire.endTransmission();

    delay(87);

    uptReadFrom();                              //Get revision number, number of faders, and 18 0x00

    delay(293);

    uptMode(0x60, 0x60);

    //    Wire.beginTransmission(uptown);         //Sending same byte twice seems to change mode
    //    Wire.write(0x60);                       //I believe this is a strictly reading mode
    //    Wire.write(0x60);
    //    Wire.endTransmission();

    delay(65);

    uptBottoms();
    delay(30);
    uptReadFrom();                              //After read mode it gets current data
    Serial.println("Done...");
  }

}

void uptMode(uint8_t a, uint8_t b) {              //Send a 2 byte command believed to change mode
  Wire.beginTransmission(uptown);         //0x80 Believed to be a mode to position faders at start
  Wire.write(a);
  Wire.write(b);
  Wire.endTransmission();

  //  delay(15);
}

void uptRunMode() {
  uptSendStart();
  uptMode(0x80, 0x80);
  delay(40);
  uptMode(0x90, 0x90);
  delay(15);
  uptWrite[0] = 0x00;
  uptWrite[1] = 0x08;
}

void uptReadFrom() {
//  Serial.println("Uptown Reading From");
  if!(Wire.requestFrom(uptown, uptReadLen)){   //Gets current data from driver board
    hwInit();
  }
  for (int d = 0; d < uptReadLen; d++) {
    uptRead[d] = Wire.readByte();
//    Serial.print(uptRead[d], HEX);
//    Serial.print("  ");
  }
//  Serial.println();
}

void uptWriteTo() {
//  Serial.println("Uptown Writing To");
  Wire.beginTransmission(uptown);
  for (int d = 0; d < uptWriteLen; d++) {
    Wire.write(uptWrite[d]);
//    Serial.print(uptWrite[d], HEX);
//    Serial.print("  ");
  }
  Wire.endTransmission();
//  Serial.println();
}

void uptSendStart() {
  uptMode(0x50, 0x50);
  delay(40);
  uptMode(0x72, 0x72);
  delay(40);
  uptMode(0x60, 0x60);
  delay(48);
  uptReadFrom();
}

void uptSendStartButtons() {
  uptMode(0x50, 0x50);
  delay(40);
  uptMode(0x72, 0x72);
  delay(40);
  uptMode(0x60, 0x60);
  delay(164);
  uptMode(0xB0, 0xB0);
  delay(48);
  uptReadFrom();
  uptWrite[0] = 0xB0;
  uptWrite[1] = 0x08;
  uptWrite[2] = 0x0F;
  uptWrite[3] = 0x0F;
  uptWrite[4] = 0x0F;
  uptWrite[5] = 0x0F;
  uptWrite[6] = 0x0F;
  uptWrite[7] = 0x0F;
  uptWrite[8] = 0x0F;
  uptWrite[9] = 0x0F;
  uptWrite[19] = 0x00;
  uptWrite[20] = 0x00;
  uptWriteTo();
  delay(30);
  uptReadFrom();
}

void uptSendStop() {
  delay(190);

  uptMode(0x60, 0x60);

  delay(170);

  uptMode(0xB0, 0xB0);
}

void uptBottoms() {
  Serial.println("Bottoms");
  uptSendStart();

  uptMode(0x80, 0x80);

  delay(15);

  uptReadFrom();

  uptWrite[0] = 0x80;
  uptWrite[1] = 0x08;
  uptWrite[2] = 0x05;
  uptWrite[3] = 0x05;
  uptWrite[4] = 0x05;
  uptWrite[5] = 0x05;
  uptWrite[6] = 0x05;
  uptWrite[7] = 0x05;
  uptWrite[8] = 0x05;
  uptWrite[9] = 0x05;
  uptWrite[10] = 0x00;
  uptWrite[11] = 0x00;
  uptWrite[12] = 0x00;
  uptWrite[13] = 0x00;
  uptWrite[14] = 0x00;
  uptWrite[15] = 0x00;
  uptWrite[16] = 0x00;
  uptWrite[17] = 0x00;
  uptWrite[18] = 0x00;
  uptWrite[19] = 0x00;
  uptWrite[20] = 0x00;
  uptWriteTo();

  uptSendStop();
}

void uptZeroDB() {
  Serial.println("ZeroDB");
  uptSendStart();

  uptMode(0x80, 0x80);

  delay(15);

  uptReadFrom();

  uptWrite[0] = 0x80;
  uptWrite[1] = 0x08;
  uptWrite[2] = 0xC3;
  uptWrite[3] = 0xC3;
  uptWrite[4] = 0xC3;
  uptWrite[5] = 0xC3;
  uptWrite[6] = 0xC3;
  uptWrite[7] = 0xC3;
  uptWrite[8] = 0xC3;
  uptWrite[9] = 0xC3;
  uptWrite[10] = 0x00;
  uptWrite[11] = 0x00;
  uptWrite[12] = 0x00;
  uptWrite[13] = 0x00;
  uptWrite[14] = 0x00;
  uptWrite[15] = 0x00;
  uptWrite[16] = 0x00;
  uptWrite[17] = 0x00;
  uptWrite[18] = 0x00;
  uptWrite[19] = 0x00;
  uptWrite[20] = 0x00;
  uptWriteTo();

  uptSendStop();
}

void uptTops() {
  Serial.println("Tops");
  uptSendStart();

  uptMode(0x80, 0x80);

  delay(15);

  uptReadFrom();

  uptWrite[0] = 0x80;
  uptWrite[1] = 0x08;
  uptWrite[2] = 0xF5;
  uptWrite[3] = 0xF5;
  uptWrite[4] = 0xF5;
  uptWrite[5] = 0xF5;
  uptWrite[6] = 0xF5;
  uptWrite[7] = 0xF5;
  uptWrite[8] = 0xF5;
  uptWrite[9] = 0xF5;
  uptWrite[10] = 0x00;
  uptWrite[11] = 0x00;
  uptWrite[12] = 0x00;
  uptWrite[13] = 0x00;
  uptWrite[14] = 0x00;
  uptWrite[15] = 0x00;
  uptWrite[16] = 0x00;
  uptWrite[17] = 0x00;
  uptWrite[18] = 0x00;
  uptWrite[19] = 0x00;
  uptWrite[20] = 0x00;
  uptWriteTo();

  uptReadFrom();
  uptSendStop();
}

void uptRocket() {
  Serial.println("Rocket Faders...    Send any key to stop");
  uptSendStart();
  uptMode(0x80, 0x80);
  delay(40);
  uptMode(0x90, 0x90);
  delay(15);
  while (Serial.available() == 0) {
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0xF5;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0xF5;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0xF5;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0xF5;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0xF5;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0xF5;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0x0F;
    uptWrite[3] = 0xF5;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();

    delay(100);
    uptReadFrom();

    uptWrite[0] = 0x90;
    uptWrite[1] = 0x08;
    uptWrite[2] = 0xF5;
    uptWrite[3] = 0x0F;
    uptWrite[4] = 0x0F;
    uptWrite[5] = 0x0F;
    uptWrite[6] = 0x0F;
    uptWrite[7] = 0x0F;
    uptWrite[8] = 0x0F;
    uptWrite[9] = 0x0F;
    uptWrite[10] = 0x00;
    uptWrite[11] = 0x00;
    uptWrite[12] = 0x00;
    uptWrite[13] = 0x00;
    uptWrite[14] = 0x00;
    uptWrite[15] = 0x00;
    uptWrite[16] = 0x00;
    uptWrite[17] = 0x00;
    uptWrite[18] = 0x00;
    uptWrite[19] = 0x00;
    uptWrite[20] = 0x00;
    uptWriteTo();
    delay(100);
  }

  uptBottoms();

}

void uptPosition() {
  String serial;
  while (serial != "stop") {
    if(Serial.available() > 0){
      serial = Serial.readString();
      if (serial == "stop") {
        
      }
      else {
        uint16_t value = serial.toInt();
        if (value > 1023){
          value = 1023;
        }
        uint8_t valueMSB = value >> 2;
        uint8_t valueLSB = (value & 0x03) | ((value & 0x03) << 2);

        Serial.println("Position");
        uptSendStart();

        uptMode(0x80, 0x80);

        delay(15);

        uptReadFrom();

        uptWrite[0] = 0x80;
        uptWrite[1] = 0x08;
        uptWrite[2] = valueMSB;
        uptWrite[3] = valueMSB;
        uptWrite[4] = valueMSB;
        uptWrite[5] = valueMSB;
        uptWrite[6] = valueMSB;
        uptWrite[7] = valueMSB;
        uptWrite[8] = valueMSB;
        uptWrite[9] = valueMSB;
        uptWrite[10] = 0x00;
        uptWrite[11] = 0x00;
        uptWrite[12] = 0x00;
        uptWrite[13] = 0x00;
        uptWrite[14] = 0x00;
        uptWrite[15] = 0x00;
        uptWrite[16] = 0x00;
        uptWrite[17] = 0x00;
        uptWrite[18] = 0x00;
        uptWrite[19] = valueLSB;
        uptWrite[20] = valueLSB;
        uptWriteTo();

        uptReadFrom();
        uptSendStop();
      }
    }
  }
}

void uptTouchOn() {
  uptMode(0xE1, 0xE1);
}

void uptTouchOff() {
  uptMode(0xE2, 0xE2);
}

void uptHoldOn() {
  uptMode(0x34, 0x34);
}

void uptHoldOff() {
  uptMode(0x33, 0x33);
}

void uptAutoOn() {
  uptMode(0x20, 0x20);
  uptCurrentMode = 1;
}

void uptReadyOn() {
  uptMode(0x90, 0x90);
  uptCurrentMode = 2;
}

void uptWriteOn() {
  uptMode(0xE0, 0xE0);
  uptCurrentMode = 3;
}

void uptManualOn() {
  uptMode(0xF0, 0xF0);
  uptCurrentMode = 0;
}

void uptS1On() {
  uptWrite[0] = uptWrite[0] | 0x01;
}

void uptS2On() {
  uptWrite[0] = uptWrite[0] | 0x02;
}

void uptS1Off() {
  uptWrite[0] = uptWrite[0] & 0xFE;
}

void uptS2Off() {
  uptWrite[0] = uptWrite[0] & 0xFD;
}

void uptSetupWriteButtons() {
  for(int c = 0; c < 8; c++) {
    uptWrite[muteAddress[c]] = uptButtonWrite[c];
  }
}

void uptSetupReadButtons() {
  for(int c = 0; c < 8; c++) {
    uptButtonRead[c] = uptRead[muteAddress[c] - 2];
    uptTouchRead[c] = uptButtonRead[c] >> 5;
    uptControlPanelRead[c] = (uptRead[16] >> c) & 0x01;
    uptButtonRead[c] = uptButtonRead[c] & 0x1F;
    if(uptButtonRead[c] != muteLastState[c]) {
      if((uptButtonRead[c] == 0x10 && muteLastState[c] == 0x00) | (uptButtonRead[c] == 0x08 && muteLastState[c] == 0x00)) {
        usbMIDI.sendControlChange(0x0F, c, 1);
        usbMIDI.sendControlChange(0x2F, 0x40 | 0x02, 1);
        usbMIDI.sendNoteOn(0x10 | c, 0x7f, 1);
      }
      else if((uptButtonRead[c] == 0x10 && muteLastState[c] == 0x08) | (uptButtonRead[c] == 0x08 && muteLastState[c] == 0x10)) {
        usbMIDI.sendControlChange(0x0F, c, 1);
        usbMIDI.sendControlChange(0x2F, 0x02, 1);
        usbMIDI.sendNoteOn(0x10 | c, 0x00, 1);
      }
      muteLastState[c] = uptButtonRead[c];
    }

    if(uptTouchRead[c] != touchLastState[c]) {
      if(uptTouchRead[c] == 0x01 && touchLastState[c] == 0x00) {
        usbMIDI.sendControlChange(0x0F, c, 1);
        usbMIDI.sendControlChange(0x2F, 0x40 | 0x00, 1);
        usbMIDI.sendNoteOn(0x60 | addressConverter[c], 0x7f, 1);
      }
      else if(touchLastState[c] == 0x01 && uptTouchRead[c] == 0x00) {
        usbMIDI.sendControlChange(0x0F, c, 1);
        usbMIDI.sendControlChange(0x2F, 0x00 | 0x00, 1);
        usbMIDI.sendNoteOn(0x60 | addressConverter[c], 0x00, 1);
      }
      touchLastState[c] = uptTouchRead[c];
    }

    if(uptControlPanelRead[c] != controlPanelLastState[c]) {
      if(uptControlPanelRead[c] == 0x01 && controlPanelLastState[c] == 0x00) {
        if(c > 0x01) {
          usbMIDI.sendControlChange(0x0F, 0x08, 1);
          usbMIDI.sendControlChange(0x2F, 0x40 | 0x05, 1);
          usbMIDI.sendNoteOn(0x47, 0x7f, 1);
        }
        usbMIDI.sendControlChange(0x0F, controlPanelZone[c], 1);
        usbMIDI.sendControlChange(0x2F, 0x40 | controlPanelPort[c], 1);
        usbMIDI.sendNoteOn(controlPanelNote[c], 0x7f, 1);
      }
      else if(controlPanelLastState[c] == 0x01 && uptControlPanelRead[c] == 0x00) {
        usbMIDI.sendControlChange(0x0F, controlPanelZone[c], 1);
        usbMIDI.sendControlChange(0x2F, 0x00 | controlPanelPort[c], 1);
        usbMIDI.sendNoteOn(controlPanelNote[c], 0x00, 1);
        if(c > 0x01) {
          usbMIDI.sendControlChange(0x0F, 0x08, 1);
          usbMIDI.sendControlChange(0x2F, 0x00 | 0x05, 1);
          usbMIDI.sendNoteOn(0x47, 0x00, 1);
        }
      }
      controlPanelLastState[c] = uptControlPanelRead[c];
    }
  }
}

void uptSetupWriteFaders() {
  uint8_t faderFirstFour;
  uint8_t faderLastFour;
  faderFirstFour = (uptFaderWrite[0] & 0x03) << 6;
  faderFirstFour = faderFirstFour | ((uptFaderWrite[1] & 0x03)) << 4;
  faderFirstFour = faderFirstFour | ((uptFaderWrite[2] & 0x03)) << 2;
  faderFirstFour = faderFirstFour | ((uptFaderWrite[3] & 0x03));
  faderLastFour = (uptFaderWrite[4] & 0x03) << 6;
  faderLastFour = faderLastFour | ((uptFaderWrite[5] & 0x03)) << 4;
  faderLastFour = faderLastFour | ((uptFaderWrite[6] & 0x03)) << 2;
  faderLastFour = faderLastFour | ((uptFaderWrite[7] & 0x03));
  uptWrite[20] = faderFirstFour;
  uptWrite[19] = faderLastFour;
  for(int f = 0; f < 8; f++) {
    uptWrite[faderMSBAddress[f]] = uptFaderWrite[f] >> 2;
  }
}

void uptSetupReadFaders() {
  for(int f = 0; f < 8; f++) {
    uptFaderRead[f] = uptRead[faderMSBAddress[f] - 2] << 2;
    uptFaderRead[f] = uptFaderRead[f] | ((uptRead[faderLSBAddress[f] - 2] >> faderLSBPosition[f]) & 0x03);
//    uptFaderRead[f] = (map(uptFaderRead[f], 20, 980, 0, 511));
    if(uptFaderRead[f] <= 20) {
      uptFaderRead[f] = 0;
    }
    if(uptTouchRead[f] == 0x01 && (uptFaderRead[f]) != faderLastValue[f]){
//      Serial.println(uptFaderRead[f]);
      usbMIDI.sendPitchBend(uptFaderRead[f] << 4, f + 1);
      usbMIDI.sendControlChange(0x00 | f, uptFaderRead[f] >> 3, 1);
      usbMIDI.sendControlChange(0x20 | f, (uptFaderRead[f] << 4) & 0x7F, 1);
      uptFaderWrite[f] =  uptFaderRead[f];
      faderLastValue[f] = uptFaderRead[f];
    }
  }
}

void OnNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if(channel == 1 && note == 0x00 && velocity == 0x00) {
    usbMIDI.sendNoteOn(0x00, 0x7f, 1);
  }
  if(channel == 1) {
    if(note >= 0x10 && note <= 0x17) {
       if(velocity == 0x7F){
        uptButtonWrite[note & 0x0F] = 0x01;
       }
    }
    else if(note == 0x51) {
       if(velocity == 0x7F){
        uptS1On();
       }
    }
    else if(note == 0x50) {
       if(velocity == 0x7F){
        uptS2On();
       }
    }
  }
  
}

void OnNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if(channel == 1 && note == 0x00 && velocity == 0x00) {
    usbMIDI.sendNoteOn(0x00, 0x7f, 1);
  }
  if(channel == 1) {
    if(note >= 0x10 && note <= 0x17) {
       if(velocity == 0x00){
        uptButtonWrite[note & 0x0F] = 0x00;
       }
    }
    else if(note == 0x51) {
       if(velocity == 0x00){
        uptS1Off();
       }
    }
    else if(note == 0x50) {
       if(velocity == 0x00){
        uptS2Off();
       }
    }
  }
}

void OnControlChange(uint8_t channel, uint8_t control, uint8_t value) {
  if(channel == 1 && control == 0x0C) {
    zoneSelect = value;
  }
  else if(channel == 1 && control == 0x2C && zoneSelect < 0x08) {
    if((value >> 4) == 0x04) {
      value = value & 0x0F;
      if(value == 0x02) {
        uptButtonWrite[zoneSelect] = 0x01;
      }
    }
    else if((value >> 4) == 0x00) {
      value = value & 0x0F;
      if(value == 0x02) {
        uptButtonWrite[zoneSelect] = 0x00;
      }
      
    }
  }
  else if(channel == 1 && control == 0x2C && zoneSelect == 0x08) {
    if((value >> 4) == 0x04) {
      value = value & 0x0F;
      if(value == 0x03) {
        uptS1On();
      }
      else if(value == 0x07) {
        uptS2On();
      }
    }
    else if((value >> 4) == 0x00) {
      value = value & 0x0F;
      if(value == 0x03) {
        uptS1Off();
      }
      else if(value == 0x07) {
        uptS2Off();
      }
    }
  }
  else if(channel == 1 && (control >= 0x00 && control <= 0x07)) {
    uptFaderWrite[control] = value << 3;
  }
  else if(channel == 1 && (control >= 0x20 && control <= 0x27)) {
    uptFaderWrite[control & 0x0F] = uptFaderWrite[control & 0x0F] | (value >> 5);
  }
}

void OnVelocityChange(uint8_t channel, uint8_t note, uint8_t velocity) {
  
}

void OnProgramChange(uint8_t channel, uint8_t program) {

}

void OnAfterTouch(uint8_t channel, uint8_t pressure) {

}

void OnPitchChange(uint8_t channel, int pitch) {
  pitch = pitch >> 4;
//  pitch = map(pitch, 0, 511, 20, 980);
  uptFaderWrite[channel - 1] = pitch;
}

void OnSysEx(const uint8_t *sxdata, uint16_t sxlength, bool sx_comp) {
  
}

