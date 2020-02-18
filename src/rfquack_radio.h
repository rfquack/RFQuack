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
#include "radio/drivers.h"
#include "defaults/radio.h"

#define _EXECUTE_CMD(radioType, command){ \
  { \
    rfquack_WhichRadio whichRadio = rfquack_WhichRadio_ ## radioType; \
    radioType *radio = _driver ## radioType; \
    command; \
  } \
}

#ifdef USE_RADIOA
#define _EXECUTE_RADIOA(command) _EXECUTE_CMD(RadioA, command)
#else
#define _EXECUTE_RADIOA(command) {}
#endif

#ifdef USE_RADIOB
#define _EXECUTE_RADIOB(command) _EXECUTE_CMD(RadioB, command)
#else
#define _EXECUTE_RADIOB(command) {}
#endif

#ifdef USE_RADIOC
#define _EXECUTE_RADIOC(command) _EXECUTE_CMD(RadioC, command)
#else
#define _EXECUTE_RADIOC(command) {}
#endif

#ifdef USE_RADIOD
#define _EXECUTE_RADIOD(command) _EXECUTE_CMD(RadioD, command)
#else
#define _EXECUTE_RADIOD(command) {}
#endif

#ifdef USE_RADIOE
#define _EXECUTE_RADIOE(command) _EXECUTE_CMD(RadioE, command)
#else
#define _EXECUTE_RADIOE(command) {}
#endif

#define _SWITCH_RADIO(_whichRadio, radioType, command){ \
  if (_whichRadio == rfquack_WhichRadio_ ## radioType){ \
    command; \
  } \
}

// Macro to execute a method on correct _radioX base on whichRadio variable.
#define SWITCH_RADIO(_whichRadio, command) { \
  _SWITCH_RADIO(_whichRadio, RadioA, _EXECUTE_RADIOA(command)) \
  _SWITCH_RADIO(_whichRadio, RadioB, _EXECUTE_RADIOB(command)) \
  _SWITCH_RADIO(_whichRadio, RadioC, _EXECUTE_RADIOC(command)) \
  _SWITCH_RADIO(_whichRadio, RadioD, _EXECUTE_RADIOD(command)) \
  _SWITCH_RADIO(_whichRadio, RadioE, _EXECUTE_RADIOE(command)) \
}

// Macro to execute a method on each _radioX
#define FOREACH_RADIO(command) { \
  _EXECUTE_RADIOA(command) \
  _EXECUTE_RADIOB(command) \
  _EXECUTE_RADIOC(command) \
  _EXECUTE_RADIOD(command) \
  _EXECUTE_RADIOE(command) \
}

#define ASSERT_RESULT(result, message){ \
  if (result != ERR_NONE){ \
    RFQUACK_LOG_ERROR(F(message ", got code %d"), result) \
  } \
}


class RFQRadio {
public:
    explicit RFQRadio(RadioA *_radioA, RadioB *_radioB, RadioC *_radioC, RadioD *_radioD, RadioE *_radioE) :
      _driverRadioA(_radioA), _driverRadioB(_radioB), _driverRadioC(_radioC), _driverRadioD(_radioD),
      _driverRadioE(_radioE) {}

    /**
     * @brief Inits radio driver.
     * @param whichRadio
     */
    void begin() {
      FOREACH_RADIO({
                      radio->setWhichRadio(whichRadio);
                      uint8_t result = radio->begin();
                      ASSERT_RESULT(result, "Unable to initialize radio")
                    })
    }

    /**
     * @brief Update queue statistics.
     */
    rfquack_Stats getRadiosStats(rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, {
        return radio->getRfquackStats();
      });
      RFQUACK_LOG_ERROR(F("Unable to fetch radio stats"))
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
      FOREACH_RADIO({ radio->rxLoop(); })

      // Check if any radio still has incoming data available.
      bool aRadioNeedsCpuTime = false;
      FOREACH_RADIO({
                      if (radio->isIncomingDataAvailable()) aRadioNeedsCpuTime = true;
                    })

      // Fetch packets from drivers only if we have spare CPU time or queues are getting full.
      FOREACH_RADIO({
                      if (!aRadioNeedsCpuTime || radio->getRxQueue()->getRemainingCount() < 20) {
                        rfquack_Packet pkt;

                        // Pick *ONE* packet from RX QUEUE (read method description)
                        if (radio->getRxQueue()->pop(&pkt)) {
                          // Send packet to the chain of registered modules.
                          modulesDispatcher.afterPacketReceived(pkt, pkt.rxRadio);
                        }
                      }
                    }
      )

    }


    uint8_t

    transmit(rfquack_Packet
             *pkt,
             rfquack_WhichRadio whichRadio
    ) {
      uint8_t result = ERR_UNKNOWN;
      SWITCH_RADIO(whichRadio, return radio->transmit(pkt))
      ASSERT_RESULT(result, "Unable to transmit message")
    }

/**
 * @brief Read register value.
 *
 * @param addr Address of the register
 *
 * @return Value from the register.
 */
    uint8_t readRegister(uint8_t reg, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio,
                   return radio->readRegister(reg))
      return -1;
    }

