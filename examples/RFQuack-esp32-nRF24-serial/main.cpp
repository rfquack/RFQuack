/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "ESP32_nRF24"
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
// carrier frequency:           2400 MHz
// data rate:                   1000 kbps
// output power:                -12 dBm
// address width:               5 bytes
// address                      0x01, 0x23, 0x45, 0x67, 0x89


/* Radio configuration */
#include <radio/drivers.h>

#define USE_RADIOA
typedef RFQnRF24 RadioA;
RadioA radioA = new Module(5, 4, 22); // CS, IRQ, CE

// Uncomment to add a new radio, then change setup(){ rfquack_setup(radioA, radioB); }
// #define USE_RADIOB
// typedef RFQnRF24 RadioB;
// RadioB radioB = new Module(6, 7, RADIOLIB_NC);

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
