#ifndef RFQUACK_PROJECT_HELLOWORLDMODULE_H
#define RFQUACK_PROJECT_HELLOWORLDMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"

extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

class HelloWorldModule : public RFQModule, public OnPacketReceived {
public:
    HelloWorldModule() : RFQModule("hello_world") {}

    void onInit() override {
      // Nothing to do :)
      RFQUACK_LOG_TRACE(F("onInit() fired!"))
    }

    bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      RFQUACK_LOG_TRACE(F("onPacketReceived() fired!"))
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {
      // Execute something
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "cliMethodName", "Description", doSmth(reply))

      // Set a bool
      CMD_MATCHES_BOOL("bool_on_CLI", "Description", cppBool)

      // This will provide set getter / setter for the "enabled" attribute:
      RFQModule::executeUserCommand(verb, args, argsLen, messagePayload, messageLen);
    }

    void doSmth(rfquack_CmdReply &reply) {
      setReplyMessage(reply, F("doSmth started"), 0);
    }

private:
    bool cppBool;

};

#endif //#ifndef RFQUACK_PROJECT_HELLOWORLDMODULE_H

