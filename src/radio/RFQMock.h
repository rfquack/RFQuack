#ifndef RFQUACK_PROJECT_RFQMOCK_H
#define RFQUACK_PROJECT_RFQMOCK_H

#include "RadioLibWrapper.h"


class RFQMock {
public:
    RFQMock() {
      _rxQueue = new Queue(sizeof(rfquack_Packet), RFQUACK_RADIO_RX_QUEUE_LEN, FIFO, true);
    }

    void setWhichRadio(rfquack_WhichRadio whichRadio) {
      _whichRadio = whichRadio;
    }

    int16_t begin() {
      return ERR_NONE;
    }

    int16_t standbyMode() {
      _mode = rfquack_Mode_IDLE;
      RFQUACK_LOG_TRACE("Idle mode entered");
      return ERR_NONE;
    }

    int16_t receiveMode() {
      _mode = rfquack_Mode_RX;
      RFQUACK_LOG_TRACE("Rx mode entered");
      return ERR_NONE;
    }

    int16_t transmitMode() {
      _mode = rfquack_Mode_TX;
      RFQUACK_LOG_TRACE("Tx mode entered");
      return ERR_NONE;
    }

    int16_t setMode(rfquack_Mode mode) {
      switch (mode) {
        case rfquack_Mode_IDLE:
          return standbyMode();
        case rfquack_Mode_RX:
          return receiveMode();
        case rfquack_Mode_TX:
          return transmitMode();
        default:
          return ERR_UNKNOWN;
      }
    }

    rfquack_Mode getMode() {
      return _mode;
    }


    int16_t transmit(uint8_t *data, size_t len) {
      return ERR_NONE;
    }

    int16_t transmit(rfquack_Packet *pkt) {
      return transmit(pkt->data.bytes, pkt->data.size);
    }

    int16_t readData(uint8_t *data, size_t len) {
      return ERR_NONE;
    }

    unsigned long lastRX = 0;

    void rxLoop() {
      // Check if there's pending data on radio's RX FIFO.
      if (_mode == rfquack_Mode_RX && (millis() - lastRX) > 5000) {
        lastRX = millis();

        rfquack_Packet pkt = rfquack_Packet_init_default;

        char str[] = "HELLO WORLD";
        int len = strlen(str); // Text without null terminator
        memcpy((uint8_t *) pkt.data.bytes, str, len);
        pkt.data.size = len;
        RFQUACK_LOG_TRACE("Received (fake) packet of len %d !", len)

        // Put radio back in receiveMode.
        receiveMode();

        // Filter packet
        if (modulesDispatcher.onPacketReceived(pkt, _whichRadio)) {
          // If packet passed filtering put it in rxQueue.
          enqueuePacket(&pkt);
        }
      }
    }

    rfquack_register_value_t readRegister(rfquack_register_address_t reg) {
      rfquack_register_address_t out = 57;
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


    Queue *getRxQueue() const {
      return _rxQueue;
    }

    int16_t setCrcFiltering(bool isPromiscuous) {
      return true;
    }

    int16_t setModulation(rfquack_Modulation modulation) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setRxBandwidth(float rxBw) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setBitRate(float br) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    int16_t setFrequencyDeviation(float freqDev) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }


private:
    rfquack_Mode _mode = rfquack_Mode_IDLE;
    Queue *_rxQueue;
    rfquack_WhichRadio _whichRadio;

    void enqueuePacket(rfquack_Packet *packet) {
      if (packet->data.size > sizeof(rfquack_Packet)) {
        Log.error(F("Packet payload is grater than container."));
        return;
      }

      if (_rxQueue->isFull()) {
        Log.error(F("rxQueue is full"));
        return;
      }

      _rxQueue->push(packet);
      RFQUACK_LOG_TRACE(F("Packet put in rxQueue, size %d bytes"), packet->data.size);
    }
};

#endif //RFQUACK_PROJECT_RFQMOCK_H
