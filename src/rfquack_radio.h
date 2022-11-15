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
#include "modules/ModulesDispatcher.h"

#define _EXECUTE_CMD(radioType, command){ \
  { \
    rfquack_WhichRadio whichRadio = rfquack_WhichRadio_ ## radioType; \
    (void) whichRadio; \
    radioType *radio = _driver ## radioType; \
    command; \
  } \
}

#ifdef USE_RADIOA
#define _EXECUTE_RADIOA(command) _EXECUTE_CMD(RadioA, command)
#else
typedef NoRadio RadioA;
#define _EXECUTE_RADIOA(command) {}
#endif

#ifdef USE_RADIOB
#define _EXECUTE_RADIOB(command) _EXECUTE_CMD(RadioB, command)
#else
typedef NoRadio RadioB;
#define _EXECUTE_RADIOB(command) {}
#endif

#ifdef USE_RADIOC
#define _EXECUTE_RADIOC(command) _EXECUTE_CMD(RadioC, command)
#else
typedef NoRadio RadioC;
#define _EXECUTE_RADIOC(command) {}
#endif

#ifdef USE_RADIOD
#define _EXECUTE_RADIOD(command) _EXECUTE_CMD(RadioD, command)
#else
typedef NoRadio RadioD;
#define _EXECUTE_RADIOD(command) {}
#endif

#ifdef USE_RADIOE
#define _EXECUTE_RADIOE(command) _EXECUTE_CMD(RadioE, command)
#else
typedef NoRadio RadioE;
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
  if (result != RADIOLIB_ERR_NONE){ \
    RFQUACK_LOG_ERROR(F(message ", got code %d"), result) \
  } \
}

extern ModulesDispatcher modulesDispatcher;

class RFQRadio {
public:
    explicit RFQRadio(RadioA *_radioA, RadioB *_radioB, RadioC *_radioC, RadioD *_radioD, RadioE *_radioE) :
      _driverRadioA(_radioA), _driverRadioB(_radioB), _driverRadioC(_radioC), _driverRadioD(_radioD),
      _driverRadioE(_radioE) {
      _rxQueue = new Queue(sizeof(rfquack_Packet), RFQUACK_RADIO_RX_QUEUE_LEN, FIFO, true);
    }

    /**
     * @brief Initializes radio driver for each radio.
     */
    int16_t begin() {
      int16_t result = RADIOLIB_ERR_NONE;
      FOREACH_RADIO({
                      radio->setWhichRadio(whichRadio);
                      result |= radio->begin();
                      ASSERT_RESULT(result, "Unable to initialize radio")
                    })
      return result;
    }


    /**
     * @brief Reads any data from radios to RX queue.
     *
     * We handle them one at a time, even if there are more than one in the queue.
     * This is because we rather have a full RX queue but zero packet lost, and
     * we want to give other functions in the loop their share of time.
     */
    void rxLoop() {
      // Fetch packets from radios RX FIFOs.
      FOREACH_RADIO({ radio->rxLoop(_rxQueue); })

      // Check if any radio still has incoming data available.
      bool aRadioNeedsCpuTime = false;
      FOREACH_RADIO({
                      if (radio->isIncomingDataAvailable()) aRadioNeedsCpuTime = true;
                    })

      // Execute 'afterPacketReceived()' hook only if there's spare CPU time or queue is getting full.
      if (!aRadioNeedsCpuTime || _rxQueue->getCount() > 20) {
        rfquack_Packet pkt;

        // Pick *ONE* packet from RX QUEUE
        if (_rxQueue->pop(&pkt)) {
          // Send packet to the chain of registered modules.
          modulesDispatcher.afterPacketReceived(pkt, pkt.rxRadio);
        }
      }

    }

    /**
     * Sends a packet over the air.
     * 
     * @param pkt Packet to be sent.
     * @param whichRadio Choosen radio.
     * 
     * @return \ref status_codes
     */
    int16_t transmit(rfquack_Packet *pkt, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->transmit(pkt))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    /**
     * Read register value.
     * 
     * @param reg
     * @param whichRadio
     * @return
     */
    int16_t readRegister(uint8_t reg, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->readRegister(reg))
      unableToFindRadioError();
      return -1;
    }

    /**
     * Write value to register.
     *
     * @param addr Address of the register to write to.
     *
     */
    void writeRegister(uint8_t reg, uint8_t value, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->writeRegister(reg, value, 7, 0))
      unableToFindRadioError();
    }

    void writeRegister(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->writeRegister(reg, value, msb, lsb))
      unableToFindRadioError();
    }

    int16_t fixedPacketLengthMode(uint8_t len, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->fixedPacketLengthMode(len));
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t variablePacketLengthMode(uint8_t len, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->variablePacketLengthMode(len));
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    /**
     * Sets radio mode (TX, RX, IDLE, JAM)
     * @param mode settings to apply
     * @param whichRadio  target radio
     * @return
     */
    int16_t setMode(rfquack_Mode mode, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setMode(mode))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setAutoAck(bool autoAckOn, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setAutoAck(autoAckOn))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    rfquack_Mode getMode(rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->getMode())
      unableToFindRadioError();
      return rfquack_Mode_IDLE;
    }

    char *getChipName(rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->getChipName())
      unableToFindRadioError();
      return nullptr;
    }

    /**
     * Returns a pointer to the native driver.
     * It's up to you cast it to the right class;
     * This is useful to all driver methods.
     * @param whichRadio
     * @return void ptr to RFQCC1101, RFQnRF24, ecc.. depending on whichRadio
     */
    void *getNativeDriver(rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return (void *) radio)
      unableToFindRadioError();
      return nullptr;
    }

    int16_t setFrequency(float carrierFreq, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setFrequency(carrierFreq))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setOutputPower(uint32_t power, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setOutputPower(power))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setPreambleLength(uint32_t size, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setPreambleLength(size))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setSyncWord(uint8_t *bytes, uint8_t size, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setSyncWord(bytes, size))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setModulation(rfquack_Modulation modulation, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setModulation(modulation))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setCrcFiltering(bool useCRC, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setCrcFiltering(useCRC))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setRxBandwidth(float rxBandwidth, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setRxBandwidth(rxBandwidth))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setBitRate(float bitRate, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setBitRate(bitRate))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setFrequencyDeviation(float frequencyDeviation, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setFrequencyDeviation(frequencyDeviation))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t setPromiscuousMode(bool enabled, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->setPromiscuousMode(enabled))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t getRSSI(float *rssi, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio,
                   return radio->getRSSI(rssi))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    int16_t isCarrierDetected(bool *cd, rfquack_WhichRadio whichRadio) {
      SWITCH_RADIO(whichRadio, return radio->isCarrierDetected(cd))
      unableToFindRadioError();
      return RADIOLIB_ERR_UNKNOWN;
    }

    void unableToFindRadioError() {
      RFQUACK_LOG_ERROR(F("Unable to find radio"));
    }

private:
    Queue *_rxQueue;
    RadioA *_driverRadioA = nullptr;
    RadioB *_driverRadioB = nullptr;
    RadioC *_driverRadioC = nullptr;
    RadioD *_driverRadioD = nullptr;
    RadioE *_driverRadioE = nullptr;
};

#endif
