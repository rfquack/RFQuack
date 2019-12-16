#ifndef RFQUACK_PROJECT_RADIOLIBWRAPPER_H
#define RFQUACK_PROJECT_RADIOLIBWRAPPER_H

// More error codes
#define ERR_COMMAND_NOT_IMPLEMENTED -590
#define ERR_WRONG_MODE              -591

// Driver status
#define RFQRADIO_MODE_STANDBY 0
#define RFQRADIO_MODE_TX 1
#define RFQRADIO_MODE_RX 2

// Enable super powers :)
#define RADIOLIB_GODMODE

/* Dirty trick to prevent RadioLib's MQTT class to be included,
 * It clashes with RFQuack's one.
*/
#define _RADIOLIB_MQTT_H

#include <RadioLib.h>
#include "../defaults/radio.h"
#include "../rfquack.pb.h"

#ifndef RFQUACK_RADIO_RX_QUEUE_LEN
#define RFQUACK_RADIO_RX_QUEUE_LEN RFQUACK_RADIO_RX_QUEUE_LEN_DEFAULT
#endif

typedef uint16_t rfquack_register_address_t;
typedef uint8_t rfquack_register_value_t;

class IRQ {
public:
    /**
     * Used from IRQ to set flag and from wrapper to clean it.
     * @param flagValue
     */
    void setFlag(bool flagValue) {
      _flag = flagValue;
    }

protected:
    volatile bool _flag;
};

/**
 * Interrupt routine used to set flag.
 * @param flag
 */
void IRAM_ATTR radioInterrupt(void *flag) {
  *((bool *) (flag)) = true;
}

template<typename T>

class RadioLibWrapper : protected IRQ, protected T {
public:
    using T::begin;
    using T::getPacketLength;

    RadioLibWrapper(Module *module) : T(module) {
      _rxQueue = new Queue(sizeof(rfquack_Packet), RFQUACK_RADIO_RX_QUEUE_LEN, FIFO, true);
    }

    /**
     * Puts radio in standby mode.
     * @return
     */
    virtual int16_t standbyMode() {
      _mode = RFQRADIO_MODE_STANDBY;
      removeInterrupts();
      RFQUACK_LOG_TRACE(F("Entering STANDBY mode."))
      return T::standby();
    }

    /**
     * Puts radio in receive mode and enables RX interrupt.
     * @return
     */
    virtual int16_t receiveMode() {
      if (_mode != RFQRADIO_MODE_RX) {

        // Set mode to RX.
        _mode = RFQRADIO_MODE_RX;
        RFQUACK_LOG_TRACE(F("Entering RX mode."))

        // Start async RX
        int16_t state = T::startReceive();
        if (state != ERR_NONE)
          return state;

        // Register an interrupt routine to set flag when radio receives something.
        setFlag(false);
        setInterruptAction(radioInterrupt);
      }else{
        RFQUACK_LOG_TRACE(F("Already in RX mode. RadioLibWrapper"))
      }

      return ERR_NONE;
    }

    /**
     * Puts radio in transmit mode.
     * @return
     */
    virtual int16_t transmitMode() {
      // If we are not in TX mode, change mode and set flag (means channel is free)
      if (_mode != RFQRADIO_MODE_TX) {
        _mode = RFQRADIO_MODE_TX;
        removeInterrupts();

        // NOTE: In TX mode the flag is present (true) if TX channel is free to use.
        setFlag(true);
        RFQUACK_LOG_TRACE(F("Entering TX mode."))
      }

      return ERR_NONE;
    }

    /**
     * Sets radio mode (RX, TX, IDLE)
     * @param mode RFQuack mode.
     * @return
     */
    int16_t setMode(rfquack_Mode mode) {
      switch (mode) {
        case rfquack_Mode_IDLE:
          return standbyMode();
        case rfquack_Mode_REPEAT:
        case rfquack_Mode_RX:
          return receiveMode();
        case rfquack_Mode_TX:
          return transmitMode();
        default:
          return ERR_UNKNOWN;
      }
    }

    /**
     * Sends packet to air.
     * @param data buffer to send.
     * @param len buffer length.
     * @return
     */
    virtual int16_t transmit(uint8_t *data, size_t len) {
      // Exit if radio is not in TX mode.
      if (_mode != RFQRADIO_MODE_TX) {
        RFQUACK_LOG_TRACE(F("In order to transmit you must be in TX mode."))
        return ERR_WRONG_MODE;
      }

      RFQUACK_LOG_TRACE(F("Request to transmit, checking if channel is free."))
      uint32_t start = micros();

      // Wait for TX channel to be free (or timeout)
      while (!isTxChannelFree()) {
        // Check if timeout has reached.
        if (micros() - start >= 60000) {
          T::standby();
          removeInterrupts();
          RFQUACK_LOG_TRACE(F("We have been waiting too long for channel to get free."))
          break;
        }
        delay(2);
      }

      RFQUACK_LOG_TRACE(F("Channel is free, go ahead transmitting"))

      // Remove flag (false), in TX mode this means "TX channel is busy"
      RFQUACK_LOG_TRACE(F("Removing flag"))
      setFlag(false);

      // Start async TX.
      int16_t state = T::startTransmit(data, len, 0); // 3rd param is not used.
      if (state != ERR_NONE)
        return state;

      // Register an interrupt, we'll use it to know when TX is over and channel gets free.
      setInterruptAction(radioInterrupt);

      return ERR_NONE;
    }

    virtual bool isTxChannelFree() {
      // TX Channel is free when "flag" is put back, this means that TX is over.
      return _flag;
    }

