#ifndef RFQUACK_PROJECT_RFQNRF24_H
#define RFQUACK_PROJECT_RFQNRF24_H

#include "RadioLibWrapper.h"
#include "rfquack_logging.h"

class RFQnRF24 : public RadioLibWrapper<nRF24> {
public:
    RFQnRF24(Module *module) : RadioLibWrapper(module, "nRF24") {}

    int16_t begin() override {
      int16_t state = RadioLibWrapper::begin();

      return state;
    }

    virtual int16_t transmitMode() override {
      // Set up TX_ADDR to last used.
      RFQUACK_LOG_TRACE(F("Setting up TX pipe."))
      int16_t state = nRF24::setTransmitPipe(_addr);
      if (state != RADIOLIB_ERR_NONE) return state;

      return RadioLibWrapper::transmitMode();
    }

    bool isTxChannelFree() override {
      // Check if max number of retransmits has occurred.
      if (nRF24::getStatus(RADIOLIB_NRF24_MAX_RT)) {
        nRF24::standby();
        nRF24::clearIRQ();
        RFQUACK_LOG_TRACE(F("No ACK received from previous TX. Abort TX."))
        return true;
      }

      // Call to base method.
      return RadioLibWrapper::isTxChannelFree();
    }

    virtual int16_t receiveMode() override {
      if (_mode != rfquack_Mode_RX) {
        // Set up receiving pipe.
        // Note: this command will bring radio back to standby mode.
        int16_t state = nRF24::setReceivePipe(0, _addr);
        if (state != RADIOLIB_ERR_NONE) return state;

        // Call to base method.
        return RadioLibWrapper::receiveMode();
      } else {
        return RADIOLIB_ERR_NONE;
      }
    }

    int16_t readData(uint8_t *data, size_t len) override {
      // set mode to standby
      size_t length = 32;

      // read packet data
      SPIreadRxPayload(data, length);

      // add terminating null
      data[length] = 0;

      // clear status bits
      setFlag(false);
      _mod->SPIsetRegValue(
        RADIOLIB_NRF24_REG_STATUS, (
          RADIOLIB_NRF24_RX_DR | 
          RADIOLIB_NRF24_TX_DS | 
          RADIOLIB_NRF24_MAX_RT), 6, 4);

      return RADIOLIB_ERR_NONE;
    }

    // NOTE: nRF24 does not have a "setSyncword()" method since it's called "address" and is set
    // per pipe.
    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) override {
      // Note: this command will bring radio back to standby mode.
      _mode = rfquack_Mode_IDLE;

      // First try to set addr width.
      int16_t state = nRF24::setAddressWidth(size);
      if (state != RADIOLIB_ERR_NONE) return state;

      // If addr width is valid store it.
      memcpy(_addr, bytes, size);

      return RADIOLIB_ERR_NONE;
    }
    
    int16_t getSyncWord(uint8_t *bytes, pb_size_t *size) {
      if (_promiscuous) {
        // No sync words when in promiscuous mode.
        *size = 0; 
        return RADIOLIB_ERR_INVALID_SYNC_WORD;
      } else {
        *size = nRF24::_addrWidth;
        memcpy(bytes, _addr, (size_t)*size);
      }
      return RADIOLIB_ERR_NONE;
    }

    // Wrap base method since it changes radio mode.
    int16_t setOutputPower(uint32_t txPower) override {
      _mode = rfquack_Mode_IDLE;
      return nRF24::setOutputPower(txPower);
    }

    // Wrap base method since it changes radio mode and unit of measure (MHz, Hz)
    int16_t setFrequency(float carrierFreq) override {
      // _mode = rfquack_Mode_IDLE;
      auto freq = (int16_t) carrierFreq;
      RFQUACK_LOG_TRACE("Frequency = %d", freq)
      int16_t state = nRF24::setFrequency(freq);
      if (state == RADIOLIB_ERR_NONE) {
        _freq = carrierFreq;
      }
      return state;
    }
    
    int16_t getModulation(char *modulation) override {
      // nRF24 supports only GFSK
      strcpy(modulation, "GFSK");
      return RADIOLIB_ERR_NONE;
    }

    int16_t setCrcFiltering(bool crcOn) override {
      return nRF24::setCrcFiltering(crcOn);
    }

    int16_t setBitRate(float br) override {
      return(nRF24::setBitRate(br));
    }
    
    int16_t getBitRate(float *br) override {
      *br = nRF24::_dataRate;
      return RADIOLIB_ERR_NONE;
    }

    int16_t setPromiscuousMode(bool isEnabled) override {
      // Disables CRC and auto ACK.
      int16_t state = setCrcFiltering(!isEnabled);

      // Set address to 0x00AA. (mind the endianness)
      byte syncW[5] = {0xAA, 0x00};
      state |= setSyncWord(syncW, 2);

      // Set fixed packet len
      state |= fixedPacketLengthMode(32);
      
      if (state == RADIOLIB_ERR_NONE) {
        _promiscuous = isEnabled;
      }
      
      return state;
    }

    int16_t fixedPacketLengthMode(uint8_t len) override {
      // Packet cannot be longer than 32 bytes
      if (len > 32) return RADIOLIB_ERR_PACKET_TOO_LONG;

      // Turn off Dynamic Payload Length as Global feature
      int16_t state = _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_FEATURE, RADIOLIB_NRF24_DPL_OFF, 2, 2);
      RADIOLIB_ASSERT(state);

      // Set len in RX_PW_P0/1
      state = _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_RX_PW_P0, len, 5, 0);
      state |= _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_RX_PW_P1, len, 5, 0);
      return state;
    }

    int16_t variablePacketLengthMode(uint8_t len) override {
      int16_t state = _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_RX_PW_P0, len, 5, 0);
      state |= _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_RX_PW_P1, len, 5, 0);
      RADIOLIB_ASSERT(state);

      // Turn on Dynamic Payload Length as Global feature
      state |= _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_FEATURE, RADIOLIB_NRF24_DPL_ON, 2, 2);
      RADIOLIB_ASSERT(state);

      // Enable dynamic payloads on all pipes
      return _mod->SPIsetRegValue(RADIOLIB_NRF24_REG_DYNPD, RADIOLIB_NRF24_DPL_ALL_ON, 5, 0);
    }

    int16_t isCarrierDetected(bool *isDetected) override {
      // Value is correct 170uS after RX mode is issued
      *isDetected = nRF24::isCarrierDetected();
      return RADIOLIB_ERR_NONE;
    }

    int16_t setAutoAck(bool autoAckOn) override {
      return nRF24::setAutoAck(autoAckOn);
    }

    void
    writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value, uint8_t msb, uint8_t lsb) override {
      _mod->SPIsetRegValue((uint8_t) reg, (uint8_t) value, msb, lsb);
    }

    void removeInterrupts() override {
      nRF24::clearIRQ();
      detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
    }
private:
    // Config variables not provided by RadioLib, initialised with default values
    byte _addr[5] = {0x01, 0x23, 0x45, 0x67, 0x89}; // Cannot be > 5 bytes. Default len is 5.
    bool _promiscuous = false;
};


#endif //RFQUACK_PROJECT_RFQNRF24_H