/**
 * @brief Write register value
 *
 * @param addr Address of the register
 *
 */
    void writeRegister(uint8_t reg, uint8_t value, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio,
                   return radio->writeRegister(reg, value))
    }

/**
 * Sets transmitted packet length.
 * @param pkt settings to apply
 * @param whichRadio target radio
 * @return
 */
    uint8_t setPacketLen(rfquack_PacketLen &pkt, rfquack_WhichRadio whichRadio) {
      int len = (uint8_t) pkt.packetLen;
      if (pkt.isFixedPacketLen) {
        RFQUACK_LOG_TRACE("Setting radio to fixed len of %d bytes", len)
        SWITCH_RADIO(whichRadio,
                     return radio->fixedPacketLengthMode(len))
      } else {
        RFQUACK_LOG_TRACE("Setting radio to variable len ( max %d bytes)", len)
        SWITCH_RADIO(whichRadio,
                     return radio->variablePacketLengthMode(len))
      }

      RFQUACK_LOG_ERROR(F("Unable to set packet len"))
      return ERR_UNKNOWN;
    }

/**
 * Sets radio mode (TX, RX, IDLE, JAM)
 * @param mode settings to apply
 * @param whichRadio  target radio
 * @return
 */
    uint8_t setMode(rfquack_Mode mode, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio,
                   return radio->setMode(mode))
      RFQUACK_LOG_ERROR(F("Unable to set mode"))
      return ERR_UNKNOWN;
    }

    rfquack_Mode getMode(rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio,
                   return radio->getMode())
      RFQUACK_LOG_ERROR(F("Unable to get radio mode"))
      return rfquack_Mode_IDLE;
    }

#define ASSERT_SET_MODEM_CONFIG(error) { \
  if (result == ERR_NONE){ \ 
      changes++; \
    }else{ \
      failures++; \
      RFQUACK_LOG_ERROR(F(error)) \
    } \
}

    void
    setModemConfig(rfquack_ModemConfig &modemConfig, rfquack_WhichRadio whichRadio, uint8_t &changes,
                   uint8_t &failures) {
      uint8_t result = ERR_UNKNOWN;
      RFQUACK_LOG_TRACE(F("Changing modem configuration"))

      if (modemConfig.has_carrierFreq) {
        SWITCH_RADIO(whichRadio, result = radio->setFrequency(modemConfig.carrierFreq))
        ASSERT_SET_MODEM_CONFIG("Unable to set frequency")
      }

      if (modemConfig.has_txPower) {
        SWITCH_RADIO(whichRadio, result = radio->setOutputPower(modemConfig.txPower))
        ASSERT_SET_MODEM_CONFIG("Unable to set tx power")
      }

      if (modemConfig.has_preambleLen) {
        SWITCH_RADIO(whichRadio, result = radio->setPreambleLength(modemConfig.preambleLen))
        ASSERT_SET_MODEM_CONFIG("Unable to set preamble len")
      }

      if (modemConfig.has_syncWords) {
        SWITCH_RADIO(whichRadio,
                     result = radio->setSyncWord(modemConfig.syncWords.bytes, modemConfig.syncWords.size))
        ASSERT_SET_MODEM_CONFIG("Unable to set syncword")
      }

      if (modemConfig.has_isPromiscuous) {
        SWITCH_RADIO(whichRadio, result = radio->setPromiscuousMode(modemConfig.isPromiscuous))
        ASSERT_SET_MODEM_CONFIG("Unable to set promiscuous mode")
      }

      if (modemConfig.has_modulation) {
        SWITCH_RADIO(whichRadio, result = radio->setModulation(modemConfig.modulation))
        ASSERT_SET_MODEM_CONFIG("Unable to set modulation")
      }

      if (modemConfig.has_useCRC) {
        SWITCH_RADIO(whichRadio, result = radio->setCrcFiltering(modemConfig.useCRC))
        ASSERT_SET_MODEM_CONFIG("Unable to set useCRC")
      }

      if (modemConfig.has_rxBandwidth) {
        SWITCH_RADIO(whichRadio, result = radio->setRxBandwidth(modemConfig.rxBandwidth))
        ASSERT_SET_MODEM_CONFIG("Unable to set rxBandwidth")
      }

      if (modemConfig.has_bitRate) {
        SWITCH_RADIO(whichRadio, result = radio->setBitRate(modemConfig.bitRate))
        ASSERT_SET_MODEM_CONFIG("Unable to set bitRate")
      }

      if (modemConfig.has_frequencyDeviation) {
        SWITCH_RADIO(whichRadio, result = radio->setFrequencyDeviation(modemConfig.frequencyDeviation))
        ASSERT_SET_MODEM_CONFIG("Unable to set frequencyDeviation")
      }
    }

public:
    RadioA *_driverRadioA = nullptr;
    RadioB *_driverRadioB = nullptr;
    RadioC *_driverRadioC = nullptr;
    RadioD *_driverRadioD = nullptr;
    RadioE *_driverRadioE = nullptr;
};

#endif
