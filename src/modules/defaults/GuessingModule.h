#ifndef RFQUACK_PROJECT_GUESSINGSCANNERMODULE_H
#define RFQUACK_PROJECT_GUESSINGSCANNERMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"

// This module is time critical, logging makes a big difference :)
#define GUESSING_MODULE_LOG(...) {}

extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

class GuessingModule : public RFQModule, public OnPacketReceived, public OnLoop {
public:
    GuessingModule() : RFQModule("guessing") {}

    void onInit() override {
      // Nothing to do :)
    }

    bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // Trash packet if module should only estimate frequency.
      if (onlyFrequency) {
        return false;
      }

      lastRxActivity = millis();
      return true;
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {
      // Start the module.
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "start", "Starts the guessing module", start(reply))

      // Stops the module.
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "stop", "Stops the guessing module", stop(reply))

      // Start frequency.
      CMD_MATCHES_FLOAT("start_freq",
                        "Start frequency in Mhz (default: 432)",
                        startFrequency)

      // End frequency.
      CMD_MATCHES_FLOAT("end_freq",
                        "End frequency in Mhz (default: 437)",
                        endFrequency)

      // RSSI Threshold.
      CMD_MATCHES_FLOAT("rssi_threshold",
                        "Minimum accepted RSSI in dB (default: -60)",
                        rssiThreshold)

      // Sampling bitrate.
      CMD_MATCHES_FLOAT("sampling_bitrate",
                        "Bitrate used for oversampling (default: 100)",
                        samplingBitrate)

      // Only estimate frequency
      CMD_MATCHES_BOOL("only_frequency",
                       "Only estimate frequency (default: false)",
                       onlyFrequency)

      // Radio to use.
      CMD_MATCHES_WHICHRADIO("which_radio",
                             "Radio to use (default: RadioA)",
                             scanRadio)
    }

    void start(rfquack_CmdReply &reply) {

      // This module is optimized for CC1101 :(
      const char *chipName = rfqRadio->getChipName(scanRadio);
      if (strncmp(chipName, "CC1101", sizeof(*chipName)) != 0) {
        setReplyMessage(reply, F("Please, use a CC1101."), -1);
        return;
      }

      // Check if start and stop frequencies are allowed.
      if (int16_t result = rfqRadio->setFrequency(startFrequency, scanRadio) != RADIOLIB_ERR_NONE) {
        setReplyMessage(reply, F("startFrequency is not valid"), result);
        return;
      }
      if (int16_t result =
        rfqRadio->setFrequency(endFrequency, scanRadio) != RADIOLIB_ERR_NONE || endFrequency <= startFrequency) {
        setReplyMessage(reply, F("endFrequency is not valid"), result);
        return;
      }

      // Save registers before modifying them.
      if (previousRegisters == nullptr) {
        saveRegisters();
      }

      // Disable autocal
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_MCSM0, RADIOLIB_CC1101_FS_AUTOCAL_NEVER, 5, 4, scanRadio);

      bootstrapScanning();

      calibrate();

      // Enable module.
      this->enabled = true;

      setReplyMessage(reply, F("Started."));
    }

    void stop(rfquack_CmdReply &reply) {
      // Disable the module.
      this->enabled = false;

      // Restore registers
      if (previousRegisters != nullptr) {
        restoreRegisters();
      }

      setReplyMessage(reply, F("Stopped."));
    }

    float almostBinarySearch(float binStartFreq, float binEndFreq, uint8_t bwStep) {
      GUESSING_MODULE_LOG("almostBinarySearch(%i, %i, %i)", (int) (binStartFreq * 1000), (int) (binEndFreq * 1000),
                          bwStep)

      // Select the right filter BW
      float filterBw = 0;
      float freqSpacing = 0;

      if (bwStep == 0) {
        GUESSING_MODULE_LOG("Pick 812 bw")
        bw812();
        filterBw = 0.812;
      } else if (bwStep == 1) {
        GUESSING_MODULE_LOG("Pick 406 bw")
        bw406();
        filterBw = 0.406;
      } else if (bwStep == 2) {
        GUESSING_MODULE_LOG("Pick 203 bw")
        bw203();
        filterBw = 0.203;
      } else if (bwStep == 3) {
        GUESSING_MODULE_LOG("Pick 102 bw")
        bw102();
        filterBw = 0.102;
      } else if (bwStep == 4) {
        GUESSING_MODULE_LOG("Pick 58 bw")
        bw58();
        filterBw = 0.058;
      } else {
        return (binEndFreq + binStartFreq) / 2;
      }
      freqSpacing = filterBw / 4 * 3;

      uint8_t numOfChunks =
        ((uint8_t) ((binEndFreq - binStartFreq) / freqSpacing)) + 1;  // Number of chunks in which band is divided.
      uint8_t numOfSamples = 1;

      float maxRssi = -100;
      float maxRssiFreq = -1;
      for (uint8_t j = 0; j < numOfSamples; j++) {
        for (uint8_t i = 0; i < numOfChunks; i++) {

          float freq = binStartFreq + (i + 0.5f) * freqSpacing;
          // Synth on freq
          setFrequency(freq);
          rfqRadio->setMode(rfquack_Mode_RX, scanRadio);


          // Wait to settle RSSI
          delayMicroseconds(600);

          // Update the maximum RSSI.
          float rssiValue = getRSSI();
          if (rssiValue > maxRssi) {
            maxRssi = rssiValue;
            maxRssiFreq = freq;
          }

          // Debug:
          if (j == numOfSamples - 1) {
            GUESSING_MODULE_LOG("AVG RSSI on %i is %i", (int) (freq * 1000), (int) rssiValue)
          }
        }
      }

      // In the 0th step go ahead only if threshold is passed.
      if (bwStep == 0 && maxRssi < rssiThreshold) {
        return -1;
      }

      if (maxRssiFreq == -1) {
        // Should never happen, it's due to an improper delay time.
        RFQUACK_LOG_ERROR("RSSI not settled.");
        return -1;
      }

      // Recursive step
      float newStart = maxRssiFreq - filterBw / 2;
      float newEnd = maxRssiFreq + filterBw / 2;
      return almostBinarySearch(newStart, newEnd, bwStep + 1);
    }

    void onLoop() override {
      // Skip if a packet was received in last 100ms... there's no need to change freq :)
      if (millis() - lastRxActivity < 100) {
        return;
      }

      // Bitrate estimation alters gains and bitrate; just bootstrap again if needed.
      if (needsBootstrap) {
        needsBootstrap = false;
        bootstrapScanning();
      }

      unsigned long start = micros();
      float frequency = almostBinarySearch(startFrequency, endFrequency, 0);
      unsigned long freqRecoveryDuration = micros() - start;
      if (frequency != -1) {

        // Send the estimated frequency and start a new frequency recovery loop.
        if (onlyFrequency) {
          sendEstimatedFrequency(frequency);
          lastRxActivity = millis(); // Do not spam.
          return;
        }

        setFrequency(frequency);
        unsigned long startEstimateBitrate = micros();
        float estimatedBitrate = estimateBitrate();
        unsigned long estimateBitrateDuration = micros() - startEstimateBitrate;

        RFQUACK_LOG_TRACE("freqRecovery in %i uS, estimateBitrateDuration in %i uS", freqRecoveryDuration,
                          estimateBitrateDuration)
        RFQUACK_LOG_TRACE("Got frequency %d kHz, Bitrate is %i bps",
                          (int) (frequency * 1000),
                          (int) (estimatedBitrate * 1000)
        )
      }
    }


