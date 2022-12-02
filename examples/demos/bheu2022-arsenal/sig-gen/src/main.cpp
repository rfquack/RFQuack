/*
    Simulating an IoT node that transmits a signal, waits to receive a reply,
    compares the reply against a secret and displays a "success" message on the
    serial console if the reply matches the secret.

    Adapted from RadioLib RF69 transmit example.

    The original example transmits packets using RF69 FSK radio module.
    Each packet contains up to 64 bytes of data, in the form of:

        - Arduino String
        - null-terminated char array (C-string)
        - arbitrary binary data (byte array)

    For default module settings, see the wiki page
    https://github.com/jgromes/RadioLib/wiki/Default-configuration#rf69sx1231
*/

// include the library
#include <RadioLib.h>

// RF69 has the following connections:

// Radio A on feather32u4 - tested working with this wiring
// #define CS_PIN 13
// #define IRQ_PIN 11
// #define RST_PIN 9

// Radio A on featheresp32 - tested working with this wiring
// #define CS_PIN 33
// #define IRQ_PIN 23
// #define RST_PIN 27

// Radio on wemosd1minilite - tested working with this wiring
#define CS_PIN 15
#define IRQ_PIN 5
#define RST_PIN 4
#define RF69H

RF69 radio = new Module(CS_PIN, IRQ_PIN, RST_PIN);

void setup()
{
    Serial.begin(115200);

    // give us the time to attach to the serial monitor
    delay(2000);

    // initialize RF69 with default settings
    Serial.print(F("[RF69] Initializing ... "));
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }

    // NOTE: some RF69 modules use high power output,
    //       those are usually marked RF69H(C/CW).
    //       To configure RadioLib for these modules,
    //       you must call setOutputPower() with
    //       second argument set to true.
#ifdef RF69H
    Serial.print(F("[RF69] Setting high power module ... "));
    state = radio.setOutputPower(17, true);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }
#endif
}


void loop()
{
    Serial.print(F("[RF69] Transmitting packet ... "));

    // you can transmit C-string or Arduino string up to 64 characters long
    String str;
    int state = 0;

    // transmit
    state = radio.transmit("Hello World!");

    // you can also transmit byte array up to 64 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.transmit(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE)
    {
        // the packet was successfully transmitted
        Serial.println(F("success!"));
    }
    else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
    {
        // the supplied packet was longer than 64 bytes
        Serial.println(F("too long!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);
    }

    // after 2s
    delay(2000);

    // start receiving
    state = radio.receive(str);

    if (state == RADIOLIB_ERR_NONE)
    {
        // packet was successfully received
        Serial.println(F("success!"));

        // print the data of the packet
        Serial.print(F("[RF69] Data:\t\t"));
        Serial.println(str);

        // TODO compare with secret here

        // print RSSI (Received Signal Strength Indicator)
        // of the last received packet
        Serial.print(F("[RF69] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
        // timeout occurred while waiting for a packet
        Serial.println(F("timeout!"));
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);
    }

    // wait for a second before transmitting again
    delay(1000);
}
