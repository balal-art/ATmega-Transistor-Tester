//const String curVer = "2020-01-01";  // Final code clean up and documentation -back to 99% used memory so done
//                                     // Added #ifdef useFonts option to save memory and insure better compiling options
//                                     // Still no guarantee but works now on multiple Ubuntu boxes / library versions for me
//const String curVer = "2019-12-30";  // Added sound mute with SEL / UP combo and added code for DOWN / SEL if desired
//                                      Added I2C scanner option to display devices
//const String curVer = "2019-12-29";   // Documented as much as I could, changed PWM pulse control to single increments +/-
//                                      Changed analog stats to be in Min / Avg / Max on the screen, moved PWM from pin 9 to 6
//                                      Allows 9,10,11,12 for Stepper outputs
//const String curVer = "2019-12-28"; // Added Servo, Ping, Logging, and Serial controls
//const String curver = "2019-12-27"; // Initial version copied from Instructable site, cleaned up tabs and documented code

// NanOMeter - Multi Function Nano Based Test base
// Stephen W Nolen / ProtoWrxs
// Core code based on VolosR work on Instructables (https://www.instructables.com/member/VolosR/)
// I used the basic lay of VolosR's board as well as it just works
// He has an updated version but I personally like the 128x32 size screen better allowing for more component room
//
// Features Includes:
//   0 Volt meter
//   1 Analog Sensor
//   2 Resistor / Ohm Tester
//   3 Diode Voltage Drop / LED Tester
//   4 Continuity Tester with Beep
//   5 PWM generator
//   6 Servo Tester
//   7 Ping  / Sonar Tester (SR04 at least)
//   8 Stepper Motor Tester
//   9 External Battery Voltage Display
//  10 Data Logging time adjustment
//  11 I2C bus scanner
// Modes 0 - 9 dump data to the serial port (USB or TTL level pins lower right)
// Mode 10 Allows adjustment from 500 (1/2 second) to a minute interval adjustments
// Data dump format is:
// Mode, millis(), data1, data2, data3, data4
// All except analog dump a single data item
// Analog dumps the read, min, average, and max data items
// Ohms dumps "inf" if nothing attached, resistance otherwise
// Continutity dumps voltage drop with 0.00 being continuity
// NOTHING is dumped during mode 10 time adjustment
// I2C dumps addresses CSV but does NOT use the Data Logging timer

// Device has some limited remote serial controls
// M will dump the mode just like the middle select button
// 0-9 will select the mode listed above
// A will select the dump timer mode
// For Mode 5, 6, and 10, the "-" emulates pressed the down button and a "+" emulates the UP button
// The "=" command resets pulses or settings to their default for the mode you are in.


//***************************************************************************************//
// Pin Uses
//  0   - Rx - On header pin lower right - Used to receive commands from serial console or other devices
//  1   - Tx - On header pin lower right - Dumps CSV data to here - runs at 115200, change in code if  needed
//  2   - Right Button
//  3(P)- Middle Button (on interrupt)
//  4   - Left Button
//  5(P)- Software Servo Pin
//  6(P)- PWM output on three pin header
//  7   - Ping Trigger
//  8   - Ping Echo
//  9(P)- Stepper IN1
// 10(P)- Stepper IN3
// 11(P)- Stepper IN2
// 12   - Stepper IN4
// 13   - Used for speaker output and LED display
// A0   - Main Analog Input
// A1   - External Battery Power Suppy Voltage 100k/10k divider
// A2   - Input for Diode and Resistor testing
// A3   -
// A4   - I2C bus
// A5   - I2C bus
// A6   -
// A7   - Voltage Meter input - on 100k/10k divider
// 


// Load the libraries
//#include <SPI.h>          // Not used for now
#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <Stepper.h>
#include <Adafruit_SSD1306.h>

// 2020-01-01 - Comment out to use basic scaled fonts
// Not as pretty but saves memory and may be needed for some combos
#define useFonts
#ifdef useFonts
  #include <Fonts/FreeSans9pt7b.h>
  #include <Fonts/FreeSans12pt7b.h>
#endif

#include <Adafruit_SoftServo.h>
// Note: To get full 180 degree movement on the servo I have modified the Adafruit_Softservo.ccp file as follows:
// micros = map(a, 0, 180, 500, 2500)
// The old 1000, 2000 mapping provides 90 degree movement only

// setup OLED - 128 x 32 is what we have here
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// Setup SoftServo for servo work
Adafruit_SoftServo myServo;  // 2019-12-28 - Using this as Servo kills PWM on 9 and 10 on Arduino

//#if (SSD1306_LCDHEIGHT != 32)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

