#ifndef RFQUACK_PROJECT_RFQNRF24_H
#define RFQUACK_PROJECT_RFQNRF24_H

#include "RadioLibWrapper.h"
#include "rfquack_logging.h"

class RFQnRF24 : public RadioLibWrapper<nRF24> {
public:
    using nRF24::setCrcFiltering;
    using nRF24::setFrequencyDeviation;

    RFQnRF24(Module *module) : RadioLibWrapper(module) {}

    virtual int16_t transmitMode() override {
      // Call to base method.
      int16_t state = RadioLibWrapper::transmitMode();
      if (state != ERR_NONE) {
        return state;
      }

      // Set up transmitting pipe.
      // This will automagically sets pipe 1 (?) for TX and pipe 0 for ACKs RX.
      RFQUACK_LOG_TRACE(F("Setting up TX pipe."))
      return nRF24::setTransmitPipe(_addr);
    }

    bool isTxChannelFree() override {
      // Check if max number of retransmits has occurred.
      if (nRF24::getStatus(NRF24_MAX_RT)) {
        nRF24::standby();
        nRF24::clearIRQ();
        RFQUACK_LOG_TRACE(F("No ACK received from previous TX. Abort TX."))
        return true;
      }

      // Call to base method.
      return RadioLibWrapper::isTxChannelFree();
    }

    virtual int16_t receiveMode() override {
      // Set up receiving pipe.
      int16_t state = nRF24::setReceivePipe(0, _addr);
      if (state != ERR_NONE)
        return state;

      // Call to base method.
      return RadioLibWrapper::receiveMode();
    }

    // NOTE: nRF24 does not have a "setSyncword()" method since it's called "address" and is set
    // per pipe during TX/RX.
    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) override {
      // Note: this command will bring radio back to standby mode.
      _mode = rfquack_Mode_IDLE;

      // First try to set addr width.
      int16_t result = nRF24::setAddressWidth(size);
      if (result != ERR_NONE) {
        return result;
      }

      // If addr width is valid store it.
      memcpy(_addr, bytes, size);

      return ERR_NONE;
    }

    // Wrap base method since it changes radio mode.
    int16_t setOutputPower(uint32_t txPower) override {
      _mode = rfquack_Mode_IDLE;
      return nRF24::setOutputPower(txPower);
    }

    // Wrap base method since it changes radio mode.
    int16_t setFrequency(float carrierFreq) override {
      _mode = rfquack_Mode_IDLE;
      return nRF24::setFrequency(carrierFreq);
    }

    int16_t setPromiscuousMode(bool isEnabled) override {
      // Set syncWord as preamble's tail.
      // TODO: syncWord should be changed back when exiting
      byte sync[2] = {0xAA, 0xAA};
      int16_t state = setSyncWord(sync, 2);
      if (state != ERR_NONE) {
        return state;
      }

      // Disable auto ACK
      state = nRF24::setAutoAck(!isEnabled);
      if (state != ERR_NONE) {
        return state;
      }

      // Disable CRC
      state = nRF24::setCrcFiltering(!isEnabled);
      if (state != ERR_NONE) {
        return state;
      }

      // Enable receive mode
      return RadioLibWrapper::receiveMode();
    }

    void removeInterrupts() override {
      nRF24::clearIRQ();
      detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
    }

private:
    byte _addr[5] = {0x01, 0x23, 0x45, 0x67, 0x89}; // Cannot be > 5 bytes. Default len is 5.
};


#endif //RFQUACK_PROJECT_RFQNRF24_H