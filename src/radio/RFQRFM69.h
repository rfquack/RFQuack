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

    RFQRF69(Module *module) : RadioLibWrapper(module, "RF69") {}

    int16_t begin() override {
      int16_t state = RadioLibWrapper::begin();

      if (state != ERR_NONE) return state;

      // Remove whitening
      state |= _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_DC_FREE_NONE, 6, 5);

      return state;
    }

    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) override {
      if (size == 0) {
        // Warning: as side effect this also disables preamble generation / detection.
        // RF69 Datasheet states: "It is not possible to only insert preamble or
        // only insert a sync word"
        RFQUACK_LOG_TRACE(F("Preamble and SyncWord disabled."))
        return RF69::disableSyncWordFiltering();
      }

      // Call to base method.
      return RF69::setSyncWord(bytes, size);
    }

    int16_t receiveMode() override {
      if (_mode == rfquack_Mode_RX)
        return ERR_NONE;

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

    int16_t setFrequency(float carrierFreq) override {
      // This command, as side effect, sets mode to Standby
      _mode = rfquack_Mode_IDLE;
      return RF69::setFrequency(carrierFreq);
    }

    int16_t getFrequency(float &carrierFreq) override {
      carrierFreq = RF69::_freq;
      return ERR_NONE;
    }

    int16_t setModulation(rfquack_Modulation modulation) override {
      if (modulation == rfquack_Modulation_OOK) {
        return RF69::setOOK(true);
      }
      if (modulation == rfquack_Modulation_FSK2) {
        return RF69::setOOK(false);
      }
      return ERR_UNSUPPORTED_ENCODING;
    }

    int16_t getBitRate(float &br) override {
      br = RF69::_br;
      return ERR_NONE;
    }

    int16_t jamMode() {
      Log.error(F("TODO: jamMode was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setPreambleLength(uint32_t size) {
      Log.error(F("TODO setPreambleLength was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setFrequencyDeviation(float freqDev) {
      Log.error(F("TODO setFrequencyDeviation was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }
    
    int16_t getFrequencyDeviation(float &freqDev) {
      Log.error(F("TODO getFrequencyDeviation was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setRxBandwidth(float rxBw) {
      Log.error(F("TODO setRxBandwidth was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setOutputPower(uint32_t txPower) {
      Log.error(F("TODO: setOutputPower was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t fixedPacketLengthMode(uint8_t len) {
      Log.error(F("TODO fixedPacketLengthMode was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t variablePacketLengthMode(uint8_t len) {
      Log.error(F("TODO variablePacketLengthMode was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setPromiscuousMode(bool isPromiscuous) {
      Log.error(F("TODO setPromiscuousMode was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setCrcFiltering(bool crcOn) {
      Log.error(F("TODO setCrcFiltering was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setAutoAck(bool autoAckOn) {
      Log.error(F("TODO setAutoAck was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t isCarrierDetected(bool &isDetected) {
      Log.error(F("TODO isCarrierDetected was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    float getRSSI(float &rssi) {
      Log.error(F("TODO getRSSI was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setModulation(rfquack_Modulation modulation) {
      Log.error(F("TODO setModulation was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }
    
    int16_t getModulation(char *modulation) {
      Log.error(F("TODO getModulation was not implemented."));
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    void
    writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value, uint8_t msb, uint8_t lsb) override {
      _mod->SPIsetRegValue((uint8_t) reg, (uint8_t) value, msb, lsb, 0);
    }

    void removeInterrupts() override {
      detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
    }
};

#endif //RFQUACK_PROJECT_RFQRF69_H