// Setup Images - Most were converted via https://diyusthad.com/image2cpp
const unsigned char PROGMEM nanoImg [] { 
0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00,
0x30, 0x0e, 0x00, 0x00, 0x30, 0x0e, 0x00, 0x00, 0x30, 0x0e, 0x00, 0x00, 0x30, 0x0e, 0x00, 0x00,
0x3f, 0xfe, 0x38, 0x0e, 0x3f, 0xfe, 0x38, 0x0e, 0x3f, 0xfe, 0x38, 0x0e, 0x3f, 0xfe, 0x38, 0x0e,
0x3f, 0xfe, 0x38, 0x0e, 0x3f, 0xfe, 0x10, 0x04, 0x38, 0x0e, 0x10, 0x04, 0x3b, 0xee, 0x10, 0x04,
0x3b, 0xee, 0x10, 0x04, 0x33, 0xe6, 0x18, 0x0c, 0x03, 0xe0, 0x1f, 0xfc, 0x03, 0xe0, 0x01, 0xc0,
0x00, 0x80, 0x01, 0xc0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xff, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'datalog', 32x32px
const unsigned char PROGMEM dataImg [] {
0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0x0f, 0x03, 0xc0, 0x00,
0x1c, 0x00, 0xe0, 0x00, 0x30, 0xfc, 0x30, 0x00, 0x63, 0xff, 0x18, 0x00, 0x07, 0x03, 0x80, 0x00,
0x0c, 0x00, 0xc3, 0x00, 0x00, 0xfc, 0x47, 0x80, 0x01, 0xce, 0x0f, 0xc0, 0x01, 0x82, 0x1e, 0xe0,
0x00, 0x30, 0x7c, 0x70, 0x00, 0x78, 0xfc, 0xf8, 0x00, 0x79, 0xff, 0xfc, 0x00, 0x33, 0xff, 0xce,
0x00, 0x07, 0xff, 0x8e, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x1f, 0xff, 0xf8, 0x00, 0x3f, 0xff, 0xf0,
0x00, 0x3f, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x1f, 0xff, 0xc0,
0x00, 0x3f, 0xff, 0x80, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x1f, 0xfe, 0x00, 0x00, 0x3f, 0xfc, 0x00,
0x00, 0x77, 0xf8, 0x00, 0x00, 0xe2, 0x70, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


const unsigned char PROGMEM pwmImg [] {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00,
0x0e, 0x0e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00,
0x0e, 0x0e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00,
0x00, 0x0e, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x70,
0x00, 0x0e, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x70,
0x00, 0x0f, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Clock image - not used for now
const unsigned char PROGMEM clockImg [] = {
0x0F, 0x80, 0x01, 0xF0, 0x39, 0xC0, 0x03, 0x9C, 0x61, 0x80, 0x01, 0x86, 0x43, 0x1F, 0xF8, 0xC2,
0xC6, 0x70, 0x0E, 0x63, 0x8D, 0xC0, 0x03, 0xB1, 0x9B, 0x00, 0x00, 0xD9, 0xF6, 0x01, 0x80, 0x6F,
0xEC, 0x01, 0x80, 0x37, 0x58, 0x01, 0x80, 0x1A, 0x10, 0x01, 0x80, 0x08, 0x30, 0x01, 0x80, 0x0C,
0x20, 0x01, 0x80, 0x04, 0x20, 0x01, 0x80, 0x04, 0x20, 0x01, 0x80, 0x04, 0x60, 0x01, 0x80, 0x06,
0x60, 0x01, 0x80, 0x06, 0x60, 0x7F, 0x80, 0x06, 0x60, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x06,
0x20, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x30, 0x00, 0x00, 0x0C, 0x10, 0x00, 0x00, 0x08,
0x18, 0x00, 0x00, 0x18, 0x08, 0x00, 0x00, 0x10, 0x0C, 0x00, 0x00, 0x30, 0x06, 0x00, 0x00, 0x60,
0x0F, 0x80, 0x01, 0xF0, 0x18, 0xE0, 0x07, 0x18, 0x30, 0x7C, 0x3E, 0x0C, 0x20, 0x0F, 0xF0, 0x04
};

const unsigned char PROGMEM batteryImg [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xF0, 0x40, 0x00, 0x00, 0x10, 0x47, 0x3C, 0xE0, 0x18,
0x47, 0xBD, 0xE0, 0x18, 0x45, 0xA5, 0xA0, 0x1E, 0x45, 0xA5, 0xA0, 0x1A, 0x45, 0xA5, 0xA0, 0x1A,
0x45, 0xA5, 0xA0, 0x1A, 0x45, 0xA5, 0xA0, 0x1A, 0x45, 0xA5, 0xA0, 0x1E, 0x47, 0xBD, 0xE0, 0x18,
0x47, 0x3C, 0xE0, 0x18, 0x40, 0x00, 0x00, 0x10, 0x3F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//const unsigned char PROGMEM battery2Img [] {
//0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xf3, 0x81, 0xff, 0x80, 0x0c, 0x80, 0x3f,
//0x80, 0x0c, 0xe2, 0x1f, 0x80, 0x0c, 0x7d, 0x0f, 0x80, 0x1e, 0x39, 0x0f, 0x80, 0x1f, 0x81, 0x8f,
//0x80, 0x1f, 0xff, 0x0f, 0x80, 0x3f, 0xff, 0x0f, 0x80, 0x1f, 0xff, 0x0f, 0x80, 0x4f, 0xff, 0x0f,
//0x80, 0x07, 0xfe, 0x0f, 0x80, 0x81, 0xfe, 0x0f, 0x80, 0x80, 0x00, 0x0f, 0x80, 0x00, 0x04, 0x0f,
//0x81, 0x08, 0x04, 0x0f, 0x81, 0x03, 0x08, 0x0f, 0x80, 0x00, 0x08, 0x0f, 0x83, 0x00, 0x00, 0x0f,
//0x82, 0x00, 0x10, 0x0f, 0x80, 0x40, 0x10, 0x0f, 0x84, 0x10, 0x00, 0x0f, 0x84, 0x07, 0xe0, 0x0f,
//0x84, 0x00, 0x20, 0x0f, 0x84, 0x00, 0x00, 0x0f, 0xc6, 0x00, 0x40, 0x0f, 0xc3, 0x00, 0x40, 0x0f,
//0xf1, 0xe0, 0x80, 0x0f, 0xfe, 0x3f, 0x7f, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
//};

// Not used for now
//const unsigned char PROGMEM temperatureImg [] = {
//0x00, 0x01, 0x80, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x04, 0x20, 0x00,
//0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x20, 0x00,
//0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x04, 0x20, 0x00,
//0x00, 0x04, 0x20, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x05, 0xA0, 0x00,
//0x00, 0x05, 0xA0, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x0D, 0xB0, 0x00,
//0x00, 0x19, 0x98, 0x00, 0x00, 0x33, 0xCC, 0x00, 0x00, 0x26, 0x64, 0x00, 0x00, 0x6C, 0x36, 0x00,
//0x00, 0x68, 0x16, 0x00, 0x00, 0x68, 0x16, 0x00, 0x00, 0x2C, 0x34, 0x00, 0x00, 0x27, 0xE4, 0x00,
//0x00, 0x33, 0xCC, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x01, 0x80, 0x00
//};

const unsigned char PROGMEM continuityImg [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x1F, 0xF8, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xC0, 0x0F, 0xFF, 0xFF, 0xF0,
0x1F, 0xC0, 0x03, 0xF8, 0x7F, 0x00, 0x00, 0xFE, 0xFC, 0x00, 0x00, 0x3F, 0xF8, 0x07, 0xE0, 0x1F,
0xF0, 0x3F, 0xFC, 0x0F, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x80, 0x03, 0xF0, 0x0F, 0xC0,
0x07, 0xC0, 0x03, 0xE0, 0x03, 0x80, 0x01, 0xC0, 0x01, 0x0F, 0xF0, 0x80, 0x00, 0x1F, 0xF8, 0x00,
0x00, 0x3F, 0xFC, 0x00, 0x00, 0x3C, 0x3C, 0x00, 0x00, 0x38, 0x1C, 0x00, 0x00, 0x01, 0x80, 0x00,
0x00, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM resistorImg [] = {
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x06, 0x04, 0x00, 0x00, 0x0F, 0x88,
0x00, 0x00, 0x18, 0xD0, 0x00, 0x00, 0x38, 0x60, 0x00, 0x00, 0x6C, 0x30, 0x00, 0x00, 0xC6, 0x18,
0x00, 0x00, 0x83, 0x08, 0x00, 0x01, 0x81, 0x8C, 0x00, 0x03, 0x00, 0xCC, 0x00, 0x07, 0x80, 0x78,
0x00, 0x0C, 0xC0, 0x30, 0x00, 0x18, 0x60, 0x60, 0x00, 0x38, 0x30, 0xC0, 0x00, 0x6C, 0x1B, 0x80,
0x01, 0xC6, 0x0E, 0x00, 0x03, 0x03, 0x0C, 0x00, 0x07, 0x01, 0x98, 0x00, 0x0D, 0x80, 0xF0, 0x00,
0x18, 0xC0, 0x60, 0x00, 0x30, 0x60, 0xC0, 0x00, 0x30, 0x31, 0x80, 0x00, 0x10, 0x19, 0x00, 0x00,
0x18, 0x0F, 0x00, 0x00, 0x0C, 0x06, 0x00, 0x00, 0x06, 0x0C, 0x00, 0x00, 0x0B, 0x18, 0x00, 0x00,
0x11, 0xF0, 0x00, 0x00, 0x20, 0x60, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM diodeImg [] = {
0x00, 0x07, 0xE0, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x78, 0x1E, 0x00,
0x00, 0xE0, 0x07, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x01, 0xC0, 0x03, 0x80,
0x01, 0xC0, 0x03, 0x80, 0x01, 0xC0, 0x03, 0x80, 0x01, 0xC0, 0x03, 0x80, 0x01, 0xC0, 0x03, 0x80,
0x01, 0xC0, 0x03, 0x80, 0x01, 0xC0, 0x03, 0x80, 0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0,
0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00,
0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00,
0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00, 0x00, 0x1C, 0x38, 0x00
};

// 'electric-rc-model-servo-image_csp34733171', 32x25px
const unsigned char PROGMEM servoImg [] = {
0x00, 0x00, 0x7c, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x38, 0x00,
0x00, 0x07, 0xff, 0x80, 0x00, 0x0f, 0xff, 0x80, 0x00, 0x0f, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80,
0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc,
0x07, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80,
0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80,
0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80,
0x07, 0xff, 0xff, 0x80
};

const unsigned char PROGMEM voltImg [] = {
0x1f, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xf8, 0x18, 0x00, 0x00, 0x18,
0x18, 0x00, 0x00, 0x18, 0x18, 0x0f, 0xf0, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x60, 0x06, 0x18,
0x18, 0x40, 0x02, 0x18, 0x18, 0xc1, 0x81, 0x18, 0x18, 0x81, 0x81, 0x18, 0x19, 0x81, 0x81, 0x98,
0x19, 0x01, 0x80, 0x98, 0x19, 0x03, 0xc0, 0x98, 0x18, 0x07, 0xe0, 0x18, 0x18, 0x0f, 0xf0, 0x18,
0x18, 0x00, 0x00, 0x18, 0x1f, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xf8,
0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf0, 0x0f, 0xf8, 0x1f, 0xe0, 0x07, 0xf8, 0x1f, 0xe0, 0x07, 0xf8,
0x1f, 0xe0, 0x07, 0xf8, 0x1f, 0xe0, 0x07, 0xf8, 0x1f, 0xe0, 0x07, 0xf8, 0x1f, 0xf0, 0x0f, 0xf8,
0x1f, 0xf8, 0x1f, 0xf8, 0x1f, 0xfe, 0x7f, 0xf8, 0x1f, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xf8,
};

const unsigned char PROGMEM radarImg [] {
0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x70, 0x00, 0x00, 0x7f, 0x7e, 0x00, 0x00, 0xff, 0x7f, 0x00,
0x01, 0xfe, 0x7f, 0xc0, 0x03, 0xe7, 0x67, 0xe0, 0x07, 0x9b, 0x79, 0xf0, 0x0f, 0x35, 0x7e, 0xb0,
0x0f, 0x75, 0x7e, 0x78, 0x1e, 0xfe, 0x7d, 0xb8, 0x1d, 0xf8, 0x5b, 0xbc, 0x3d, 0xf7, 0x67, 0xdc,
0x3b, 0xef, 0x67, 0xdc, 0x3b, 0xff, 0xdb, 0xde, 0x3b, 0xdf, 0x3b, 0xde, 0x00, 0x00, 0x00, 0x00,
0x3b, 0xdf, 0x7b, 0xde, 0x3b, 0xff, 0x7b, 0xde, 0x3b, 0xef, 0x7e, 0x5c, 0x3d, 0xf7, 0x75, 0x1c,
0x3d, 0xfb, 0x4e, 0xbc, 0x1e, 0xe4, 0x3f, 0xb8, 0x1e, 0xd7, 0xff, 0x78, 0x0f, 0x6f, 0x7e, 0xf0,
0x07, 0x9f, 0x7d, 0xf0, 0x07, 0xef, 0x73, 0xe0, 0x03, 0xf8, 0x1f, 0xc0, 0x00, 0xff, 0x7f, 0x80,
0x00, 0x7f, 0x7e, 0x00, 0x00, 0x1f, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'stepper', 32x32px
const unsigned char PROGMEM stepperImg [] {
0x00, 0x03, 0xc0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x03, 0xc0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x1f, 0xf7, 0xef, 0xf8, 0x1f, 0xe7, 0xe7, 0xf8,
0x3f, 0x87, 0xe1, 0xfc, 0x3f, 0x07, 0xe0, 0xfc, 0x3f, 0x07, 0xe0, 0xfc, 0x3f, 0x01, 0x80, 0xfc,
0x3f, 0x80, 0x01, 0xfc, 0x7f, 0xc0, 0x03, 0xfe, 0x7f, 0xf8, 0x1f, 0xfe, 0x7f, 0xff, 0xff, 0xfe,
0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,
0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe,
0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe,
0x7f, 0xff, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0xfe, 0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80,
};

// 'speaker', 32x32px
const unsigned char PROGMEM speakerImg [] {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x01, 0x80, 0x40, 0x00, 0x03, 0x80, 0xe0, 0x00, 0x07, 0x80, 0x70, 0x00, 0x1f, 0x80, 0x70,
0x00, 0x3f, 0x87, 0x38, 0x00, 0x7f, 0x87, 0x18, 0x00, 0xff, 0x83, 0x9c, 0x01, 0xff, 0x91, 0x8c,
0x3f, 0xff, 0xb9, 0xcc, 0x3f, 0xff, 0x98, 0xcc, 0x3f, 0xff, 0x9c, 0xce, 0x3f, 0xff, 0x8c, 0xce,
0x3f, 0xff, 0x8c, 0xce, 0x3f, 0xff, 0x8c, 0xee, 0x3f, 0xff, 0x8c, 0xce, 0x3f, 0xff, 0x9c, 0xce,
0x3f, 0xff, 0x98, 0xcc, 0x3f, 0xff, 0xb9, 0xcc, 0x01, 0xff, 0xb1, 0x8c, 0x00, 0x7f, 0x83, 0x9c,
0x00, 0x3f, 0x87, 0x18, 0x00, 0x1f, 0x87, 0x38, 0x00, 0x0f, 0x80, 0x70, 0x00, 0x07, 0x80, 0x70,
0x00, 0x03, 0x80, 0xe0, 0x00, 0x01, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// 'nospeaker', 33x32px
const unsigned char PROGMEM nospeakerImg [] {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x0c,
0x00, 0x80, 0x00, 0x00, 0x06, 0x01, 0x80, 0x60, 0x00, 0x03, 0x03, 0x80, 0x70, 0x00, 0x01, 0x87,
0x80, 0x38, 0x00, 0x00, 0xcf, 0x83, 0x18, 0x00, 0x00, 0x6f, 0x83, 0x9c, 0x00, 0x00, 0x37, 0x81,
0x8c, 0x00, 0x00, 0xdb, 0x89, 0xcc, 0x00, 0x1f, 0xed, 0x9c, 0xce, 0x00, 0x1f, 0xf6, 0x9c, 0xe6,
0x00, 0x1f, 0xfb, 0x0e, 0x66, 0x00, 0x1f, 0xfd, 0x8e, 0x66, 0x00, 0x1f, 0xfe, 0xc6, 0x66, 0x00,
0x1f, 0xff, 0x66, 0x66, 0x00, 0x1f, 0xff, 0xb6, 0x66, 0x00, 0x1f, 0xff, 0x98, 0x66, 0x00, 0x1f,
0xff, 0x8c, 0xe6, 0x00, 0x1f, 0xff, 0x96, 0xce, 0x00, 0x00, 0x7f, 0x83, 0x4c, 0x00, 0x00, 0x3f,
0x81, 0x8c, 0x00, 0x00, 0x1f, 0x82, 0xdc, 0x00, 0x00, 0x0f, 0x83, 0x68, 0x00, 0x00, 0x07, 0x80,
0x30, 0x00, 0x00, 0x03, 0x80, 0x58, 0x00, 0x00, 0x00, 0x80, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'satImg', 32x32px
const unsigned char PROGMEM satImg [] {
0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x07, 0x00, 0x33, 0x00, 0x3f, 0x80,
0x6f, 0x00, 0x79, 0xc0, 0x6c, 0x01, 0xe0, 0xc0, 0x0d, 0x83, 0x81, 0xe0, 0x01, 0xce, 0x01, 0xe0,
0x00, 0xfc, 0x03, 0xe0, 0x00, 0x78, 0x03, 0x20, 0x00, 0x78, 0x06, 0x30, 0x00, 0xfc, 0x0e, 0x30,
0x01, 0xce, 0x1c, 0x30, 0x03, 0x84, 0x38, 0x60, 0x03, 0x00, 0x70, 0x60, 0x06, 0x00, 0xe0, 0x60,
0x0c, 0x01, 0xc0, 0x60, 0x0c, 0x03, 0x80, 0xc0, 0x18, 0x07, 0x01, 0xc0, 0x18, 0x1e, 0x01, 0x80,
0x18, 0x38, 0x03, 0xc0, 0x19, 0xf0, 0x0e, 0xc0, 0x1f, 0xc0, 0x1c, 0x60, 0x0f, 0x80, 0xf8, 0x60,
0x03, 0xff, 0xe0, 0x30, 0x00, 0xff, 0xe0, 0x30, 0x00, 0x00, 0xc0, 0x18, 0x00, 0x00, 0xc0, 0x1c,
0x00, 0x01, 0x80, 0x0c, 0x00, 0x01, 0xc0, 0x0e, 0x00, 0x03, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00
};

// Init vars
//int   Pause =300;    // Pause after display - change as needed
byte  Pause =300;    // Hard pause delay in some modes
int   Raw   = 0;     // Used to read Raw values
float Vin   = 0;     // Internal Calc
float Vout  = 0;     // External calc
float R1    = 2200;  // Divider for top side of resistor and diode tester - May want to tweak based on your actual reading
float R2    = 0;     // Default for lower side of resistor and diode, gets calc


float Vin2  = 0.00;     // Used for voltage meter calcs
float Vout2 = 0.00;     
                        //float Res1= 100000.00;// resistance of R1 (100K) 
//float Res1  = 105789.00;// Tweaked to actual readin
float Res1 = 100000.00;
                        //float Res2= 10000.00; // resistance of R2 (10K) 
float Res2  = 10000.00;  // Tweaked to actual readin

int   Val = 0;        // Working vars
float buffer= 0;

int maximum=1;        // Min and Max for analog reads defaults
int minimum=1024;

int Mode=0;           // Mode - default to 0 
int maxModes = 11;    // Maximum modes for rollover

unsigned long numberOfTimes=0;
unsigned long sum=0;
int avg=0;

int Pulse=125;         // Default PWM puluse width
int servoPulse = 90;   // Used for Servo testing

byte TRIGGER_PIN = 7;  // SR04 / Sonar trigger pin
byte ECHO_PIN    = 8 ; // SRO4 / Sonar echo return pin
//long mydistance   = 0;
int mydistance = 0;

long dataTimer;           // used for logging data to the serial port, set trigger in mode
long dataTrigger = 1000;  // default to every second

long i2cTimer;    // used for I2C bus scan refreshes - could use the dataTimer I suppose but if set high then slow refresh

byte inCommand;   // serial.read input command for remote control

boolean potMode   = false; // Where to read for PWM and Servo - true = pot, false = buttons, UP/DOWN together to toggle
boolean soundOn   = true;  // Can toggle sound on / off - DOWN then SEL buttons

// Setup stepper motor option on 9,11,10,12
// NOTE!: You will need a separate supply to drive the motor or you may blow the Nano regulator
const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
Stepper myStepper(stepsPerRevolution, 9, 11, 10, 12);
int stepCount = 0;

void setup()   
{  
  // Start up the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)

  // Show opening screen
  display.clearDisplay();
      display.drawBitmap(98, 0,  nanoImg, 32, 30, 1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
//      display.print("Ver:" + curVer); // Saves some bytes for memory like this
      display.print("Ver:2020-01-01");
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans9pt7b);
      #else
        display.setCursor(0,14);
        display.setTextSize(2);
      #endif
      display.print("NanOMeter");
  display.display();

  Serial.begin(115200);   // For data logging and remote control
  
  // Input buttons
  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  
  // speaker and built in LED working together
  pinMode(13,OUTPUT);

  // Attach servo port
  myServo.attach(5);

  // Setup stepper speed
  myStepper.setSpeed(6);

  // Interrrupt routine to read the middle button
  attachInterrupt(1, buttonPress, FALLING);

  delay(3000);
  display.clearDisplay();
  display.display();
  dataTimer = millis();
  i2cTimer = millis();
}

void loop()
{
    // Up AND Select - 
    if ((digitalRead(2)==0) && (digitalRead(3)==0))
    {
      // room to toggle one more item here but no memory
      
      delay(250);
    }
    
    // Select AND Down - Toggle sound on/off - draw speaker icon to show result
    if ((digitalRead(3)==0) && (digitalRead(4)==0))
    {
      soundOn = !soundOn;
      toner(13,800,50);
      display.clearDisplay(); 
        if (soundOn)
        {display.drawBitmap(96, 0,  speakerImg, 32, 32, 1);}
        else
        {display.drawBitmap(96, 0,  nospeakerImg, 33, 32, 1);}        
      display.display();
      delay(1000);
    }
    
    // UP and DOWN - Check / toggle potMode here - not needed in each mode
    if ((digitalRead(2)==0) && (digitalRead(4)==0))
    {
      potMode = !potMode;   // toggle where we are getting information
      toner(13,800,50);
      delay(250);
    }

  // If we have a serial command then read it in
  if (Serial.available() > 0) 
  {
    inCommand = Serial.read(); // read the incoming byte:
    // M just bumps the mode - not overlly useful but how I started
    if (inCommand == 'M')
    {
      Mode++;
      toner(13,2250,50);
      if(Mode > maxModes)
      {
        Mode=0;
      }
    }
    // 0 - 9 goes directly into the requested mode, A jumps to the Adjust Data Log Timer mode
    // Obviously it would be nice to have direct commands such as 5:200 to set PWM to 200 BUT running out of memory for code
    // And I don't want to burn a lot of time optimizing things
    if (inCommand == '0'){toner(13,2250,50);Mode = 0;} // Volt meter
    if (inCommand == '1'){toner(13,2250,50);Mode = 1;} // Analog read mode
    if (inCommand == '2'){toner(13,2250,50);Mode = 2;} // Resistor read mode
    if (inCommand == '3'){toner(13,2250,50);Mode = 3;} // Diode drop mode
    if (inCommand == '4'){toner(13,2250,50);Mode = 4;} // Continuity mode
    if (inCommand == '5'){toner(13,2250,50);Mode = 5;} // PWM mode
    if (inCommand == '6'){toner(13,2250,50);Mode = 6;} // Servo tester mode
    if (inCommand == '7'){toner(13,2250,50);Mode = 7;} // Ping-SR04 mode
    if (inCommand == '8'){toner(13,2250,50);Mode = 8;} // Stepper test mode
    if (inCommand == '9'){toner(13,2250,50);Mode = 9;} // Battery test mode
    if (inCommand == 'A'){toner(13,2250,50);Mode = 10;}// Adjust data logging
    if (inCommand == 'I'){toner(13,2250,50);Mode = 11;}// I2C scan mode
  }

  // Process the Modes - These should really call subs for this but works for me for now
   
  //********************************************//
  // Volt Meter - Uses A7 as input and 100K / 10K voltage divider
  //********************************************//
  //
  if(Mode==0)
  {
    float volt3=readVcc()/1000.0;
    Val = analogRead(A7);
    Vout2 = (Val * volt3) / 1024.0; // see text
    Vin2  = Vout2 / (Res2/(Res1+Res2)); 
    // Trying to compensate for the 1N914 reverse voltage protection diode used on probes
    // This seems to need to compensate for the actual voltage more as higher votages are slightly off
    // Not a big deal for my needs but be aware of that issue
    // You can use your diode voltage drop feature to see what the diode says at the 5v point at least
    if (Vin2 > 0.0)
    {
      //Vin2 += 0.33;
      Vin2 += 0.0;
    }
    display.clearDisplay(); 
      display.drawBitmap(96, 0,  voltImg, 32, 32, 1);
      display.setFont();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.print("Volts:");
      display.setCursor(50,0);
      display.print(volt3);
      display.print("v");
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif      
      display.print(Vin2);
      display.print("v");
    display.display();

    // Send logging data to serial port
    if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("Volts,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(Vin2);
      dataTimer = millis();
    }
    delay(Pause);
  }

  //********************************************//
  // Analog input - Reads A0 and displays the analog read information along with min,max, and avg
  //********************************************//
  if(Mode==1)
  {
    if((digitalRead(4)==0) || (inCommand == '=')) // button to reset stats or inCommand of =1
    { 
      delay(100); minimum=1024; maximum=0; numberOfTimes=0; sum=0; 
    }
    display.clearDisplay();
      //display.drawBitmap(43, 0,  analogImg, 32, 28, 1);
      display.setFont();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Analog:");
      int value=analogRead(A0);
      numberOfTimes=numberOfTimes+1;
      sum=sum+value;
      avg=sum/numberOfTimes;
      if(value>maximum)
        maximum=value;
      if(value<minimum)
        minimum=value;
      display.setFont();
      int lineWide=map(value,0,1024,0,128);
      display.setCursor(70,0);
        display.print("MIN:");
      display.setCursor(95,0);
        display.print(minimum);
      display.setCursor(70,10);
        display.print("AVG:");
      display.setCursor(95,10);
        display.print(avg);
      display.setCursor(70,20);
        display.print("MAX:");
      display.setCursor(95,20);
        display.print(maximum);
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans9pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif
        display.print(value);
//      display.drawLine(0,31,lineWide,31,1);
      lineWide = map(value,0,1024,0,30);
      for (int myLine = 50; myLine < 65; myLine++)
      {
        display.drawLine(myLine,31,myLine,30-lineWide,1);
      }
   display.display();

    // Send logging data to serial port
    if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("Analog,");
      Serial.print(millis());
      Serial.print(",");
      Serial.print(value);
      Serial.print(",");
      Serial.print(minimum);
      Serial.print(",");
      Serial.print(avg);
      Serial.print(",");
      Serial.println(maximum);
      dataTimer = millis();
    }

   // Auto reset average every so often
   if(numberOfTimes>100000)
    {
      numberOfTimes=0;
      sum=0;
    }
  
  }

  //********************************************//
  // Ohm Meter - Reads A2 value and calclates the resistor value
  //********************************************//
  if(Mode==2)
  {
    display.clearDisplay(); 
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Ohms:");
      display.drawBitmap(96, 0,  resistorImg, 32, 32, 1);
      Raw= analogRead(A2);
      Vin=readVcc()/1000.0;
      buffer= Raw * Vin;
      Vout= (buffer)/1024.0;
      buffer= (Vin/Vout) -1;
      R2= R1 * buffer;
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif
      if(R2<700000)
        display.print(R2);
      if(R2>700000)
        display.print("Empty");
    display.display();
    delay(Pause);

    // Send logging data to serial port
    if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("Ohms,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(R2);
      dataTimer = millis();
    }
    
  }

  //********************************************//
  // Diode Voltage Drop - Read the A2 input and calcs the voltage drop across an diode or LED
  //********************************************//
  if(Mode==3)
  {
    display.clearDisplay();
      display.drawBitmap(96, 0,  diodeImg, 32, 32, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Voltage Drop:");
      Raw= analogRead(A2);
      Vin=readVcc()/1000.0;
      buffer= Raw * Vin;
      Vout= (buffer)/1024.0;
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif

      if(Vout==0)
      {
          display.print("Empty/0");
      }
      else
      {
         display.print(Vin-Vout);
      }
    display.display();

    // Send logging data to serial port
    if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("VoltDrop,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(Vout);
      dataTimer = millis();
    }

    delay(Pause);
  }

  //********************************************//
  // Continuity Testor - Reads A2 and uses resistance < 1 to determine continuity
  //********************************************//
  if(Mode==4)
  {
    display.clearDisplay();
      Raw= analogRead(A2);
      Vin=readVcc()/1000.0;
      buffer= Raw * Vin;
      Vout= (buffer)/1024.0;
      float continuity=Vin-Vout;
      display.drawBitmap(96, 0,  continuityImg, 32, 32, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Continuity: ");
      display.print(Vout);
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif

      if(continuity<1)
      {
        toner(13,2250,65000);
        display.print("Yes");
      }
     if(continuity>1)
     {
        noTone(13);
        display.print("None ");        
     }
   display.display();

   // Send logging data to serial port
   if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("Continuity,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(continuity);
      dataTimer = millis();
    }

 }

  //********************************************//
  // PWM output - PWM output for driving things - this is NOT servo BUT will kind of work with servo
  //********************************************//
  if(Mode==5)
  {
    if ((digitalRead(2)==0) || (inCommand == '+'))
    { 
      if(Pulse<255)
      {
        toner(13,1800,10);
        Pulse += 1; 
      } 
    }

    if((digitalRead(4)==0) || (inCommand == '-'))
    { 
      if(Pulse>1)
      {
        toner(13,1400,10);
        Pulse -= 1; 
      } 
    }
    // Recenter on = command
    if(inCommand == '=')
    {
      Pulse=127;
    }
    // If we have a pot on the analog input, use it to control the servo
    if (potMode)
    {
      Val = analogRead(A0);
      Pulse = map(Val,0,1023,0,255);
    }
 
    int lineWide2=0;
    analogWrite(6,Pulse);
    display.clearDisplay();
      //display.drawBitmap(72, 1,  pwmImg, 58, 30, 1);
      display.drawBitmap(96, 1,  pwmImg, 32, 32, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Pulse Width:");
      if (potMode)
      {
        display.print("(Pot)");
      }
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif

      display.print(Pulse);
      lineWide2= map(Pulse,0,255,0,128);
      display.drawLine(0,31,lineWide2,31,1);
   display.display();

   // Send logging data to serial port
   if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("PWM,");      
      Serial.print(millis());
      Serial.print(",");
      Serial.println(Pulse);
      dataTimer = millis();
    }

  }   

  //********************************************//
  // Servo Tester - Software Servo output on pin 5 - drives servo header
  //********************************************//
  if(Mode==6)
  {
    if((digitalRead(2)==0) || (inCommand == '+'))
    { 
      if(servoPulse<180)
      {
        //toner(13,1800,10);
        servoPulse += 1; 
      } 
    }

    if((digitalRead(4)==0) || (inCommand == '-'))
    { 
      if(servoPulse>1)
      {
        //toner(13,1400,10);
        servoPulse -= 1; 
      } 
    }
    // Recenter on = command
    if(inCommand == '=')
    {
      servoPulse=90;
    }

    // If we have a pot on the analog input, use it to control the servo
    if (potMode)
    {
      Val = analogRead(A0);
      servoPulse = map(Val,0,1023,0,180);
    }

    int lineWide2=0;
    myServo.write(servoPulse);
    
    display.clearDisplay();
      display.drawBitmap(96, 1,  servoImg, 32, 25, 1);
      //display.drawBitmap(72, 1,  pwmImg, 58, 30, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Servo:");
      if (potMode)
      {
        display.print("(Pot)");
      }
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif
      
      display.print(servoPulse);
      lineWide2= map(servoPulse,0,180,0,128);
      display.drawLine(0,31,lineWide2,31,1);
   display.display();
   myServo.refresh();
   
   // Send logging data to serial port

   if (getET(dataTimer) > dataTrigger)
   {
      Serial.print("Servo,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(servoPulse);
      dataTimer = millis();
   }
 }   

  //********************************************//
  // SR 04 ping tester
  //********************************************//
  if(Mode==7)
  {
    display.clearDisplay();
      display.drawBitmap(96, 0,  radarImg, 32, 32, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Ping Distance:");
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif

      mydistance = ReadSonar();
      display.print(mydistance);
      display.print(" cm");
    display.display();
    delay(Pause);

    // Send logging data to serial port
    if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("Sonar,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(mydistance);
      dataTimer = millis();
    }

  }

  //********************************************//
  // Stepper - Stepper on pins 9,10,11,12 - Upper right side of Nano
  //********************************************//
  if(Mode==8)
  {
    if((digitalRead(2)==0) || (inCommand == '+'))
    { 
      myStepper.step(1);
      stepCount += 1;
    }

    if((digitalRead(4)==0) || (inCommand == '-'))
    { 
      myStepper.step(-1);
      stepCount -= 1;
    }

    int lineWide2=0;

    display.clearDisplay();
      display.drawBitmap(96, 1,  stepperImg, 32, 32, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Stepper:");
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif

      display.print(stepCount);
      lineWide2= map(stepCount,-2048,2048,0,128);
      display.drawLine(0,31,lineWide2,31,1);
   display.display();
   
   // Send logging data to serial port
   if (getET(dataTimer) > dataTrigger)
   {
      Serial.print("Stepper,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(stepCount);
      dataTimer = millis();
   }
 }   


  //********************************************//
  // Internal Battery Vcc - Reads internal voltage somehow and displays
  // This cannot be the actual BATTERY voltage - must be internal based on other input?
  //********************************************//
//  if(Mode==8)
//  {
//    
//    display.clearDisplay();
//      display.drawBitmap(96, 0,  batteryImg, 32, 32, 1);
//      display.setFont();
//      display.setCursor(0,0);
//      display.setTextSize(1);
//      display.print("Battery:");
//      #ifdef useFonts
//      display.setCursor(0,24);
//        display.setFont(&FreeSans12pt7b);
//      #else
//        display.setCursor(0,10);
//        display.setTextSize(2);
//      #endif
//
//      float volt2=readVcc()/1000.0;
//      display.print(volt2);
//      display.setCursor(55,28);
//      display.print("V");
//    display.display();
//    delay(Pause);

//    // Send logging data to serial port
//    if (getET(dataTimer) > dataTrigger)
//    {
//      Serial.print("IntBatt");
//      Serial.print(",");
//      Serial.print(millis());
//      Serial.print(",");
//      Serial.println(volt2);
//      dataTimer = millis();
//    }

//  }

  //********************************************//
  // External Battery Vcc - Reads internal voltage somehow and displays
  //********************************************//
  if(Mode==9)
  {
    float volt3=readVcc()/1000.0;
    Val = analogRead(A1);
    Vout2 = (Val * volt3) / 1024.0; // see text
    Vin2  = Vout2 / (Res2/(Res1+Res2)); 
    //Vin2 += 0.64;            // Drop for protection diode
    //Vin2 -= 0.10;             // resting value no connection
    if (Vin2 > 0.9)
    {
      Vin2 += 0.33;
    }
    display.clearDisplay(); 
      display.drawBitmap(96, 0,  batteryImg, 32, 32, 1);
      display.setFont();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Ext Battery:");
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif
      display.print(Vin2);
      display.print("v");
    display.display();
    delay(Pause);

    // Send logging data to serial port
    if (getET(dataTimer) > dataTrigger)
    {
      Serial.print("ExtBatt,");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(Vin2);
      dataTimer = millis();
    }
  }

  //********************************************//
  // Adjust data dump timer
  //********************************************//
  if(Mode==10)
  {
    // If we have a pot on the analog input, use it to control the servo
    if (potMode)
    {
      Val = analogRead(A0);
      Pulse = map(Val,0,1023,0,255);
    }

    if((digitalRead(2)==0) || (inCommand == '+'))
    { 
      if(dataTrigger<60000)   // Max one minute
      {
        toner(13,1300,10);
        dataTrigger += 10; 
      } 
    }

    if((digitalRead(4)==0) || (inCommand == '-'))
    { 
      if(dataTrigger>500)   // Min 1/2 second
      {
        toner(13,1400,10);        
        dataTrigger -= 10;
      } 
    }

    // If we have a pot on the analog input, use it to control the servo
    if (potMode)
    {
      Val = analogRead(A0);
      dataTrigger = map(Val,0,1000,500,60000);
      dataTrigger = constrain(dataTrigger,500,60000);
    }

    if (inCommand == '='){dataTrigger = 1000;}
    
    int lineWide2=0;
    display.clearDisplay();
      display.drawBitmap(96, 1,  dataImg, 32, 32, 1);
      display.setFont();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("Data Log:");
      if (potMode)
      {
        display.print("(Pot)");
      }
      #ifdef useFonts
        display.setCursor(0,24);
        display.setFont(&FreeSans12pt7b);
      #else
        display.setCursor(0,10);
        display.setTextSize(2);
      #endif
      display.print(dataTrigger);
      display.print(" mS");
      lineWide2= map(dataTrigger,500,5000,0,128);
      display.drawLine(0,31,lineWide2,31,1);
   display.display();
  }

  //********************************************//
  // I2C Bus Scan
  //********************************************//
  if(Mode==11)
  {
    if (getET(i2cTimer) > 2000) // Just scan every two seconds, not every loop
    {
      i2cScan();
      i2cTimer = millis();
    }
  }
 
}
//********************************************//
// END OF MAIN LOOP
//********************************************//

// Interrupt routine to read the middle button and change Modes
void buttonPress()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 220) 
  {
    Mode++;
    toner(13,2250,50);
    if(Mode > maxModes)
    Mode=0;
  }
  last_interrupt_time = interrupt_time;
}


long readVcc() 
{
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

long ReadSonar()
{
  long duration, distance;
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration / 58.2;
 
  return distance;

}

// ** returns the ET in millis for timer variable passed
long getET(long mytimer)
{
  long result;
  result = (millis() - mytimer);
  return result;
}

// Regular old I2C scanning routine and display
void i2cScan()
{
  byte error;
  int address;
  int nDevices;

 display.clearDisplay();
   display.drawBitmap(96, 1,  satImg, 32, 32, 1);
   display.setFont();
   display.setCursor(0,0);
   display.setTextSize(1);
   display.print("I2C Scan:");
   display.setCursor(0,12);

   Serial.print("I2C,");
   Serial.print(millis());

    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0)
      {
        if (address<16)
        {
          Serial.print(",0");
          display.print("0");
        }
        else
        {
          Serial.print(",");
        }
        Serial.print(address,HEX);
        display.print(address,HEX);
        display.print(" ");
        nDevices++;
        if (nDevices == 5)      // 
        {
          display.println();
        }
      }
  }
  display.display();
  Serial.println();
}

// tone() replacement to allow muting sound via soundOn
void toner(byte mypin, int mysound, int mylen)
{
  if (soundOn)
  {
    tone(mypin, mysound, mylen);
  }
}

//****************************************************************************//
// End of the world as we know it..ÿ°
