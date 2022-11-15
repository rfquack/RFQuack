/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "ESP32_CC1101"
#define RFQUACK_TOPIC_PREFIX "MAKE_THIS_UNIQUE"

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
RadioA radioA = new Module(2, 21, RADIOLIB_NC);

// Uncomment to add a new radio, then change setup(){ rfquack_setup(radioA, radioB); }
// #define USE_RADIOB
// typedef RFQCC1101 RadioB;
// RadioB radioB = new Module(3, 22, RADIOLIB_NC);


/*****************************************************************************
 * /RFQuack configuration
 *****************************************************************************/
#include <rfquack.h>

void setup() {
  rfquack_setup(&radioA);
}

 void loop() {
   rfquack_loop();
 }
