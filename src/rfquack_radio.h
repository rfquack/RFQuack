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
#elif defined(RFQUACK_RADIOB_MOCK)

#include <radio/RFQMock.h>

typedef RFQMock RadioB;
#else
#define RFQUACK_SINGLE_RADIO
typedef void RadioB;
#endif

#include "defaults/radio.h"

// Macro to execute a method on _radioA or _radioB based on whichRadio enum.
#ifndef RFQUACK_SINGLE_RADIO
#define RADIO_A_OR_B_CMD(whichRadio, ...) { \
 if (whichRadio == rfquack_WhichRadio_RADIOA){ \
    RadioA *radio = _radioA; \
    int16_t result = ERR_UNKNOWN; \
    __VA_ARGS__; \
    if (result != ERR_NONE) RFQUACK_LOG_TRACE(F("⚠️ Error follows")) \
    RFQUACK_LOG_TRACE(F(#__VA_ARGS__ " on RadioA, resultCode=%d"), result); \
  } \
 if (whichRadio == rfquack_WhichRadio_RADIOB){ \
    RadioB *radio = _radioB; \
    int16_t result = ERR_UNKNOWN; \
    __VA_ARGS__; \
    if (result != ERR_NONE) RFQUACK_LOG_TRACE(F("⚠️ Error follows")) \
    RFQUACK_LOG_TRACE(F(#__VA_ARGS__ " on RadioB, resultCode=%d"), result); \
  } \
}
#else
#define RADIO_A_OR_B_CMD(whichRadio, ...) { \
 if (whichRadio == rfquack_WhichRadio_RADIOA){ \
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
  rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA; \
  RadioA *radio = _radioA; \
  __VA_ARGS__; \
}{ \
  rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOB; \
   RadioB *radio = _radioB; \
  __VA_ARGS__; \
}
#else
#define RADIO_A_AND_B_CMD(...) { \
  rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA; \
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
     * @brief Inits radio driver.
     * @param whichRadio
     */
    void begin(rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, {
        radio->setWhichRadio(whichRadio);
        result = radio->begin();
      })
    }

    /**
     * @brief Update queue statistics.
     */
    rfquack_Stats getRadiosStats(rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, {
        return radio->getRfquackStats();
      });
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
                            modulesDispatcher.afterPacketReceived(pkt, whichRadio);
                          }
                        })

    }


    void transmit(rfquack_Packet *pkt, rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, result = radio->transmit(pkt))
    }

    /**
     * @brief Read register value.
     *
     * @param addr Address of the register
     *
     * @return Value from the register.
     */
    uint8_t readRegister(uint8_t reg, rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
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
    void writeRegister(uint8_t reg, uint8_t value, rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, return radio->writeRegister(reg, value))
    }

    /**
     * Sets transmitted packet length.
     * @param pkt settings to apply
     * @param whichRadio target radio
     * @return
     */
    uint8_t setPacketLen(rfquack_PacketLen &pkt, rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      int len = (uint8_t) pkt.packetLen;
      if (pkt.isFixedPacketLen) {
        RFQUACK_LOG_TRACE("Setting radio to fixed len of %d bytes", len)
        RADIO_A_OR_B_CMD(whichRadio, return radio->fixedPacketLengthMode(len))
      } else {
        RFQUACK_LOG_TRACE("Setting radio to variable len ( max %d bytes)", len)
        RADIO_A_OR_B_CMD(whichRadio, return radio->variablePacketLengthMode(len))
      }
    }

    /**
     * Sets radio mode (TX, RX, IDLE, JAM)
     * @param mode settings to apply
     * @param whichRadio  target radio
     * @return
     */
    uint8_t setMode(rfquack_Mode mode, rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, return radio->setMode(mode))
    }

    rfquack_Mode getMode(rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
      RADIO_A_OR_B_CMD(whichRadio, return radio->getMode())
    }

    uint8_t
    setModemConfig(rfquack_ModemConfig &modemConfig, rfquack_WhichRadio whichRadio = rfquack_WhichRadio_RADIOA) {
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

      if (modemConfig.has_isPromiscuous) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setPromiscuousMode(modemConfig.isPromiscuous))
        changes++;
      }

      if (modemConfig.has_modulation) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setModulation(modemConfig.modulation))
      }

      if (modemConfig.has_useCRC) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setCrcFiltering(modemConfig.useCRC))
      }

      if (modemConfig.has_rxBandwidth) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setRxBandwidth(modemConfig.rxBandwidth))
      }

      if (modemConfig.has_bitRate) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setBitRate(modemConfig.bitRate))
      }

      if (modemConfig.has_frequencyDeviation) {
        RADIO_A_OR_B_CMD(whichRadio, result = radio->setFrequencyDeviation(modemConfig.frequencyDeviation))
      }

      return changes;
    }

public:
    RadioA *_radioA;
#ifndef RFQUACK_SINGLE_RADIO
    RadioB *_radioB;
#endif
};

#endif
