/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "EPS32_CC1101"

/* Transport configuration */
#define RFQUACK_TRANSPORT_SERIAL
#define RFQUACK_SERIAL_BAUD_RATE 115200


/* Radio configuration */
#define RFQUACK_RADIOA_CC1101
#define RFQUACK_RADIO_PIN_CS 5
#define RFQUACK_RADIO_PIN_GDO0 4
#define RFQUACK_RADIO_PIN_GDO1 22 // GDO1 is not used, can be set to anything.

/* Enable Radio debug messages */
#define RFQUACK_LOG_ENABLED
#define RFQUACK_DEV
#define RADIOLIB_DEBUG

/* Disable Software Serial logging */
#define RFQUACK_LOG_SS_DISABLED

/* Default radio config */
// carrier frequency:                   868.0 MHz
// bit rate:                            4.8 kbps
// Rx bandwidth:                        325.0 kHz
// frequency deviation:                 48.0 kHz
// sync word:                           0xD391

/*****************************************************************************
 * /RFQuack configuration - DO NOT EDIT BELOW THIS LINE
 *****************************************************************************/
 #include "rfquack.h"

 RadioA radio = new Module(RFQUACK_RADIO_PIN_CS, RFQUACK_RADIO_PIN_GDO0, RFQUACK_RADIO_PIN_GDO1);

 void setup() {
   rfquack_setup(radio);
 }

 void loop() {
   rfquack_loop();
 }
