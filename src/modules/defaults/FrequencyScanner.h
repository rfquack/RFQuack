#ifndef RFQUACK_PROJECT_FREQUENCYSCANNER_H
#define RFQUACK_PROJECT_FREQUENCYSCANNER_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"

class FrequencyScannerModule : public RFQModule {
public:
    FrequencyScannerModule() : RFQModule("frequency_scanner") {}

    virtual void onInit() {
    }

    virtual bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) {
      // Discharge every packet while freq scanning.
      return false;
    }

    bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      return true;
    }

    virtual void
    executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload, unsigned int messageLen) {
      // Start frequency scan
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "start", "Starts frequency scan", start(reply))

      // Frequency step.
      CMD_MATCHES_FLOAT("freq_step",
                        "Frequency step in Mhz (default: 1)",
                        frequencyStep)

      // Start frequency.
      CMD_MATCHES_FLOAT("start_freq",
                        "Start frequency in Mhz (default: 2400)",
                        startFrequency)

      // End frequency.
      CMD_MATCHES_FLOAT("end_freq",
                        "End frequency in Mhz (default: 2500)",
                        endFrequency)

      // Radio to use.
      CMD_MATCHES_WHICHRADIO("which_radio",
                             "Radio to use (default: RadioA)",
                             radioToUse)

      // Radio to use.
      CMD_MATCHES_UINT("rounds",
                       "How many times sweep on frequency range (default: 3)",
                       rounds)
    }

    void start(rfquack_CmdReply &reply) {
      // Check if start and stop frequencies are allowed.
      if (!setFrequency(startFrequency)) {
        setReplyMessage(reply, F("startFrequency is not valid"), ERR_INVALID_FREQUENCY);
        return;
      }
      if (!setFrequency(endFrequency) || endFrequency <= startFrequency) {
        setReplyMessage(reply, F("endFrequency is not valid"), ERR_INVALID_FREQUENCY);
        return;
      }

      if (frequencyStep < 0) {
        setReplyMessage(reply, F("Frequency step must be positive"), ERR_INVALID_FREQUENCY);
        return;
      }

      // Check if radio supports RSSI API. If not look, as alternative, for Carrier Detection API:
      bool hasRSSI;
      bool hasCD;
      {
        float fakeRSSI;
        hasRSSI = rfqRadio->getRSSI(fakeRSSI, radioToUse) != ERR_COMMAND_NOT_IMPLEMENTED;
        bool fakeCD;
        hasCD = rfqRadio->isCarrierDetected(fakeCD, radioToUse) != ERR_COMMAND_NOT_IMPLEMENTED;
      }

      // Exit if neither RSSI or CD is there.
      if (!hasCD && !hasRSSI) {
        setReplyMessage(reply, F("Radio needs to support RSSI or Carrier Detection"), ERR_COMMAND_NOT_IMPLEMENTED);
        return;
      }

      // CC1101 best config for freq scanning is max br (255 kbps), GSK (FSK2 is ok too) and 102 kHz filter bw
      // q.radioA.set_modem_config(bitRate=255, modulation="FSK2", rxBandwidth=102)
      // TODO: Find a way to apply them here only if radio is CC1101

      // Enable module in order to catch and discharge every packet received promiscuously
      this->enabled = true;

      uint16_t hops = (endFrequency - startFrequency) / frequencyStep;
      RFQUACK_LOG_TRACE(F("We'll change frequency %d times"), hops)

      // malloc an array to store results
      Item *results{new Item[hops]{}};
      memset(results, 0, sizeof(Item) * hops);

      // Start sweeping
      for (int sweep = 0; sweep < rounds; sweep++) {
        RFQUACK_LOG_TRACE("Scan round %d / %d", sweep, rounds)

        float currentFreq = startFrequency;
        for (int hop = 0; hop < hops; hop++) {

          // Try to set frequency
          if (!setFrequency(currentFreq)) {
            Log.error(F("Unable to setFrequency = %d Hz"), (int) (currentFreq * 1000));
          } else {
            Log.trace(F("Set frequency = %d Hz"), (int) (currentFreq * 1000));

            // Put radio in RX mode.
            rfqRadio->setMode(rfquack_Mode_RX, radioToUse);

            delayMicroseconds(1700);

            // Check if something was transmitting via carrier detection.
            if (hasCD && !hasRSSI) {
              bool isCarrierDetected;
              rfqRadio->isCarrierDetected(isCarrierDetected, radioToUse);

              if (isCarrierDetected) {
                results[hop].hop = hop; // Should set it only first time,
                results[hop].detections++;
              }
            } else {
              // Check if something was transmitting via RSSI
              float rssi;
              rfqRadio->getRSSI(rssi, radioToUse);
              results[hop].hop = hop; // Should set it only first time,
              results[hop].detections += rssi; // Who minds the fractional part.
            }

            rfqRadio->setMode(rfquack_Mode_IDLE, radioToUse);
          }
          currentFreq += frequencyStep;
        }
        // Sweep is over.
      }

      // Disable module
      this->enabled = false;

      // Sort the array of results
      bubbleSort(results, hops);

      // Build reply message
      // At the end of the array we find the max number of carrier detections.
      uint8_t itemIndex = hops - 1;
      if (results[itemIndex].detections == 0) {
        setReplyMessage(reply, F("Nothing detected"));
        return;
      }

      uint8_t sentItems = 0;
      while (itemIndex > 0 && sentItems < 10 && results[itemIndex].detections != 0) {
        rfquack_CmdReply frequencyReply = rfquack_CmdReply_init_default;
        char message[30];
        uint32_t frequency = (startFrequency + (float) (results[itemIndex].hop) * frequencyStep) * 1000;

        if (hasCD && !hasRSSI){
          // Detections is the number of times the carrier was detected on that frequency.
          sprintf(message, "%d Hz carrier detections", frequency, results[itemIndex].hop);
          setReplyMessage(frequencyReply, message, results[itemIndex].detections);
        }else{
          // Detections is a summation of RSSI, divide it to get an average RSSI.
          sprintf(message, "%d Hz average RSSI", frequency, results[itemIndex].hop);
          setReplyMessage(frequencyReply, message, results[itemIndex].detections / rounds);
        }

        PB_ENCODE_AND_SEND(rfquack_CmdReply, frequencyReply, RFQUACK_TOPIC_SET, this->name, "start");
        sentItems++;
        itemIndex--;
      }

      delete[] results;
      rfqRadio->setPromiscuousMode(false, radioToUse);

      setReplyMessage(reply, F("Sending top 10 frequencies"));
    }

private:
    struct Item {
        uint8_t hop;
        float detections;
    };

    void bubbleSort(Item a[], uint8_t size) {
      for (int i = 0; i < (size - 1); i++) {
        for (int o = 0; o < (size - (i + 1)); o++) {
          if (a[o].detections > a[o + 1].detections) {
            Item t = a[o];
            a[o] = a[o + 1];
            a[o + 1] = t;
          }
        }
      }
    }


    bool setFrequency(float freq) {
      rfquack_ModemConfig modemConfig = rfquack_ModemConfig_init_default;
      modemConfig.has_carrierFreq = true;
      modemConfig.carrierFreq = freq;
      uint8_t changes = 0;
      uint8_t failures = 0;
      rfqRadio->setModemConfig(modemConfig, radioToUse, changes, failures);
      return (changes == 1 && failures == 0);
    }

    float frequencyStep = 1;
    float startFrequency = 2400;
    float endFrequency = 2525;
    uint8_t rounds = 5;
    rfquack_WhichRadio radioToUse = rfquack_WhichRadio_RadioA;
};

#endif //RFQUACK_PROJECT_FREQUENCYSCANNER_H