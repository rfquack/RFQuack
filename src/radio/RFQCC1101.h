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
    using CC1101::setCrcFiltering;
    using CC1101::printRegisters;

    RFQCC1101(Module *module) : RadioLibWrapper(module) {}

    int16_t ookMode() {
      // Set ASK modulation
      uint8_t state = SPIsetRegValue(CC1101_REG_MDMCFG2, CC1101_MOD_FORMAT_ASK_OOK, 6, 4);

      // Set PA_TABLE to be used when transmitting "1"
      state |= SPIsetRegValue(CC1101_REG_FREND0, 1, 2, 0);

      // Set PA_TABLE values:
      // PA_TABLE[0] power to be used when transmitting a 0  (no power)
      // PA_TABLE[0] power to be used when transmitting a 1  (full power)
      byte paValues[2] = {0x00, 0x60};
      SPIwriteRegisterBurst(CC1101_REG_PATABLE, paValues, 2);

      return (state);
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
      return CC1101::setSyncWord(bytes, size);
    }

    int16_t receiveMode() override {
      if (_mode == RFQRADIO_MODE_RX){
        return ERR_NONE;
      }

      // Set mode to standby (needed to flush fifo)
      standby();

      // Flush RX FIFO
      SPIsendCommand(CC1101_CMD_FLUSH_RX);

      // Stay in RX mode after a packet is received.
      uint8_t state = SPIsetRegValue(CC1101_REG_MCSM1, CC1101_RXOFF_RX, 3, 2);

      // Do not append status byte
      state |= SPIsetRegValue(CC1101_REG_PKTCTRL1, CC1101_APPEND_STATUS_OFF, 2,2);

      // set GDO0 mapping
      state |= SPIsetRegValue(CC1101_REG_IOCFG0, CC1101_GDOX_RX_FIFO_FULL_OR_PKT_END);

      if (state != ERR_NONE)
        return state;

      // Issue receive mode.
      SPIsendCommand(CC1101_CMD_RX);
      _mode = RFQRADIO_MODE_RX;

      return ERR_NONE;
    }

    bool isIncomingDataAvailable() override {
      // Makes sense only if in RX mode.
      if (_mode != RFQRADIO_MODE_RX) {
        return false;
      }

      // GDO0 is asserted if:
      // "RX FIFO is filled at or above the RX FIFO threshold or the end of packet is reached"
      return digitalRead(_mod->getInt0());
    }

    int16_t readData(uint8_t *data, size_t len) override {
      // Exit if not in RX mode.
      if (_mode != RFQRADIO_MODE_RX) {
        RFQUACK_LOG_TRACE(F("Trying to readData without being in RX mode."))
        return ERR_WRONG_MODE;
      }

      uint8_t bytesInFIFO = SPIgetRegValue(CC1101_REG_RXBYTES, 6, 0);
      uint8_t readBytes = 0;

      unsigned long startTime = millis();

      // Keep reading from FIFO until we get all the message.
      while (readBytes < len) {
        if (millis() - startTime > 100){
          RFQUACK_LOG_TRACE(F("Waiting for more than 100mS. Stop here."));
          break;
        }

        if (bytesInFIFO==0) {
          RFQUACK_LOG_TRACE("[RX FIFO] Waiting for data. %d/%d", readBytes, len);
          delay(1);
          bytesInFIFO = SPIgetRegValue(CC1101_REG_RXBYTES, 6, 0);
          continue;
        }

        // Read the minimum between len and bytesInFifo
        uint8_t bytesToRead = (bytesInFIFO > len) ? len : bytesInFIFO;
        RFQUACK_LOG_TRACE("Reading %d bytes from FIFO. %d/%d", bytesToRead, readBytes, len)

        // Read data from FIFO
        SPIreadRegisterBurst(CC1101_REG_FIFO, bytesToRead, (data + readBytes));
        readBytes += bytesToRead;

        // Get how many bytes are left in FIFO.
        bytesInFIFO = SPIgetRegValue(CC1101_REG_RXBYTES, 6, 0);
        RFQUACK_LOG_TRACE("%d bytes left in FIFO", bytesInFIFO)
      }

      // add terminating null
      data[len] = 0;

      return ERR_NONE;
    }

    int16_t setFrequency(float carrierFreq) override {
      // This command, as side effect, sets mode to Standby
      _mode = RFQRADIO_MODE_STANDBY;
      return CC1101::setFrequency(carrierFreq);
    }

    void removeInterrupts() override {
      detachInterrupt(digitalPinToInterrupt(_mod->getInt0()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getInt0()), func, (void *) (&_flag), FALLING);
    }
};

#endif //RFQUACK_PROJECT_RFQCC1101_H
