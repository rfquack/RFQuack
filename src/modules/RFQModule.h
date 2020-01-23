#ifndef RFQUACK_PROJECT_RFQMODULE_H
#define RFQUACK_PROJECT_RFQMODULE_H

#include <rfquack_common.h>

#define CMD_MATCHES(verb, verbValue, argx, argxValue, ...) { \
  if (argx != NULL && strcmp(verb, verbValue) == 0 && strcmp(argx, argxValue) == 0) { \
      __VA_ARGS__; \
      return; \
  } \
}

class RFQModule {
public:
    RFQModule(const char *moduleName) {
      uint8_t len = strlen(moduleName) + 1;
      this->name = new char[len];
      memcpy(this->name, moduleName, len);
    }

public:
    virtual void onInit() = 0;

    /**
     * Called as soon as the packet is received, before entering the RX Queue,
     * Useful to perform filtering in order to trash packets before they are stored.
     * Useful to perform time-sensitive operations as well as fastly reply to a packet.
     * Note: changes to 'pkt' will persist across modules calls.
     * @param pkt received packet
     * @param whichRadio which radio received the packet (RADIOA or RADIOB)
     * @return 'false' will instantly trash the packet, 'true' will pass it to next module.
     */
    virtual bool onPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) = 0;

    /**
     * Called as soon as a packet is popped from RX QUEUE.
     * Useful to perform non-time-sensitive operations.
     * Note: changes to 'pkt' will persist across modules calls.
     * @param pkt
     * @param whichRadio
     * @return 'false' will instantly trash the packet, 'true' will pass it to next module.
     */
    virtual bool afterPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) = 0;

    /**
     * Called when user sends a command to configure this module.
     * @param verb GET, SET or UNSET.
     * @param args Array of arguments.
     * @param argsLen  Length of arguments.
     * @param messagePayload Serialized protobuf payload.
     * @param messageLen Length of the protobuf payload.
     */
    virtual void executeUserCommand(char *verb, char **args, uint8_t argsLen,
                                    char *messagePayload, unsigned int messageLen) {

      /* Following commands are common to all modules: */

      // Enable module:  "rfquack/in/set/<moduleName>/enabled"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], "enabled", enabled = true)

      // Disable module:  "rfquack/in/unset/<moduleName>/enabled"
      CMD_MATCHES(verb, RFQUACK_TOPIC_UNSET, args[0], "enabled", enabled = false)
    }

    char *getName() { return this->name; }

    bool isEnabled() const {
      return enabled;
    }

private:
    char *name; // Name of the module.
    bool enabled = true; // Whatever the module is enabled when loaded.
};

#endif //RFQUACK_PROJECT_RFQMODULE_H