    /**
     * Transmit a RFQuack Packet.
     * @param pkt packet to transmit
     * @return
     */
    int16_t transmit(rfquack_Packet *pkt) {
      if (pkt->has_repeat && pkt->repeat == 0) {
        RFQUACK_LOG_TRACE (F("Zero packet repeat: no transmission"))
        return ERR_NONE;
      }

      uint32_t repeat = 1;
      uint32_t correct = 0;

      if (pkt->has_repeat) {
        repeat = pkt->repeat;
      }

      for (uint32_t i = 0; i < repeat; i++) {
        int16_t result = transmit((uint8_t *) (pkt->data.bytes), pkt->data.size);
        RFQUACK_LOG_TRACE("Packet trasmitted, resultCode=%d", result)

        if (result == ERR_NONE) {
          correct++;
        }

        if (pkt->has_delayMs)
          delay(pkt->delayMs);
      }

      RFQUACK_LOG_TRACE("%d/%d packets transmitted", repeat, correct)
      return ERR_NONE;
    }

    /**
     * True whenever there's data available on radio's RX FIFO.
     * @return
     */
    virtual bool isIncomingDataAvailable() {
      // Flag makes sense only if in RX mode.
      if (_mode != RFQRADIO_MODE_RX) {
        return false;
      }

      return _flag;
    }

    /**
     * Consumes data from radio RX FIFO.
     * @param data
     * @param len
     * @return
     */
    virtual int16_t readData(uint8_t *data, size_t len) {
      // Exit if not in RX mode.
      if (_mode != RFQRADIO_MODE_RX) {
        RFQUACK_LOG_TRACE(F("Trying to readData without being in RX mode."))
        return ERR_WRONG_MODE;
      }

      // Let's assume readData() call rate is so high that there's
      // always at most a packet to read from radio.
      setFlag(false);

      // Shame on RadioLib, after readData driver resets interrupts and goes in STANDBY.
      _mode = RFQRADIO_MODE_STANDBY;

      // Read data from fifo.
      return T::readData(data, len);
    }

    /**
     * Main receive loop; reads any data from the RX FIFO and push it to the RX queue.
     */
    void rxLoop() {
      // Check if there's pending data on radio's RX FIFO.
      if (isIncomingDataAvailable()) {
        rfquack_Packet pkt;

        // Pop packet from RX FIFO.
        uint8_t packetLen = getPacketLength(false);
        int16_t result = readData((uint8_t *) pkt.data.bytes, packetLen);
        RFQUACK_LOG_TRACE("Recieved packet, resultCode=%d", result)

        // Put radio back in receiveMode.
        receiveMode();

        // Update stats
        if (result == ERR_NONE) {
          _rfquackStats.rx_packets++;
        } else {
          _rfquackStats.rx_failures++;
        }

        // Fill missing data
        pkt.data.size = packetLen;
        pkt.millis = millis();
        pkt.has_millis = true;

        // Filter packet
        if (rfquack_packet_filter(&pkt)) {
          // If packet passed filtering put it in rxQueue.
          enqueuePacket(&pkt);
        }
      }
    }

    /**
     * Reads a radio's internal register.
     * @param reg register to read.
     * @return
     */
    virtual rfquack_register_value_t readRegister(rfquack_register_address_t reg) {
      return T::_mod->SPIreadRegister((uint8_t) reg);
    }

    /**
     * Writes to a radio's internal register.
     * @param reg register to write.
     * @param value register's value.
     */
    virtual void writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value) {
      T::_mod->SPIwriteRegister((uint8_t) reg, (uint8_t) value);
    }

    /**
     * Sets transmitted / received preamble length.
     * @param size size
     * @return
     */
    virtual int16_t setPreambleLength(uint32_t size) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Sents radio frequency.
     * @param carrierFreq
     * @return
     */
    virtual int16_t setFrequency(float carrierFreq) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Sets radio output power.
     * @param txPower
     * @return
     */
    virtual int16_t setOutputPower(uint32_t txPower) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Sets radio syncWord and it's size.
     * @param bytes pointer to syncword.
     * @param size syncword size.
     * @return
     */
    virtual int16_t setSyncWord(uint8_t *bytes, pb_size_t size) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Puts radio in fixed packet length mode.
     * @param len packet's size.
     * @return
     */
    virtual int16_t fixedPacketLengthMode(uint8_t len) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Puts radio in fixed packet length mode.
     * @param len maximum packet size.
     * @return
     */
    virtual int16_t variablePacketLengthMode(uint8_t len) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Puts radio in promiscuous mode.
     * @param isPromiscuous
     * @return
     */
    virtual int16_t setPromiscuousMode(bool isPromiscuous) {
      return ERR_COMMAND_NOT_IMPLEMENTED;
    }

    /**
     * Retrieve RFQuack stats for this radio.
     * @return
     */
    const rfquack_Stats &getRfquackStats() const {
      return _rfquackStats;
    }

    Queue *getRxQueue() const {
      return _rxQueue;
    }

    /**
     * Sets callback on interrupt.
     * @param func
     */
    virtual void setInterruptAction(void (*func)(void *)) = 0;

    virtual void removeInterrupts() = 0;

protected:
    uint8_t _mode = RFQRADIO_MODE_STANDBY; // RFQRADIO_MODE_[STANDBY|RX|TX]
private:
    rfquack_Stats _rfquackStats;
    Queue *_rxQueue;

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
      RFQUACK_LOG_TRACE("Packet put in rxQueue, size %d bytes", packet->data.size);
    }
};


#endif //RFQUACK_PROJECT_RADIOLIBWRAPPER_H
