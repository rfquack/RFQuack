/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "EPS32_CC1101"

/* Transport configuration */
#define RFQUACK_TRANSPORT_SERIAL
#define RFQUACK_SERIAL_BAUD_RATE 115200

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


/* Radio configuration */
#include <radio/drivers.h>

#define USE_RADIOA
typedef RFQCC1101 RadioA;
RadioA radioA = new Module(2, 21, NC);

// #define USE_RADIOB
typedef NoRadio RadioB;
RadioB radioB;

// #define USE_RADIOC
typedef NoRadio RadioC;
RadioC radioC;

// #define USE_RADIOD
typedef NoRadio RadioD;
RadioD radioD;

// #define USE_RADIOE
typedef NoRadio RadioE;
RadioE radioE;

/*****************************************************************************
 * /RFQuack configuration - DO NOT EDIT BELOW THIS LINE
 *****************************************************************************/
#include <rfquack.h>

void setup() {
  rfquack_setup(radioA, radioB, radioC, radioD, radioE);
}

 void loop() {
   rfquack_loop();
 }
