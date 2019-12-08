/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "HUZZAH32_RF69HW"

/* Network configuration */
#define RFQUACK_NETWORK_WIFI
#include "wifi_credentials.h" // <- not committed because it contains secrets

/* Transport configuration */
#define RFQUACK_TRANSPORT_MQTT
#define RFQUACK_MQTT_BROKER_HOST "192.168.42.225"

#define RFQUACK_MQTT_BROKER_PORT 1883

/* Logging configuration */
#define RFQUACK_LOG_ENABLED
#define RFQUACK_LOG_SS_DISABLED

/* Radio configuration */
#define RFQUACK_RADIO_RF69
#define RFQUACK_RADIO_HIGHPOWER 1 // for RF69HCW (note the "H")
#define RFQUACK_DEBUG_RADIO false

#define RFQUACK_RADIO_PIN_CS 13   // beware, it's the LED
#define RFQUACK_RADIO_PIN_IRQ 27
#define RFQUACK_RADIO_PIN_RST 15

/*****************************************************************************
 * /RFQuack configuration - DO NOT EDIT BELOW THIS LINE
 *****************************************************************************/

#include "rfquack.h"

void setup() { rfquack_setup(); }
void loop() { rfquack_loop(); }
