#ifndef RFQUACK_PROJECT_STATSMODULE_H
#define RFQUACK_PROJECT_STATSMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"
#include "../../rfquack_radio.h"


extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.
extern rfquack_Status rfq; // Status of RFQuack

class StatsModule : public RFQModule {
public:
    StatsModule() : RFQModule(RFQUACK_TOPIC_STATUS) {}

    void onInit() override {
      sendStatus();
    }

    bool onPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) override {
      // Packet will be passed to the following module.
      return true;
    }

    bool afterPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) override {
      // Packet will be passed to the following module.
      return true;
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen,
                            char *messagePayload, unsigned int messageLen) override {

      // Set modem configuration: "rfquack/in/get/status"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], RFQUACK_TOPIC_MODEM_CONFIG,
                  sendStatus())

      Log.warning(F("Don't know how to handle command."));
    }

    void sendStatus() {
      rfqRadio->updateRadiosStats();

      // Send rfq.stats
      PB_ENCODE_AND_SEND(rfquack_Status_fields, rfq,
                         RFQUACK_OUT_TOPIC
                           RFQUACK_TOPIC_SEP
                           RFQUACK_TOPIC_STATUS);
    }


};

#endif //RFQUACK_PROJECT_STATSMODULE_H
