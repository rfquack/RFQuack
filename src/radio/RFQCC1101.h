#ifndef RFQUACK_PROJECT_RFQCC1101_H
#define RFQUACK_PROJECT_RFQCC1101_H


#include "RadioLibWrapper.h"
#include "rfquack_logging.h"

class RFQCC1101 : public RadioLibWrapper<CC1101> {
public:
    using CC1101::setPreambleLength;
    using CC1101::setOutputPower;
    using CC1101::setPromiscuousMode;
    using CC1101::variablePacketLengthMode;
    using CC1101::fixedPacketLengthMode;

    RFQCC1101(Module *module) : RadioLibWrapper(module) {}

    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) override {
      if (size == 0) {
        // Warning: as side effect this also disables preamble generation / detection.
        // CC1101 Datasheet states: "It is not possible to only insert preamble or
        // only insert a sync word"
        RFQUACK_LOG_TRACE("Preamble and SyncWord disabled.")
        return CC1101::disableSyncWordFiltering();
      }

      // Set syncWord.
      return CC1101::setSyncWord(bytes, size);
    }

    int16_t setFrequency(float carrierFreq) override {
      //This command as side effect sets mode to Standby
      _mode = RFQRADIO_MODE_STANDBY;
      return CC1101::setFrequency(carrierFreq);
    }

    void removeInterrupts() override {
      detachInterrupt(digitalPinToInterrupt(_mod->getInt0()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getInt0()), func, (void*)(&_flag), FALLING);
    }
};

#endif //RFQUACK_PROJECT_RFQCC1101_H
