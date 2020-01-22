/*
 * RFQuack is a versatile RF-hacking tool that allows you to sniff, analyze, and
 * transmit data over the air. Consider it as the modular version of the great
 *
 * Copyright (C) 2019 Trend Micro Incorporated
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef rfquack_radio_h
#define rfquack_radio_h

#include "rfquack_common.h"
#include "rfquack_logging.h"
#include "rfquack_transport.h"
#include "rfquack.pb.h"

#include <cppQueue.h>

// Choose _radioA type (mandatory)
#ifdef RFQUACK_RADIOA_NRF24

#include <radio/RFQnRF24.h>

typedef RFQnRF24 RadioA;
#elif defined(RFQUACK_RADIOA_CC1101)

#include <radio/RFQCC1101.h>

typedef RFQCC1101 RadioA;

#elif defined(RFQUACK_RADIOA_MOCK)

#include <radio/RFQMock.h>

typedef RFQMock RadioA;
#else
#error "Please select the driver for the first radio using RFQUACK_RADIOA_*"
#endif

// Choose radioB type (optional)
#ifdef RFQUACK_RADIOB_NRF24
#include <radio/RFQnRF24.h>
typedef RFQnRF24 RadioB;
#elif defined(RFQUACK_RADIOB_CC1101)
#include <radio/RFQCC1101.h>
typedef RFQCC1101 RadioB;
#else
#define RFQUACK_SINGLE_RADIO
typedef void RadioB;
#endif

#include "defaults/radio.h"

// Macro to execute a method on _radioA or _radioB based on whichRadio enum.
#ifndef RFQUACK_SINGLE_RADIO
#define RADIO_A_OR_B_CMD(whichRadio, ...) { \
 if (whichRadio == RADIOA){ \
    RadioA *radio = _radioA; \
    int16_t result = ERR_UNKNOWN; \
    __VA_ARGS__; \
    if (result != ERR_NONE) RFQUACK_LOG_TRACE(F("⚠️ Error follows")) \
    RFQUACK_LOG_TRACE(F(#__VA_ARGS__ " on RadioA, resultCode=%d"), result); \
  } \
 if (whichRadio == RADIOB){ \
    RadioA *radio = _radioB; \
    int16_t result = ERR_UNKNOWN; \
    __VA_ARGS__; \
    if (result != ERR_NONE) RFQUACK_LOG_TRACE(F("⚠️ Error follows")) \
    RFQUACK_LOG_TRACE(F(#__VA_ARGS__ " on RadioB, resultCode=%d"), result); \
  } \
}
#else
#define RADIO_A_OR_B_CMD(whichRadio, ...) { \
 if (whichRadio == RADIOA){ \
    RadioA *radio = _radioA; \
    int16_t result = ERR_UNKNOWN; \
    __VA_ARGS__; \
    if (result != ERR_NONE) RFQUACK_LOG_TRACE(F("⚠️ Error follows")) \
    RFQUACK_LOG_TRACE(F(#__VA_ARGS__ " on RadioA, resultCode=%d"), result); \
  } \
}
#endif

// Macro to execute a method on both _radioA and _radioB
#ifndef RFQUACK_SINGLE_RADIO
#define RADIO_A_AND_B_CMD(...) { \
  WhichRadio whichRadio = RADIOA; \
  RadioA *radio = _radioA; \
  __VA_ARGS__; \
}{ \
  WhichRadio whichRadio = RADIOB; \
   RadioA *radio = _radioB; \
  __VA_ARGS__; \
}
#else
#define RADIO_A_AND_B_CMD(...) { \
  WhichRadio whichRadio = RADIOA; \
  RadioA *radio = _radioA; \
  __VA_ARGS__; \
}
#endif

class RFQRadio {
public:
#ifdef RFQUACK_SINGLE_RADIO

    explicit RFQRadio(RadioA *radioA) : _radioA(radioA) {}

#else

    explicit RFQRadio(RadioA *radioA, RadioB *radioB) : _radioA(radioA), _radioB(radioB) {}

#endif


    /**
     * @brief Changes the radio state in the radio driver.
     * @param mode
     * @param whichRadio
     */
    void setMode(rfquack_Mode mode, WhichRadio whichRadio = RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, result = radio->setMode(mode))
    }

    /**
     * @brief Inits radio driver.
     * @param whichRadio
     */
    void begin(WhichRadio whichRadio = RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, {
        radio->setWhichRadio(whichRadio);
        result = radio->begin();
      })
    }

    /**
     * @brief Update queue statistics.
     */
    void updateRadiosStats() {
      // TODO: Wait for multiple statuses to be impl. in PB.
      // rfq.stats = _radioA->getRfquackStats();
    }


    /**
     * @brief Reads any data from each radio's RX FIFO and push it to the RX queue;
     * If any packets are in RX queue, send them all via the transport.
     * If in repeat mode, modify and retransmit.
     *
     * We handle them one at a time, even if there are more than one in the queue.
     * This is because we rather have a full RX queue but zero packet lost, and
     * we want to give other functions in the loop their share of time.
     */
    void rxLoop() {
      // Fetch packets from radios RX FIFOs. (first radioA, than radioB if any).
      RADIO_A_AND_B_CMD({ radio->rxLoop(); })

      // Fetch packets packets from both drivers queues.
      RADIO_A_AND_B_CMD({
                          rfquack_Packet pkt;

                          // Pick *ONE* packet from RX QUEUE (read method description)
                          if (radio->getRxQueue()->pop(&pkt)) {

                            // Send packet to the chain of registered modules.
                            if (modulesDispatcher.afterPacketReceived(pkt, whichRadio)) {

                              // Send packet to transport if no module prevented this from happening.
                              PB_ENCODE_AND_SEND(rfquack_Packet_fields, pkt, RFQUACK_OUT_TOPIC
                              RFQUACK_TOPIC_SEP
                              RFQUACK_TOPIC_PACKET);
                            }
                          }
                        })
    }


    void transmit(rfquack_Packet *pkt, WhichRadio whichRadio = RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, result = radio->transmit(pkt))
    }

    /**
     * @brief Read register value.
     *
     * @param addr Address of the register
     *
     * @return Value from the register.
     */
    uint8_t readRegister(uint8_t reg, WhichRadio whichRadio = RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, return radio->readRegister(reg))

      // If radio is neither A or B.
      return -1;
    }

    /**
     * @brief Write register value
     *
     * @param addr Address of the register
     *
     */
    void writeRegister(uint8_t reg, uint8_t value, WhichRadio whichRadio = RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, return radio->writeRegister(reg, value))
    }

    /**
     * @brief Sets the packet format (fixed or variable), and its length.
     *
     */
    void setPacketFormat(rfquack_PacketFormat &fmt, WhichRadio whichRadio = RADIOA) {
      if (fmt.fixed) {
        RFQUACK_LOG_TRACE("Setting radio to fixed len of %d bytes", (uint8_t) fmt.len)
        RADIO_A_OR_B_CMD(whichRadio, result = radio->fixedPacketLengthMode((uint8_t) fmt.len))
      } else {
        RFQUACK_LOG_TRACE("Setting radio to variable len ( max %d bytes)", (uint8_t) fmt.len)
        RADIO_A_OR_B_CMD(whichRadio, result = radio->variablePacketLengthMode((uint8_t) fmt.len))
      }
    }

    void setPromiscuous(bool promiscuous, WhichRadio whichRadio = RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, result = radio->setPromiscuousMode(promiscuous))
    }

    uint8_t setModemConfig(rfquack_ModemConfig &modemConfig, WhichRadio whichRadio = RADIOA) {
      uint8_t changes = 0;

      RFQUACK_LOG_TRACE(F("Changing modem configuration"))

      if (modemConfig.has_carrierFreq) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setFrequency(modemConfig.carrierFreq))
        changes++;
      }

      if (modemConfig.has_txPower) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setOutputPower(modemConfig.txPower))
        changes++;
      }

      if (modemConfig.has_preambleLen) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setPreambleLength(modemConfig.preambleLen))
        changes++;
      }

      if (modemConfig.has_syncWords) {
        RADIO_A_OR_B_CMD(whichRadio,
                         result = radio->setSyncWord(modemConfig.syncWords.bytes, modemConfig.syncWords.size))
        changes++;
      }

      return changes;
    }

private:
    RadioA *_radioA;
#ifndef RFQUACK_SINGLE_RADIO
    RadioB *_radioB;
#endif
};

#endif
