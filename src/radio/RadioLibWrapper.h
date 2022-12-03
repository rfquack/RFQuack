#ifndef RFQUACK_PROJECT_RADIOLIBWRAPPER_H
#define RFQUACK_PROJECT_RADIOLIBWRAPPER_H

// More error codes
#define ERR_COMMAND_NOT_IMPLEMENTED -590
#define ERR_WRONG_MODE -591

// Enable super powers :)
#define RADIOLIB_LOW_LEVEL
#define RADIOLIB_GODMODE
#define RADIOLIB_DEBUG

/* Dirty trick to prevent RadioLib's MQTT class to be included,
 * It clashes with RFQuack's one.
 */
#define _RADIOLIB_MQTT_H

#include <RadioLib.h>
#include <cppQueue.h>
#include "../defaults/radio.h"
#include "../modules/ModulesDispatcher.h"

extern ModulesDispatcher modulesDispatcher;

typedef uint16_t rfquack_register_address_t;
typedef uint8_t rfquack_register_value_t;

extern QueueHandle_t queue; // Queue of incoming packets

class IRQ
{
public:
  /* RX flag */
  void disableReceivedFlag(void) { _receivedFlag = false; }
  void setReceivedFlag(void)
  {
    if (!_enableRxInterrupt)
    {
      return;
    }
    _receivedFlag = true;
  }

  /* TX flag */
  void disableTransmittedFlag(void) { _transmittedFlag = false; }
  void setTransmittedFlag(void)
  {
    if (!_enableTxInterrupt)
    {
      return;
    }
    _transmittedFlag = true;
  }

  /* IRQ enable */
  void enableRxInterrupt(void) { _enableRxInterrupt = true; }
  void disableRxInterrupt(void) { _enableRxInterrupt = false; }
  void enableTxInterrupt(void) { _enableTxInterrupt = true; }
  void disableTxInterrupt(void) { _enableTxInterrupt = false; }

protected:
  volatile bool _enableRxInterrupt = false;
  volatile bool _enableTxInterrupt = false;
  volatile bool _receivedFlag = false;
  volatile bool _transmittedFlag = false;
  volatile bool _canTransmit = false;
};

void IRAM_ATTR radioInterrupt(void *flag)
{
  *((bool *)(flag)) = true;
}

template <typename T>

class RadioLibWrapper : protected T, protected IRQ
{
public:
  using T::getPacketLength;

  RadioLibWrapper(Module *module, const char *chipName) : T(module)
  {
    this->chipName = new char[strlen(chipName) + 1];
    strcpy(this->chipName, chipName);
  }

  virtual int16_t begin()
  {
    return T::begin();
  }

  /**
   * Sets if this instance is either RADIOA / RADIOB / RADIOC / ...
   *
   * @param whichRadio Which radio to use.
   */
  void setWhichRadio(rfquack_WhichRadio whichRadio)
  {
    _whichRadio = whichRadio;
  }

  /**
   * Puts radio in standby mode.
   *
   * @return \ref status_codes
   */
  virtual int16_t standbyMode()
  {
    _mode = rfquack_Mode_IDLE;

    RFQUACK_LOG_TRACE(F("Entering STANDBY mode."))

    return T::standby();
  }

  /**
   * Puts radio in receive mode and enables RX interrupt.
   *
   * @return \ref status_codes
   */
  virtual int16_t receiveMode()
  {
    _mode = rfquack_Mode_RX;

    RFQUACK_LOG_TRACE(F("Entering RX mode."))

    // Register an interrupt routine to set flag when radio receives something.
    setRxInterruptAction(radioInterrupt);
    disableReceivedFlag();
    enableRxInterrupt();

    // Start async RX
    int16_t state = T::startReceive();

    if (state != RADIOLIB_ERR_NONE)
      return state;
  }

