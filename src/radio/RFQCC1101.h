#ifndef RFQUACK_PROJECT_RFQCC1101_H
#define RFQUACK_PROJECT_RFQCC1101_H

#include "RadioLibWrapper.h"
#include "rfquack_logging.h"

class RFQCC1101 : public RadioLibWrapper<CC1101> {
public:
    using CC1101::setPreambleLength;
    using CC1101::fixedPacketLengthMode;
    using CC1101::setOutputPower;
    using CC1101::setPromiscuousMode;
    using CC1101::variablePacketLengthMode;
    using CC1101::setCrcFiltering;
    using CC1101::setRxBandwidth;
    using CC1101::setBitRate;
    using CC1101::setFrequencyDeviation;
    using CC1101::getFrequencyDeviation;
    using CC1101::setFrequency;

    RFQCC1101(Module *module) : RadioLibWrapper(module, "CC1101") {}

    int16_t begin() override {
      int16_t state = RadioLibWrapper::begin();

      if (state != RADIOLIB_ERR_NONE) return state;

      // Set MAX_DVGA_GAIN: Disable the highest step amplification,
      // This will prevent noise to be amplified and trigger the CS.
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAX_DVGA_GAIN_1, 7, 6);

      // Same as above could be achieved by reducing the LNA again. Both seem to work well, just pick one.
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_LNA_GAIN_REDUCE_17_1_DB, 5, 3);

      // MAGN_TARGET: Set the target for the amplifier loop.
      //state |= SPIsetRegValue(CC1101_REG_AGCCTRL2, CC1101_MAGN_TARGET_33_DB, 2, 0);

      // Remove whitening
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF, 6, 6);

      // Do not append status byte
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, RADIOLIB_CC1101_APPEND_STATUS_OFF, 2, 2);

      return state;
    }

    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) override {
      if (size == 0) {
        // Warning: as side effect this also disables preamble generation / detection.
        // CC1101 Datasheet states: "It is not possible to only insert preamble or
        // only insert a sync word"
        RFQUACK_LOG_TRACE(F("Preamble and SyncWord disabled."))
        return CC1101::disableSyncWordFiltering();
      }

      // Call to base method.
      int16_t state = CC1101::setSyncWord(bytes, size, 0, true);
      if (state == RADIOLIB_ERR_NONE) {
        memcpy(_syncWords, bytes, size);
      }
      return state;
    }
    
    int16_t getSyncWord(uint8_t *bytes, pb_size_t *size) override {
      if (CC1101::_promiscuous) {
        // No sync words when in promiscuous mode.
        *size = 0;
        return RADIOLIB_ERR_INVALID_SYNC_WORD;
      } else {
        *size = CC1101::_syncWordLength;
        memcpy(bytes, _syncWords, (size_t)*size);
      }
      return RADIOLIB_ERR_NONE;
    }

    int16_t receiveMode() override {
      if (_mode == rfquack_Mode_RX) {
        return RADIOLIB_ERR_NONE;
      }

      // Set mode to standby (needed to flush fifo)
      standby();

      // Flush RX FIFO
      SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);

      _mode = rfquack_Mode_RX;

      // Stay in RX mode after a packet is received.
      uint8_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MCSM1, RADIOLIB_CC1101_RXOFF_RX, 3, 2);

      // set GDO0 mapping. Asserted when RX FIFO > THR.
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_RX_FIFO_FULL_OR_PKT_END);

      // Set THR to 4 bytes
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, RADIOLIB_CC1101_FIFO_THR_TX_61_RX_4, 3, 0);

      if (state != RADIOLIB_ERR_NONE) return state;

      // Issue receive mode.
      SPIsendCommand(RADIOLIB_CC1101_CMD_RX);
      _mode = rfquack_Mode_RX;

      return RADIOLIB_ERR_NONE;
    }

    void scal() {
      SPIsendCommand(RADIOLIB_CC1101_CMD_CAL);
    }

    bool isIncomingDataAvailable() override {
      // Makes sense only if in RX mode.
      if (_mode != rfquack_Mode_RX) {
        return false;
      }

      // GDO0 is asserted if:
      // "RX FIFO is filled at or above the RX FIFO threshold or the end of packet is reached"
      return digitalRead(_mod->getIrq());
    }

    int16_t readData(uint8_t *data, size_t len) override {
      // Exit if not in RX mode.
      if (_mode != rfquack_Mode_RX) {
        RFQUACK_LOG_TRACE(F("Trying to readData without being in RX mode."))
        return ERR_WRONG_MODE;
      }

      return CC1101::readData(data, len);
    }

    int16_t setModulation(rfquack_Modulation modulation) override {
      if (modulation == rfquack_Modulation_OOK) {
        return CC1101::setOOK(true);
      }
      if (modulation == rfquack_Modulation_FSK2) {
        return CC1101::setOOK(false);
      }
      return RADIOLIB_ERR_UNSUPPORTED_ENCODING;
    }
    
    int16_t getModulation(char *modulation) override {
      if (CC1101::_modulation == RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK) {
        strcpy(modulation, "OOK");
        return RADIOLIB_ERR_NONE;
      }
      if (CC1101::_modulation == RADIOLIB_CC1101_MOD_FORMAT_2_FSK) {
        strcpy(modulation, "FSK2");
        return RADIOLIB_ERR_NONE;
      }
      return RADIOLIB_ERR_UNSUPPORTED_ENCODING;
    }

    int16_t jamMode() override {
      // If the TX FIFO is empty, the modulator will continue to send preamble bytes until the first
      // byte is written to the TX FIFO.

      // Set a sync word (no sync words means no preamble generation)
      byte syncW[] = {0xFF, 0xFF};
      uint16_t state = this->setSyncWord(syncW, 2);
      if (state != RADIOLIB_ERR_NONE) return state;

      // Enable FSK mode with 0 frequency deviation
      state = this->setModulation(rfquack_Modulation_FSK2);
      state |= this->setFrequencyDeviation(0);
      if (state != RADIOLIB_ERR_NONE) return state;

      // Set bitrate to 1
      state = this->setBitRate(1);
      if (state != RADIOLIB_ERR_NONE) return state;

      // Put radio in TX Mode
      state = this->transmitMode();
      if (state != RADIOLIB_ERR_NONE) return state;

      // Put radio in fixed len mode
      state = this->fixedPacketLengthMode(1);
      if (state != RADIOLIB_ERR_NONE) return state;

      // Transmit an empty packet.
      rfquack_Packet packet = rfquack_Packet_init_zero;
      packet.data.bytes[0] = 0xFF;
      packet.data.size = 0;
      this->transmit(&packet);

      return RADIOLIB_ERR_NONE;
    }

    float getRSSI(float *rssi) override {
      CC1101::_rawRSSI = SPIreadRegister(RADIOLIB_CC1101_REG_RSSI);
      *rssi = CC1101::getRSSI();

      return RADIOLIB_ERR_NONE;
    }

    int16_t isCarrierDetected(bool *isDetected) override {
      uint8_t pktStatus = SPIreadRegister(RADIOLIB_CC1101_REG_PKTSTATUS);
      *isDetected = (pktStatus & 0x40);
      return RADIOLIB_ERR_NONE;
    }

    void writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value, uint8_t msb, uint8_t lsb) override {
      SPIsetRegValue((uint8_t) reg, (uint8_t) value, msb, lsb, 0);
    }

    void removeInterrupts() override {
      detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
    }

private:
    // Config variables not provided by RadioLib, initialised with default values
    byte _syncWords[RADIOLIB_CC1101_DEFAULT_SW_LEN] = RADIOLIB_CC1101_DEFAULT_SW;
};

#endif //RFQUACK_PROJECT_RFQCC1101_H
