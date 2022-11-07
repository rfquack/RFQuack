#ifndef RFQUACK_PROJECT_RFQCC1101_H
#define RFQUACK_PROJECT_RFQCC1101_H

#include "RadioLibWrapper.h"
#include "rfquack_logging.h"



void IRAM_ATTR radioInterruptP(void *mod);
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
      //Promiscuous mode sync word matching is fully manual process look into interrupt for more info
      if (_promiscuous) {
        if (size == 0) {
          _syncWords[0]=0;
          _syncWords[1]=0;
        } else if (size == 1) {
          _syncWords[0]=0;
          _syncWords[1]=bytes[0];
        } else {
          _syncWords[0]=bytes[0];
          _syncWords[1]=bytes[1];
        }
        return RADIOLIB_ERR_NONE;
      }

      if (size == 0) {
        // Warning: as side effect this also disables preamble generation / detection.
        // CC1101 Datasheet states: "It is not possible to only insert preamble or
        // only insert a sync word"
        RFQUACK_LOG_TRACE(F("Preamble and SyncWord disabled."))
        _syncWords[0] = 0;
        _syncWords[1] = 0;
        return CC1101::disableSyncWordFiltering();
      }

      // Call to base method.
      int16_t state = CC1101::setSyncWord(bytes, size, 0, true);
      if (state == RADIOLIB_ERR_NONE) {
        memcpy(_syncWords, bytes, size);
      }
      return state;
    }
    
    int16_t getSyncWord(uint8_t *bytes, pb_size_t &size) override {
      if (CC1101::_promiscuous) {
        size = 2;
        bytes[0] = _syncWords[0];
        bytes[1] = _syncWords[1];
        // No sync words when in promiscuous mode.
        //size = 0;
        return RADIOLIB_ERR_NONE;
      } else {
        size = CC1101::_syncWordLength;
        memcpy(bytes, _syncWords, size);
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

      if (_promiscuous) {
        CC1101::receiveDirect(true);
        attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), radioInterruptP, (void *) (this), FALLING);
      } else {
        // Issue receive mode.
        SPIsendCommand(RADIOLIB_CC1101_CMD_RX);
      }
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
      if (_promiscuous) {
        if (_resetbuffer) return false;
        if ((_lastOneMs > 0) && ((millis() - _lastOneMs) < 500) && (bufferIdx < 127)) return false;
        return bufferIdx > 0;
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

      if (_promiscuous) {
        for (int i = 0; (i < bufferIdx) && (i < len); i++) {
          data[i] = _buffer[i];
        }
        _resetbuffer = true;
        return RADIOLIB_ERR_NONE;
      }

      return CC1101::readData(data, len);
    }

    int16_t getFrequencyDeviation(float &freqDev) override {
      if (CC1101::_modulation == RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK) {
        // In OOK frequency deviation is zero
        freqDev = 0.0;
      } else {
        freqDev = CC1101::_freqDev;
      }
      return RADIOLIB_ERR_NONE;
    }

    int16_t setModulation(rfquack_Modulation modulation) override {
       RFQUACK_LOG_TRACE(F("setting modulation %d"), modulation)
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

    float getRSSI(float &rssi) override {
      CC1101::_rawRSSI = SPIreadRegister(RADIOLIB_CC1101_REG_RSSI);
      rssi = CC1101::getRSSI();
      return RADIOLIB_ERR_NONE;
    }

    int16_t isCarrierDetected(bool &isDetected) override {
      uint8_t pktStatus = SPIreadRegister(RADIOLIB_CC1101_REG_PKTSTATUS);
      isDetected = pktStatus & 0x40;
      return RADIOLIB_ERR_NONE;
    }

    void writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value, uint8_t msb, uint8_t lsb) override {
      SPIsetRegValue((uint8_t) reg, (uint8_t) value, msb, lsb, 0);
    }

    void removeInterrupts() override {
      if (_intUp) {
          detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
          _intUp = false;
      }
    }

    void setInterruptAction(void (*func)(void *)) override {
      _intUp = true;
      if (_promiscuous) {
        attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), radioInterruptP, (void *) (&_flag), FALLING);
      } else {
        attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
      }
    }
    int16_t setFrequencyDeviation(float freqDev) override {
      freqDev = CC1101::setFrequencyDeviation(freqDev);
      return RADIOLIB_ERR_NONE;
    }
    int16_t setFrequency(float freq) override {
      CC1101::setFrequency(freq);
      return RADIOLIB_ERR_NONE;
    }

    size_t getPacketLength(bool update) override {
      if (_promiscuous) {
        return bufferIdx;
      } else {
        return CC1101::getPacketLength(update);
      }
    }

    /**
     * Puts radio in promiscuous mode.
     * @param isPromiscuous
     * @return result code (ERR_NONE or ERR_COMMAND_NOT_IMPLEMENTED)
     */
    int16_t setPromiscuousMode(bool isPromiscuous) {
      CC1101::setPromiscuousMode(isPromiscuous);
      if (isPromiscuous) {
        CC1101::disableSyncWordFiltering(true); //carrier detect
        pinMode(_mod->getGpio(), INPUT);
        
        _promiscuous = true;
        RFQUACK_LOG_TRACE(F("activating PromiscuousMode"))
      } else {
        _promiscuous = false;
      }
      return RADIOLIB_ERR_NONE;
    }
    
    void parseInterrupt() {
      static uint8_t tmp = 0;
      static uint8_t bitIdx = 0;
      static uint16_t syncbuf = 0;


      if (_resetbuffer) {
        bitIdx = 8;
        tmp = 0;
        bufferIdx = 0;
        _resetbuffer = false;
        _resync = true;
        syncbuf = 0;
      }

      if ((_resync) && ((_syncWords[1] != 0) || (_syncWords[0] != 0))) {
        // rotate 1 bit in syncbuf
        syncbuf = (syncbuf << 1) | digitalRead(_mod->getGpio());
        
        // check if syncbuf matches syncword
        bool syncmatch = true;
        
        if (_syncWords[1] != 0) {
          syncmatch &= ((syncbuf & 0xFF) == _syncWords[1]);
        }
        if(_syncWords[0] != 0) {
          syncmatch &= (((syncbuf >> 8 ) & 0xFF) == _syncWords[0]);
        }

        if (!syncmatch) {
          return;
        }

        //copy the syncbuf to _buffer
        if (_syncWords[0] != 0) {
          _buffer[bufferIdx++] = 0xff & (syncbuf >> 8);
        }
        _buffer[bufferIdx++] = 0xff & syncbuf;

        _lastOneMs = millis();
        _resync = false;
        return;
      }


      if (digitalRead(_mod->getGpio())) {
        bitIdx--;
        tmp += 1 << bitIdx;
        _lastOneMs = millis();
        _resync = false;
      } else {
        //if buffer is empty sync on the first 1 transmitted
        //this is a fallback in case of empty sync word
        if (!_resync) {
          bitIdx--;
        }
      }
      
      if (bitIdx == 0) {
        bitIdx = 8;
        _buffer[bufferIdx] = tmp;
        if (tmp == 0) {
          _zerocount++;
        } else {
          _zerocount = 0;
        }
        if (_zerocount >= 3) {
          _resync = true;
          bitIdx = 8;
        }

        tmp = 0;
        _flag = true;
        if (bufferIdx < 128) bufferIdx++;
      }


    }

 
    

private:
    volatile uint8_t bufferIdx;    
    uint8_t _buffer[149];
    bool _resetbuffer = false;
    bool _promiscuous = false;
    bool _intUp = false;
    bool _resync = true;
    uint8_t _zerocount = 0;
    uint32_t _lastOneMs = 0;
    // Config variables not provided by RadioLib, initialised with default values
    byte _syncWords[2] = {0xD3, 0x91};
};


    void IRAM_ATTR radioInterruptP(void *mod) {
      //RFQUACK_LOG_TRACE(F("P int"));
      RFQCC1101 * cmod = ((RFQCC1101 *) mod);
      cmod -> parseInterrupt();
    }
#endif //RFQUACK_PROJECT_RFQCC1101_H
