#ifndef RFQUACK_PROJECT_RFQRF69_H
#define RFQUACK_PROJECT_RFQRF69_H


#include "RadioLibWrapper.h"
#include "rfquack_logging.h"

class RFQRF69 : public RadioLibWrapper<RF69> {
public:
    using RF69::setPreambleLength;
    using RF69::setOutputPower;
    using RF69::setPromiscuousMode;
    using RF69::variablePacketLengthMode;
    using RF69::setCrcFiltering;
    using RF69::setRxBandwidth;
    using RF69::fixedPacketLengthMode;
    using RF69::setFrequency;
    using RF69::setFrequencyDeviation;
    using RF69::getFrequencyDeviation;
    using RF69::setBitRate;

    RFQRF69(Module *module) : RadioLibWrapper(module, "RF69") {}

    int16_t begin() override {
      int16_t state = RadioLibWrapper::begin();

      if (state != RADIOLIB_ERR_NONE) return state;

      // Remove whitening
      state |= _mod->SPIsetRegValue(RADIOLIB_RF69_REG_PACKET_CONFIG_1, RADIOLIB_RF69_DC_FREE_NONE, 6, 5);

      return state;
    }

    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) override {
      if (size == 0) {
        RFQUACK_LOG_TRACE(F("Preamble and SyncWord disabled."))
        return RF69::disableSyncWordFiltering();
      }

      // Call to base method.
      int16_t state = RF69::setSyncWord(bytes, size, 0);
      if (state == RADIOLIB_ERR_NONE) {
        memcpy(_syncWords, bytes, size);
      }
      return state;
    }

    int16_t getSyncWord(uint8_t *bytes, pb_size_t *size) override {
      if (RF69::_promiscuous) {
        // No sync words when in promiscuous mode.
        *size = 0;
        return RADIOLIB_ERR_INVALID_SYNC_WORD;
      } else {
        *size = RF69::_syncWordLength;
        memcpy(bytes, _syncWords, *size);
      }
      return RADIOLIB_ERR_NONE;
    }

    int16_t getBitRate(float *br) override {
      *br = RF69::_br;
      return RADIOLIB_ERR_NONE;
    }

    int16_t receiveMode() override {
      if (_mode == rfquack_Mode_RX)
        return RADIOLIB_ERR_NONE;

      return RF69::startReceive();
    }

    bool isIncomingDataAvailable() override {
      // Makes sense only if in RX mode.
      if (_mode != rfquack_Mode_RX) {
        return false;
      }

      // GDO0 is asserted if:
      // "CRC is OK (but recall that we have promiscuous mode)"
      return digitalRead(_mod->getIrq());
    }

    int16_t readData(uint8_t *data, size_t len) override {
      // Exit if not in RX mode.
      if (_mode != rfquack_Mode_RX) {
        RFQUACK_LOG_TRACE(F("Trying to readData without being in RX mode."))
        return ERR_WRONG_MODE;
      }

      return RF69::readData(data, len);
    }

    int16_t setModulation(rfquack_Modulation modulation) override {
      if (modulation == rfquack_Modulation_OOK) {
        return RF69::setOOK(true);
      }
      if (modulation == rfquack_Modulation_FSK2) {
        return RF69::setOOK(false);
      }
      return RADIOLIB_ERR_UNSUPPORTED_ENCODING;
    }

    int16_t getModulation(char *modulation) {
      if (!RF69::_ook) {
        strcpy(modulation, "FSK2");
      } else {
        strcpy(modulation, "OOK");
      }

      return RADIOLIB_ERR_NONE;
    }

    // TODO implement jamMode for RFM69
    int16_t jamMode() {
      Log.error(F("TODO: jamMode was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    // TODO implement setAutoAck for RFM69
    int16_t setAutoAck(bool autoAckOn) override {
      Log.error(F("TODO setAutoAck was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    // TODO implement isCarrierDetected for RFM69
    int16_t isCarrierDetected(bool *isDetected) override {
      Log.error(F("TODO isCarrierDetected was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    float getRSSI(float *rssi) override {
      *rssi = RF69::getRSSI();

      return *rssi;
    }

    void writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value, uint8_t msb, uint8_t lsb) override {
      _mod->SPIsetRegValue((uint8_t) reg, (uint8_t) value, msb, lsb, 0);
    }

    void removeInterrupts() override {
      detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
    }

private:
    byte _syncWords[RADIOLIB_RF69_DEFAULT_SW_LEN] = RADIOLIB_RF69_DEFAULT_SW;
    // TODO set cached variables here
};

#endif //RFQUACK_PROJECT_RFQRF69_H
