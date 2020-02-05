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
    using CC1101::setBitRate;
    using CC1101::setRxBandwidth;
    using CC1101::setFrequencyDeviation;

    RFQCC1101(Module *module) : RadioLibWrapper(module) {}

    void printRegisters() {
      Serial.println("DUMP: ");
      // From 0x00 to 0x28
      for (uint8_t i = 0; i < 0x2E; i++) {
        Serial.print(i, HEX);
        Serial.print(": ");
        Serial.println(SPIreadRegister(i), HEX);
      }

      uint8_t data[2];
      SPIreadRegisterBurst(CC1101_REG_PATABLE, 2, data);

      Serial.print("PA0");
      Serial.print(": ");
      Serial.println(data[0], HEX);


      Serial.print("PA1");
      Serial.print(": ");
      Serial.println(data[1], HEX);
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
      if (_mode == rfquack_Mode_RX) {
        return ERR_NONE;
      }

      // Set mode to standby (needed to flush fifo)
      standby();

      // Flush RX FIFO
      SPIsendCommand(CC1101_CMD_FLUSH_RX);

      // Stay in RX mode after a packet is received.
      uint8_t state = SPIsetRegValue(CC1101_REG_MCSM1, CC1101_RXOFF_RX, 3, 2);

      // Do not append status byte
      state |= SPIsetRegValue(CC1101_REG_PKTCTRL1, CC1101_APPEND_STATUS_OFF, 2, 2);

      // Remove whitening
      state |= SPIsetRegValue(CC1101_REG_PKTCTRL0, CC1101_WHITE_DATA_OFF, 6, 6);

      // Set THR to 4 bytes
      state |= SPIsetRegValue(CC1101_REG_FIFOTHR, 0, 3, 0);

      // set GDO0 mapping. Asserted when RX FIFO > 4 bytes.
      state |= SPIsetRegValue(CC1101_REG_IOCFG0, CC1101_GDOX_RX_FIFO_FULL_OR_PKT_END);

      if (state != ERR_NONE)
        return state;

      // Issue receive mode.
      SPIsendCommand(CC1101_CMD_RX);
      _mode = rfquack_Mode_RX;

      return ERR_NONE;
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

      uint8_t bytesInFIFO = SPIgetRegValue(CC1101_REG_RXBYTES, 6, 0);
      uint8_t readBytes = 0;

      unsigned long lastPop = millis();

      // Keep reading from FIFO until we get all the message.
      while (readBytes < len) {
        if (millis() - lastPop > 10) {
          RFQUACK_LOG_TRACE(F("No data for more than 10mS. Stop here."));
          break;
        }

        if (bytesInFIFO == 0) {
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
        lastPop = millis();

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
      _mode = rfquack_Mode_IDLE;
      return CC1101::setFrequency(carrierFreq);
    }

    int16_t setModulation(rfquack_Modulation modulation) override {
      if (modulation == rfquack_Modulation_OOK) {
        return CC1101::setOOK(true);
      }
      if (modulation == rfquack_Modulation_FSK2) {
        return CC1101::setOOK(true);
      }
      return ERR_UNSUPPORTED_ENCODING;
    }

    void removeInterrupts() override {
      detachInterrupt(digitalPinToInterrupt(_mod->getIrq()));
    }

    void setInterruptAction(void (*func)(void *)) override {
      attachInterruptArg(digitalPinToInterrupt(_mod->getIrq()), func, (void *) (&_flag), FALLING);
    }


};

#endif //RFQUACK_PROJECT_RFQCC1101_H
