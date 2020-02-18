/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "ESP32_nRF24"

/* Network configuration */
#define RFQUACK_NETWORK_ESP32
#define RFQUACK_NETWORK_SSID "SSID"
#define RFQUACK_NETWORK_PASS "PASS"

/* Transport configuration */
#define RFQUACK_TRANSPORT_MQTT
#define RFQUACK_MQTT_BROKER_HOST "192.168.1.104"

/* Enable Radio debug messages */
#define RFQUACK_LOG_ENABLED
#define RFQUACK_DEV
#define RADIOLIB_DEBUG

/* Disable Software Serial logging  */
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
RadioA radioA = new Module(5, 4, 22);

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
