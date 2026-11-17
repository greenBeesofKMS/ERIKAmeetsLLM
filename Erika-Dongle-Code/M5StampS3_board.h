//https://docs.m5stack.com/en/core/StampS3

// wiring between board and connector to Erika
// GND               // black
// +5V               // red
#define DTD_PIN 9    // white or grey, previously (<V3) hardwired to GND
#if HWVERSION < 3 
  #define RTS_PIN 42 // yellow (G42 is the tiny "in between" pad on 1.27 mm spacing and hard to solder)
#else
  #define RTS_PIN 7  // yellow (V3 changed, G7 is in 2.54 spacing and now easier to solder)
#endif
#define ERIKA_RX 44  // green, connected to hardware serial0 with UART / standard pins, G44 is with resistor for 5V input capability
#define ERIKA_TX 43  // blue, connected to hardware serial0 with UART / standard pins

#define ERIKA_BAUD 1200
#define HWSERIAL Serial0
//#include <SoftwareSerial.h>
//#define SWSERIAL

#define NUMPIXELS 1
#define PIN_NEOPIXEL 21

#define PIN_NEOPIXEL_POWER 38  // for Stamp S3a only

#if HWVERSION < 2
  #define WM_ONDEMAND_PIN 1 //V1 without button but reed switch
#else
  #define WM_ONDEMAND_PIN 0 //V2+ with use of built-in button
#endif

#define RTS_PIN_V2 42
#define RTS_PIN_V3 7
#define WM_ONDEMAND_PIN_V1 1
#define WM_ONDEMAND_PIN_V2 0