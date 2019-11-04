/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "HUZZAH_RF69HW"

/* Network configuration */
#define RFQUACK_NETWORK_ESP8266
#include "wifi_credentials.h" // <- not committed because it contains secrets

/* Transport configuration */
#define RFQUACK_TRANSPORT_MQTT
#define RFQUACK_MQTT_BROKER_HOST "192.168.42.225"

#define RFQUACK_MQTT_BROKER_PORT 1883

/* Radio configuration */
#define RFQUACK_RADIO_RF69
#define RFQUACK_RADIO_HIGHPOWER 1 // for RF69HCW (note the "H")

#define RFQUACK_RADIO_PIN_CS                    15 
#define RFQUACK_RADIO_PIN_IRQ                   4
#define RFQUACK_RADIO_PIN_RST                   5

#define RFQUACK_LOG_SS_DISABLED

/*****************************************************************************
 * /RFQuack configuration - DO NOT EDIT BELOW THIS LINE
 *****************************************************************************/

#include "rfquack.h"

void setup() { rfquack_setup(); }
void loop() { rfquack_loop(); }
