/*
    Simulating an IoT node that transmits a signal, waits to receive a reply,
    compares the reply against a secret and displays a "success" message on the
    serial console if the reply matches the secret.

    Adapted from RadioLib RF69 transmit example.

    - Create a random string at boot
    - TX random string at 434MHz
    - RX at 433MHZ
    - compare any RX to random string + some modifications

*/

// include the library
#include <RadioLib.h>

// Radio connections on d1_mini_lite - tested working with this wiring
#define CS_PIN 15
#define IRQ_PIN 5
#define RST_PIN 4
#define RF69H

// Radio parameters
#define TX_FREQ 434
#define TX_POWER 20
#define RX_FREQ 433
#define RX_BW 15.6
#define FREQ_DEV 5.0

// Simulating actuator
#define LED_PIN LED_BUILTIN

// Payload
#define SECRET_POS 10
#define SECRET 33
#define TX_LEN 16
#define RAND_BYTES 8
String letters[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
    "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
    "W", "X", "Y", "Z"
    };
String rand_tx = "OH OH OH ";

RF69 radio = new Module(CS_PIN, IRQ_PIN, RST_PIN);

void setupLed()
{
    pinMode(LED_PIN, OUTPUT);
}

void blinkLed()
{
    for (size_t i = 0; i < 5; i++)
    {
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        Serial.print(F("🔵 BLINK "));
    }

    Serial.println("");
}

int check_secret_content(const char *content, const char *oracle)
{
    if (content[SECRET_POS] == (oracle[SECRET_POS] ^ SECRET))  // wow: much secret!
        return 0;
    
    return 1;
}

void setup()
{
    Serial.begin(115200);

    setupLed();

    // give us the time to attach to the serial monitor
    delay(2000);

    blinkLed();

    delay(500);

    for (size_t i = 0; i < RAND_BYTES; i++)
    {
        rand_tx = rand_tx + letters[random(0, 40)];
    }

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
    state = radio.setOutputPower(TX_POWER, true);
    state |= radio.setFrequencyDeviation(FREQ_DEV);
    state |= radio.setRxBandwidth(RX_BW);
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
    // you can transmit C-string or Arduino string up to 64 characters long
    String str;
    int state = 0;

    // transmit
    state = radio.setFrequency(TX_FREQ);
    if (state == RADIOLIB_ERR_NONE)
    {
        // the packet was successfully transmitted
        Serial.println(F("---------------------------------- TX @ 434MHz"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("setFrequency(434) failed, code: "));
        Serial.println(state);
    }

    state |= radio.transmit(rand_tx);

    // you can also transmit byte array up to 64 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.transmit(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE)
    {
        // the packet was successfully transmitted
        Serial.println(F("TX success!"));
        Serial.print(F("\tData: "));
        Serial.println(rand_tx);
    }
    else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
    {
        // the supplied packet was longer than 64 bytes
        Serial.println(F("too long!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("TX failed, code: "));
        Serial.println(state);
    }

    // start receiving immediately
    state = radio.setFrequency(RX_FREQ);
    if (state == RADIOLIB_ERR_NONE)
    {
        // the packet was successfully transmitted
        Serial.println(F("---------------------------------- RX @ 433MHz"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("setFrequency(433) failed, code: "));
        Serial.println(state);
    }

    state = radio.receive(str);

    if (state == RADIOLIB_ERR_NONE)
    {
        // packet was successfully received
        Serial.println(F("success!"));

        // print the data of the packet
        Serial.print(F("RX data:\t"));
        Serial.println(str);

        if (check_secret_content(str.c_str(), rand_tx.c_str()) == 0)
        {
            blinkLed();  // simulating an actuator
        }

        // print RSSI (Received Signal Strength Indicator)
        // of the last received packet
        Serial.print(F("\t RSSI: "));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
        // timeout occurred while waiting for a packet
        Serial.println(F("RX timeout!"));
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("Other failed, code: "));
        Serial.println(state);
    }

    // wait for a second before transmitting again
    delay(1000);
}
