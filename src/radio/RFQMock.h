#ifndef RFQUACK_PROJECT_RFQMOCK_H
#define RFQUACK_PROJECT_RFQMOCK_H

#include "RadioLibWrapper.h"


class RFQMock {
public:
    RFQMock() {
      _rxQueue = new Queue(sizeof(rfquack_Packet), RFQUACK_RADIO_RX_QUEUE_LEN, FIFO, true);
    }

    void setWhichRadio(WhichRadio whichRadio) {
      _whichRadio = whichRadio;
    }

    int16_t begin() {
      return ERR_NONE;
    }

    int16_t standbyMode() {
      return ERR_NONE;
    }

    int16_t receiveMode() {
      return ERR_NONE;
    }

    int16_t transmitMode() {
      return ERR_NONE;
    }

    int16_t setMode(rfquack_Mode mode) {
      return ERR_NONE;
    }

    int16_t transmit(uint8_t *data, size_t len) {
      return ERR_NONE;
    }

    int16_t transmit(rfquack_Packet *pkt) {
      return ERR_NONE;
    }

    int16_t readData(uint8_t *data, size_t len) {
      return ERR_NONE;
    }

    void rxLoop() {
    }

    rfquack_register_value_t readRegister(rfquack_register_address_t reg) {
      rfquack_register_address_t out = 0;
      return out;
    }

    void writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value) {
    }

    int16_t setPreambleLength(uint32_t size) {
      return ERR_NONE;
    }

    int16_t setFrequency(float carrierFreq) {
      return ERR_NONE;
    }

    int16_t setOutputPower(uint32_t txPower) {
      return ERR_NONE;
    }

    int16_t setSyncWord(uint8_t *bytes, pb_size_t size) {
      return ERR_NONE;
    }

    int16_t fixedPacketLengthMode(uint8_t len) {
      return ERR_NONE;
    }

    int16_t variablePacketLengthMode(uint8_t len) {
      return ERR_NONE;
    }

    int16_t setPromiscuousMode(bool isPromiscuous) {
      return ERR_NONE;
    }

    rfquack_Stats &getRfquackStats() const {
      rfquack_Stats stats;
      return stats;
    }

    Queue *getRxQueue() const {
      return _rxQueue;
    }

private:
    Queue *_rxQueue;
    WhichRadio _whichRadio;
};

#endif //RFQUACK_PROJECT_RFQMOCK_H
