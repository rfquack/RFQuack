/*****************************************************************************
 * RFQuack configuration/
 *****************************************************************************/

/* ID definition */
#define RFQUACK_UNIQ_ID "HUZZAH32_RF69HW"

/* Transport configuration */
#define RFQUACK_TRANSPORT_SERIAL
#define RFQUACK_SERIAL_BAUD_RATE 115200

/* Logging configuration */
#define RFQUACK_LOG_ENABLED
#define RFQUACK_LOG_SS_DISABLED true

/* Radio configuration */
#define RFQUACK_RADIO_RF69
#define RFQUACK_RADIO_HIGHPOWER 1 // for RF69HCW (note the "H")

#define RFQUACK_RADIO_PIN_CS 13   // beware, it's the LED
#define RFQUACK_RADIO_PIN_IRQ 27
#define RFQUACK_RADIO_PIN_RST 15

/*****************************************************************************
 * /RFQuack configuration - DO NOT EDIT BELOW THIS LINE
 *****************************************************************************/

#include "rfquack.h"

void setup() { rfquack_setup(); }
void loop() { rfquack_loop(); }