private:
    void sendEstimatedFrequency(float freq) {
      rfquack_CmdReply cmdReply = rfquack_CmdReply_init_default;
      setReplyMessage(cmdReply, F("Found frequency"), (uint32_t) (freq * 1000));
      PB_ENCODE_AND_SEND(rfquack_CmdReply, cmdReply, RFQUACK_TOPIC_SET, this->name, "frequency")
    }

    // Sets the registers needed to perform accurate freq scanning.
    void bootstrapScanning() {
      RFQUACK_LOG_TRACE(F("bootstrapScanning()"))

      // Configure the radio.
      int16_t status = rfqRadio->setMode(rfquack_Mode_IDLE, scanRadio);
      status |= rfqRadio->setBitRate(250, scanRadio);
      status |= rfqRadio->setModulation(rfquack_Modulation_OOK, scanRadio);
      status |= rfqRadio->setCrcFiltering(true, scanRadio);

      // Require syncWord, in order to stop packet capture.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_SYNC_MODE_16_16, 2, 0, scanRadio);

      // Re-Enable the highest gain.
      // This helps during freq scanning *BUT* will cause noise to trigger the CS during RX.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAX_DVGA_GAIN_0, 7, 6, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_LNA_GAIN_REDUCE_17_1_DB, 5, 3, scanRadio);


      // SmartRF specific values for the chosen RF band.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_FSCTRL1, 0x06, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_FREND1, 0x56, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_FSCAL0, 0x1F, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST0, 0x09, scanRadio);

      // Start selecting 812kHz BW.
      bw812();
    }


    float estimateBitrate(float samplingBitrate, uint8_t *data, uint8_t len) {
      // Count the number of consecutive 1
      int32_t consecutiveNumberOf1 = 0;
      bool wasLastBitA1 = false;

#define COUNTER_SIZE 50
      int32_t counterOfConsecutive1[COUNTER_SIZE] = {0}; // array[numberOf1s] = number_of_occurrencies.

      for (uint8_t iByte = 0; iByte < len; iByte++) {
        for (int bit = 0; bit < 8; bit++) {
          int8_t shifted = (data[iByte] << bit);
          if (shifted < 0) {
            // The left most bit is 1.
            if (wasLastBitA1) {
              consecutiveNumberOf1++;
            } else {
              wasLastBitA1 = true;
              consecutiveNumberOf1 = 1;
            }
          } else {
            // The left most bit is a 0
            if (wasLastBitA1) {
              wasLastBitA1 = false;

              if (consecutiveNumberOf1 > 0 && consecutiveNumberOf1 < COUNTER_SIZE) {
                // Increase the number of times we've seen this consecutiveNumberOf1.
                counterOfConsecutive1[consecutiveNumberOf1]++;
              }
              consecutiveNumberOf1 = 0;
            }
          }
        }
      }

      // Find the most occurrent number of 1:
      int32_t mostOccurrentNumberOf1 = 1;
      int32_t maxOccurrence = 1;

      for (uint8_t i = 0; i < COUNTER_SIZE; i++) {
        if (counterOfConsecutive1[i] > maxOccurrence) {
          maxOccurrence = counterOfConsecutive1[i];
          mostOccurrentNumberOf1 = i;
        }
      }

      if (mostOccurrentNumberOf1 > 1) {
        return samplingBitrate /
               (((float) (
                 (mostOccurrentNumberOf1 - 1) * counterOfConsecutive1[mostOccurrentNumberOf1 - 1] +
                 (mostOccurrentNumberOf1 + 0) * counterOfConsecutive1[mostOccurrentNumberOf1 + 0] +
                 (mostOccurrentNumberOf1 + 1) * counterOfConsecutive1[mostOccurrentNumberOf1 + 1]
               )) /
                ((float) (counterOfConsecutive1[mostOccurrentNumberOf1 - 1] +
                          counterOfConsecutive1[mostOccurrentNumberOf1 + 0] +
                          counterOfConsecutive1[mostOccurrentNumberOf1 + 1])));
      } else {
        return samplingBitrate / (float) mostOccurrentNumberOf1;
      }
    }

    float estimateBitrate() {
      // Increase bandwidth.
      bw203();

      // Set the radio.
      rfqRadio->setModulation(rfquack_Modulation_OOK, scanRadio);
      rfqRadio->setCrcFiltering(false, scanRadio);
      rfqRadio->setSyncWord(nullptr, 0, scanRadio); // Disable syncWord. Will enable CS detection without syncw.
      rfqRadio->setBitRate(samplingBitrate, scanRadio);
      rfqRadio->fixedPacketLengthMode(254, scanRadio);

      // Reduce gain to reduce noise:
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAX_DVGA_GAIN_1, 7, 6, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_LNA_GAIN_REDUCE_17_1_DB, 5, 3, scanRadio);

      // Start receive
      rfqRadio->setMode(rfquack_Mode_RX, scanRadio);

      lastRxActivity = millis();

      // Wait 10mS for a packet to estimate the bitrate.
      byte receivedData[64 + 1];
      float estimatedBitrate = -1;
      while (millis() - lastRxActivity < 10) {
        if (((RFQCC1101 *) (rfqRadio->getNativeDriver(scanRadio)))->isIncomingDataAvailable()) { // FIX FIX
          ((RFQCC1101 *) (rfqRadio->getNativeDriver(scanRadio)))->readData((uint8_t *) receivedData, 32);
          estimatedBitrate = estimateBitrate(samplingBitrate, receivedData, 32);
          rfqRadio->setBitRate(estimatedBitrate, scanRadio);
          lastRxActivity = millis();
          break;
        }
      }

      needsBootstrap = true; // Re-bootstrap radio registers after packets are received.
      return estimatedBitrate;
    }

    bool needsBootstrap = false;
    ulong lastRxActivity = 0;

    float getRSSI() {
      float rssi;
      rfqRadio->getRSSI(&rssi, scanRadio);
      return rssi;
    }

    void saveRegisters() {
      if (previousRegisters != nullptr) {
        RFQUACK_LOG_ERROR(F("previousRegisters is not null"));
        return;
      }

      uint8_t registersToStore[] = {RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_REG_MCSM0, RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_REG_FSCTRL1,
                                    RADIOLIB_CC1101_REG_FREND1,
                                    RADIOLIB_CC1101_FSCAL0, RADIOLIB_CC1101_REG_TEST0, RADIOLIB_CC1101_REG_TEST1, RADIOLIB_CC1101_REG_TEST2};

      // Store a copy of the registers since we'll alter them.
      previousRegisters = new rfquack_Register[sizeof(registersToStore)];
      previousRegistersSize = sizeof(registersToStore);

      for (int i = 0; i < previousRegistersSize; i++) {
        previousRegisters[i].address = registersToStore[i];
        previousRegisters[i].value = rfqRadio->readRegister(registersToStore[i], scanRadio);
      }
    }

    void restoreRegisters() {
      if (previousRegisters == nullptr) {
        RFQUACK_LOG_ERROR(F("previousRegisters null"));
        return;
      }

      // Put registers back.
      for (int i = 0; i < previousRegistersSize; i++) {
        rfqRadio->writeRegister(previousRegisters[i].address, previousRegisters[i].value, scanRadio);

      }
      delete[] previousRegisters;
      previousRegisters = nullptr;
    }

    // Store previous value of altered registers.
    rfquack_Register *previousRegisters = nullptr;
    uint8_t previousRegistersSize = 0;

    void calibrate() {
      // Delete any previous calibration.
      if (FSCALA1 != nullptr || FSCALA2 != nullptr || FSCALA3 != nullptr) {
        delete[] FSCALA1;
        delete[] FSCALA2;
        delete[] FSCALA3;
      }

      fscalSize = (uint16_t) ((endFrequency - startFrequency) / spacing) + 1;

      FSCALA1 = new uint8_t[fscalSize];
      FSCALA2 = new uint8_t[fscalSize];
      FSCALA3 = new uint8_t[fscalSize];

      float frequency;
      for (int i = 0; i < fscalSize; i++) {
        frequency = startFrequency + (float) i * spacing;
        RFQUACK_LOG_TRACE("Calibrating on %i KHz", (int) (frequency * 1000));

        // Set frequency.
        rfqRadio->setFrequency(frequency, scanRadio);
        delay(1);
        ((RFQCC1101 *) (rfqRadio->getNativeDriver(scanRadio)))->scal();
        delay(2);

        // Store calibration
        FSCALA1[i] = rfqRadio->readRegister(RADIOLIB_CC1101_REG_FSCAL1, scanRadio);
        FSCALA2[i] = rfqRadio->readRegister(RADIOLIB_CC1101_REG_FSCAL2, scanRadio);
        FSCALA3[i] = rfqRadio->readRegister(RADIOLIB_CC1101_REG_FSCAL3, scanRadio);
      }
    }

    void setFrequency(float freq) {
      int freqId = (int) ((freq - startFrequency) / spacing);

      rfqRadio->setFrequency(freq, scanRadio);

      // SET FSCAL REGS:
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_FSCAL1, FSCALA1[freqId], scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_FSCAL2, FSCALA2[freqId], scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_FSCAL3, FSCALA3[freqId], scanRadio);
    }

    // FSCAL:
    uint8_t *FSCALA1 = nullptr;
    uint8_t *FSCALA2 = nullptr;
    uint8_t *FSCALA3 = nullptr;
    uint16_t fscalSize = 0;

    // These values are dumped from SMART RF
    void bw812() {
      rfqRadio->setRxBandwidth(812, scanRadio);
      // 33 for <100Khz, 42Khz otherwise.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAGN_TARGET_42_DB, 2, 0, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST2, 0x88, scanRadio);  //// FOR BW 812
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST1, 0x31, scanRadio); //// FOR BW 812
    }

    void bw406() {
      rfqRadio->setRxBandwidth(406, scanRadio);
      // 33 for <100Khz, 42Khz otherwise.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAGN_TARGET_42_DB, 2, 0, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST2, 0x88, scanRadio);  //// FOR BW 406
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST1, 0x31, scanRadio); //// FOR BW 406
    }

    void bw102() {
      rfqRadio->setRxBandwidth(102, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAGN_TARGET_33_DB, 2, 0, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST2, 0x81, scanRadio);  //// FOR BW 102
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST1, 0x35, scanRadio); //// FOR BW 102
    }

    void bw203() {
      rfqRadio->setRxBandwidth(203, scanRadio);
      // 33 for <100Khz, 42Khz otherwise.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAGN_TARGET_33_DB, 2, 0, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST2, 0x81, scanRadio);  //// FOR BW 203
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST1, 0x35, scanRadio); //// FOR BW 203
    }

    void bw58() {
      rfqRadio->setRxBandwidth(58, scanRadio);
      // 33 for <100Khz, 42Khz otherwise.
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_AGCCTRL2, RADIOLIB_CC1101_MAGN_TARGET_33_DB, 2, 0, scanRadio);
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST2, 0x81, scanRadio); //// FOR BW 58
      rfqRadio->writeRegister(RADIOLIB_CC1101_REG_TEST1, 0x35, scanRadio); ///// FOR BW 58
    }

    float spacing = 0.02;
    float startFrequency = 432;
    float endFrequency = 437;
    float rssiThreshold = -60;
    float samplingBitrate = 30;
    bool onlyFrequency = false;
    rfquack_WhichRadio scanRadio = rfquack_WhichRadio_RadioA;
};

#endif //RFQUACK_PROJECT_FREQUENCYSCANNERMODULE_H