  /**
   * Puts radio in transmit mode.
   *
   * @return \ref status_codes
   */
  virtual int16_t transmitMode()
  {
    // If we are not in TX mode, change mode and set flag (means channel is free)
    if (_mode != rfquack_Mode_TX)
    {
      _mode = rfquack_Mode_TX;

      RFQUACK_LOG_TRACE(F("Entering TX mode."))
    }

    _canTransmit = true;

    return RADIOLIB_ERR_NONE;
  }

  /**
   * Puts radio in jam mode (starts jamming).
   *
   * @return \ref status_codes
   */
  virtual int16_t jamMode()
  {
    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets radio mode (RX, TX, IDLE, JAM)
   *
   * @param mode RFQuack mode (\ref rfquack_Mode)
   *
   * @return \ref status_codes
   */
  int16_t setMode(rfquack_Mode mode)
  {
    RFQUACK_LOG_TRACE(F("setMode <- %d"), mode);

    switch (mode)
    {
    case rfquack_Mode_IDLE:
      return standbyMode();
    case rfquack_Mode_RX:
      return receiveMode();
    case rfquack_Mode_TX:
      return transmitMode();
    case rfquack_Mode_JAM:
      return jamMode();
    default:
      return RADIOLIB_ERR_UNKNOWN;
    }
  }

  /**
   * @brief Get the radio mode (RX, TX, IDLE, JAM).
   *
   * @return \ref rfquack_Mode
   */
  rfquack_Mode getMode()
  {
    return _mode;
  }

  /**
   * Sends packet to air.
   *
   * @param data buffer to send
   * @param len buffer length
   *
   * @return \ref status_codes
   */
  virtual int16_t transmit(uint8_t *data, size_t len)
  {
    // Exit if radio is not in TX mode.
    if (_mode != rfquack_Mode_TX)
    {
      RFQUACK_LOG_TRACE(F("In order to transmit you must be in TX mode."))

      return ERR_WRONG_MODE;
    }

    RFQUACK_LOG_TRACE(F("Request to transmit, checking if channel is free."))
    uint32_t start = micros();

    // Wait for TX channel to be free (or timeout)
    while (!isTxChannelFree())
    {
      // Check if timeout has reached.
      if (micros() - start >= 60000)
      {
        T::standby();

        RFQUACK_LOG_TRACE(F("We have been waiting too long for channel to get free."))
        break;
      }

      delay(2);
    }

    RFQUACK_LOG_TRACE(F("Channel is free, go ahead transmitting"))

    // mark TX as busy
    _canTransmit = false;

    // set ISR
    setTxInterruptAction(radioInterrupt);

    // set that we expect to finish TX
    disableTransmittedFlag();
    enableTxInterrupt();

    // Start async TX.
    int16_t state = T::startTransmit(data, len, 0); // 3rd param is not used.
    if (state != RADIOLIB_ERR_NONE)
      return state;

    return RADIOLIB_ERR_NONE;
  }

  virtual bool isTxChannelFree()
  {
    return (
        !_enableTxInterrupt // not waiting to finish TX
    );
  }

  /**
   * Transmit an RFQuack Packet.
   *
   * @param pkt packet to transmit
   *
   * @return \ref status_codes
   */
  uint8_t transmit(rfquack_Packet *pkt)
  {
    if (pkt->has_repeat && pkt->repeat == 0)
    {
      RFQUACK_LOG_TRACE(F("Zero packet repeat: no transmission"))
      return RADIOLIB_ERR_NONE;
    }

    uint32_t repeat = 1;
    uint32_t correct = 0;

    if (pkt->has_repeat)
    {
      repeat = pkt->repeat;
    }

    for (uint32_t i = 0; i < repeat; i++)
    {
      int16_t result = transmit((uint8_t *)(pkt->data.bytes), pkt->data.size);
      RFQUACK_LOG_TRACE(F("Packet trasmitted, resultCode=%d"), result)

      if (result == RADIOLIB_ERR_NONE)
      {
        correct++;
      }
    }

    RFQUACK_LOG_TRACE(F("%d packets transmitted"), correct)
    if (correct > 0)
      return RADIOLIB_ERR_NONE;
    return RADIOLIB_ERR_UNKNOWN;
  }

  /**
   * True whenever there's data available on radio's RX FIFO.
   *
   * @return whether there is data available to read.
   */
  virtual bool isIncomingDataAvailable()
  {
    // Flag makes sense only if in RX mode.
    if (_mode != rfquack_Mode_RX)
    {
      return false;
    }

    return _receivedFlag;
  }

  /**
   * Consumes data from radio RX FIFO.
   *
   * @param[out] data pointer to memory area to write data to.
   * @param len Number of bytes that will be read. When 0, the packet * length will be retrieved automatically. When more bytes than received * are requested, only the number of bytes requested will be returned.
   *
   * @return \ref status_codes
   */

  virtual int16_t readData(uint8_t *data, size_t len)
  {
    // Exit if not in RX mode.
    if (_mode != rfquack_Mode_RX)
    {
      RFQUACK_LOG_TRACE(F("Trying to readData without being in RX mode."))
      return ERR_WRONG_MODE;
    }

    // Read data from fifo.
    return T::readData(data, len);
  }

  /**
   * Main transmit loop; check whether there's data being transmitted and clears IRQs.
   */
  void txLoop()
  {
    if (_transmittedFlag)
    {
      // disable the interrupt service routine while
      // processing the data
      disableTxInterrupt();

      // reset TX flag
      disableTransmittedFlag();

      // clean up after transmission is finished
      // this will ensure transmitter is disabled,
      // RF switch is powered down etc.
      T::finishTransmit();
      _canTransmit = true;
    }
  }

  /**
   * Main receive loop; reads any data from the RX FIFO and push it to the RX queue.
   *
   * @param[out] rxQueue pointer to queue to write received data to.
   */
  void rxLoop(Queue *rxQueue)
  {
    // only if we're waiting to receive something (faster polling loop)
    if (_enableRxInterrupt)
    {
      // Check if there's pending data on radio's RX FIFO.
      if (isIncomingDataAvailable())
      {
        // disable the interrupt service routine while
        // processing the data
        disableRxInterrupt();

        // reset RX flag
        disableReceivedFlag();

        rfquack_Packet pkt = rfquack_Packet_init_zero;

        // Pop packet from RX FIFO.
        uint8_t packetLen = getPacketLength(true);
        uint64_t startReceive = millis();
        int16_t result = readData((uint8_t *)pkt.data.bytes, packetLen);

        if (result != RADIOLIB_ERR_NONE)
        {
          RFQUACK_LOG_ERROR(F("Error while reading data from driver, code=%d"), result);
        }

        RFQUACK_LOG_TRACE(F("Putting radio back in RX"));

        // Put radio back in receiveMode.
        receiveMode();

        // Fill missing data
        pkt.data.size = packetLen;
        pkt.millis = startReceive;
        pkt.has_millis = true;
        pkt.rxRadio = this->_whichRadio;
        pkt.has_rxRadio = true;
        strcpy(pkt.model, getChipName());
        pkt.has_model = true;

        // Set the syncWords
        pkt.has_syncWords = (getSyncWord(pkt.syncWords.bytes, &(pkt.syncWords.size))) == RADIOLIB_ERR_NONE;

        // Set the bitrate
        pkt.has_bitRate = (getBitRate(&(pkt.bitRate))) == RADIOLIB_ERR_NONE;

        // Set the carrierFreq
        pkt.has_carrierFreq = (getFrequency(&(pkt.carrierFreq))) == RADIOLIB_ERR_NONE;

        pkt.has_frequencyDeviation = (getFrequencyDeviation(&(pkt.frequencyDeviation))) == RADIOLIB_ERR_NONE; // Set the frequency deviation

        pkt.has_modulation = getModulation(pkt.modulation) == RADIOLIB_ERR_NONE; // Set the modulation

        pkt.has_RSSI = (getRSSI(&(pkt.RSSI))) == RADIOLIB_ERR_NONE; // Set the RSSI

        // onPacketReceived() hook
        if (modulesDispatcher.onPacketReceived(pkt, _whichRadio))
        {
          // If packet passed filtering put it in rxQueue.
          enqueuePacket(&pkt, rxQueue);
        }
      }
    }
  }

  /**
   * Reads a radio's internal register.
   *
   * @param reg address of register to read from.
   *
   * @return rfquack_register_value_t content read from the register.
   */
  virtual rfquack_register_value_t readRegister(rfquack_register_address_t reg)
  {
    return T::_mod->SPIreadRegister((uint8_t)reg);
  }

  /**
   * Writes to a radio's internal register.
   *
   * @param reg register address to write to.
   * @param value value to write.
   *
   * @param msb Most significant bit of the register variable. Bits above this one will be masked out.
   * @param lsb Least significant bit of the register variable. Bits below this one will be masked out.
   *
   * @returns Masked register value or status code.
   */
  virtual void writeRegister(rfquack_register_address_t reg, rfquack_register_value_t value, uint8_t msb = 7, uint8_t lsb = 0) = 0;

  /**
   * Sets transmitted / received preamble length.
   *
   * @param size Number of bytes of the preamble.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setPreambleLength(uint32_t size)
  {
    RFQUACK_LOG_ERROR(F("setPreambleLength was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets radio frequency.
   *
   * @param freq Carrier frequency in MHz.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setFrequency(float freq)
  {
    RFQUACK_LOG_ERROR(F("setFrequency was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Gets the radio frequency.
   *
   * @param[out] freq Variable where the carrierFrequency gets stored, in MHz
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getFrequency(float *freq)
  {
    RFQUACK_LOG_ERROR(F("getFrequency was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets frequency deviation.
   *
   * @param frequency deviation in kHz
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setFrequencyDeviation(float freqDev)
  {
    RFQUACK_LOG_ERROR(F("setFrequencyDeviation was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Gets the radio frequency deviation.
   *
   * @param[out] freqDev variable where the frequency deviation gets stored, in MHz
   * @return result code (ERR_NONE or ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getFrequencyDeviation(float *freqDev)
  {
    RFQUACK_LOG_ERROR(F("getFrequencyDeviation was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets receiver bandwidth.
   *
   * @param rxBw in MHz
   * @return result code (ERR_NONE or ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setRxBandwidth(float rxBw)
  {
    RFQUACK_LOG_ERROR(F("setRxBandwidth was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Gets receiver bandwidth.
   *
   * @param[out] rxBw Pointer to variable where to store the RX bandwidth in MHz.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getRxBandwidth(float *rxBw)
  {
    RFQUACK_LOG_ERROR(F("getRxBandwidth was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets bit rate.
   *
   * @param br Bitrate in kbps.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setBitRate(float br)
  {
    RFQUACK_LOG_ERROR(F("setBitRate was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Retrieves the bit rate.
   *
   * @param[out] br variable where bitrate gets stored
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getBitRate(float *br)
  {
    RFQUACK_LOG_ERROR(F("getBitRate was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets radio output power.
   *
   * @param txPower TX power in dBm.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setOutputPower(uint32_t txPower)
  {
    RFQUACK_LOG_ERROR(F("setOutputPower was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Retrieves radio output power.
   *
   * @param[out] txPower Pointer to variable where result will be stored.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getOutputPower(uint32_t *txPower)
  {
    RFQUACK_LOG_ERROR(F("setOutputPower was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets radio syncWord and its size.
   *
   * @param bytes Pointer to variable holding the sync word.
   * @param size Sync word size in bytes.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setSyncWord(uint8_t *bytes, pb_size_t size)
  {
    RFQUACK_LOG_ERROR(F("setSyncWord was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Gets radio sync word and its size.
   *
   * @param bytes Pointer to buffer where the sync word will be stored.
   * @param[out] size pointer to variable where the sync word size will be stored.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getSyncWord(uint8_t *bytes, pb_size_t *size)
  {
    RFQUACK_LOG_ERROR(F("getSyncWord was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Puts radio in fixed packet length mode.
   *
   * @param len Packet length in bytes.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t fixedPacketLengthMode(uint8_t len)
  {
    RFQUACK_LOG_ERROR(F("fixedPacketLengthMode was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Puts radio in fixed packet length mode.
   *
   * @param len Maximum packet size in bytes.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t variablePacketLengthMode(uint8_t len)
  {
    RFQUACK_LOG_ERROR(F("variablePacketLengthMode was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Puts radio in promiscuous mode.
   *
   * @param isPromiscuous
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setPromiscuousMode(bool isPromiscuous)
  {
    RFQUACK_LOG_ERROR(F("setPromiscuousMode was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Enables / Disables CRC filtering.
   *
   * @param crcOn Enable CRC filter if true. Disable otherwise.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setCrcFiltering(bool crcOn)
  {
    RFQUACK_LOG_ERROR(F("setCrcFiltering was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Enables / Disables automatic packet acknowledgement.
   *
   * @param autoAckOn Enable automatic packet acknowledgement if true. Disable otherwise.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setAutoAck(bool autoAckOn)
  {
    RFQUACK_LOG_ERROR(F("setAutoAck was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Whatever carrier presence was detected during last RX.
   *
   * @param[out] bool Pointer to variable to store the result.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t isCarrierDetected(bool *isDetected)
  {
    RFQUACK_LOG_ERROR(F("isCarrierDetected was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Gets RSSI (Recorded Signal Strength Indicator) of the last received packet.
   *
   * @param[out] rssi Pointer to variable where the RSSI will be stored.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual float getRSSI(float *rssi)
  {
    RFQUACK_LOG_ERROR(F("getRSSI was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets the transmission encoding.
   *
   * @param encoding Encoding to be used.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setEncoding(uint8_t encoding)
  {
    RFQUACK_LOG_ERROR(F("setEncoding was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets the radio modulation.
   *
   * @param modulation Modulation class.
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t setModulation(rfquack_Modulation modulation)
  {
    RFQUACK_LOG_ERROR(F("setModulation was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Gets the radio modulation.
   *
   * @param[out] modulation pointer to char buffer where modulation will be stored as a string (properly terminated).
   *
   * @return \ref status_codes (\ref ERR_NONE or \ref ERR_COMMAND_NOT_IMPLEMENTED)
   */
  virtual int16_t getModulation(char *modulation)
  {
    RFQUACK_LOG_ERROR(F("getModulation was not implemented."));

    return ERR_COMMAND_NOT_IMPLEMENTED;
  }

  /**
   * Sets callback on RX interrupt.
   */
  virtual void setRxInterruptAction(void (*func)(void *)) = 0;

  /**
   * Sets callback on end-of-TX interrupt.
   */
  virtual void setTxInterruptAction(void (*func)(void *)) = 0;

  virtual void removeInterrupts() = 0;

  /**
   * @brief Get the chip name.
   *
   * @return char* chip name.
   */
  char *getChipName() const
  {
    return chipName;
  }

protected:
  rfquack_Mode _mode = rfquack_Mode_IDLE; // RFQRADIO_MODE_[STANDBY|RX|TX|JAM]

private:
  char *chipName;
  rfquack_WhichRadio _whichRadio;

  /**
   * Enqueue a packet onto the queue.
   *
   * @param[in] packet Packet to enqueue.
   * @param[out] rxQueue Queue to enqueue packets to.
   */
  void enqueuePacket(rfquack_Packet *packet, Queue *rxQueue)
  {
    if (packet->data.size > sizeof(rfquack_Packet))
    {
      RFQUACK_LOG_ERROR(F("Packet payload is grater than container."));
      return;
    }

    if (rxQueue->isFull())
    {
      RFQUACK_LOG_ERROR(F("rxQueue is full"));
      return;
    }

    rxQueue->push(packet);
    RFQUACK_LOG_TRACE(F("Packet put in rxQueue, size %d bytes"), packet->data.size);
  }
};

#endif // RFQUACK_PROJECT_RADIOLIBWRAPPER_H
