#ifndef RFQUACK_PROJECT_PINGMODULE_H
#define RFQUACK_PROJECT_PINGMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"

// RFQuack's shortest module! On ping it will reply pong :)
// This is used for auto-discovery. You are free to add any stats to the "pong" message.
class PingModule : public RFQModule {
public:
    PingModule() : RFQModule("ping") {}

    void onInit() override {
      // Nothing to do :)
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {
      // Ping this dongle
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "ping", "Replies to ping", ping(reply))
    }

    void ping(rfquack_CmdReply &reply) {
      setReplyMessage(reply, F("Pong!"), 0);
    }
};

#endif //#ifndef RFQUACK_PROJECT_PINGMODULE_H

