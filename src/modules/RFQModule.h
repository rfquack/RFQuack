#ifndef RFQUACK_PROJECT_RFQMODULE_H
#define RFQUACK_PROJECT_RFQMODULE_H

#include "rfquack_common.h"

// Decodes a protobuf payload
#define PB_DECODE(pkt, fields, payload, payload_length) { \
  pb_istream_t istream = pb_istream_from_buffer((uint8_t *) payload, payload_length); \
  if (!pb_decode(&istream, fields, &(pkt))) { \
    Log.error(F("Cannot decode fields: " #fields ", Packet: %s"), PB_GET_ERROR(&istream)); \
    return; \
  } \
}

// rfquack/in/set/<moduleName>/<protobuf_type>/<cmdValue>
// Example: rfquack/in/set/driver/rfquack_FloatValue/frequency
#define _CMD_MATCHES_SET(pbStruct, cmdValue, command) { \
  if (strcmp(verb, RFQUACK_TOPIC_SET) == 0 && (args[0] != NULL && strcmp(args[0], #pbStruct) == 0) \
      && (args[1] != NULL && strcmp(args[1], cmdValue) == 0) ) { \
    pbStruct pkt =  pbStruct ## _init_default ; \
    PB_DECODE(pkt, pbStruct ## _fields, messagePayload, messageLen); \
    rfquack_CmdReply reply = rfquack_CmdReply_init_default; \
    command; \
    PB_ENCODE_AND_SEND(rfquack_CmdReply, reply, RFQUACK_TOPIC_SET, this->name, cmdValue) \
    return; \
  } \
}

// rfquack/in/get/<moduleName>/<cmdValue>
// Example: rfquack/in/get/driver/frequency
#define _CMD_MATCHES_GET(cmdValue, command) { \
  if (strcmp(verb, RFQUACK_TOPIC_GET) == 0 && (args[0] != NULL && strcmp(args[0], cmdValue) == 0)) { \
      command; \
      return; \
  } \
}

// Replies to rfquack/in/info with command info.
#define _DESCRIPTION(cmdValue, cmdDescription, pbStruct, _cmdType) { \
  if (strcmp(verb, RFQUACK_TOPIC_INFO) == 0){ \
    rfquack_CmdInfo pkt = rfquack_CmdInfo_init_default; \
    strcpy(pkt.argumentType, #pbStruct);\
    strcpy(pkt.description, #cmdDescription); \
    pkt.cmdType = _cmdType; \
    RFQUACK_LOG_TRACE(F("Sending " #cmdValue " info to client")); \
    PB_ENCODE_AND_SEND(rfquack_CmdInfo, pkt, RFQUACK_TOPIC_INFO, this->name, cmdValue)  \
  } \
}

// Replies to SET/GET requests.
#define _CMD_MATCHES_PRIMITIVE_PB(pbStruct, cmdValue, targetVariable, cmdDescription) { \
  /* Matches "SET" command: decode the message then calls "onSet" code block. */ \
  _CMD_MATCHES_SET(pbStruct, cmdValue, { \
    targetVariable = pkt.value; \
  }) \
  /* Matches "GET" command: creates a pb message then executes "onGet" code block */ \
  _CMD_MATCHES_GET(cmdValue, { \
    pbStruct pkt = pbStruct ## _init_default ; \
    pkt.value = targetVariable; \
    RFQUACK_LOG_TRACE(F("Sending " #cmdValue " value to client")); \
    PB_ENCODE_AND_SEND(pbStruct, pkt, RFQUACK_TOPIC_GET, this->name, cmdValue) \
  })  \
  /* Send info to the client about current command. */ \
  _DESCRIPTION(cmdValue, cmdDescription, pbStruct, rfquack_CmdInfo_CmdTypeEnum_ATTRIBUTE) \
}

#define CMD_MATCHES_BOOL(cmdValue, description, targetVariable) { \
  _CMD_MATCHES_PRIMITIVE_PB(rfquack_BoolValue, cmdValue, targetVariable, description) \
}

#define CMD_MATCHES_UINT(cmdValue, description, targetVariable) { \
  _CMD_MATCHES_PRIMITIVE_PB(rfquack_UintValue, cmdValue, targetVariable, description) \
}

#define CMD_MATCHES_INT(cmdValue, description, targetVariable) { \
  _CMD_MATCHES_PRIMITIVE_PB(rfquack_IntValue, cmdValue, targetVariable, description) \
}

#define CMD_MATCHES_FLOAT(cmdValue, description, targetVariable) { \
  _CMD_MATCHES_PRIMITIVE_PB(rfquack_FloatValue, cmdValue, targetVariable, description) \
}

#define CMD_MATCHES_WHICHRADIO(cmdValue, description, targetVariable) { \
  _CMD_MATCHES_PRIMITIVE_PB(rfquack_WhichRadioValue, cmdValue, targetVariable, description) \
}

#define CMD_MATCHES_METHOD_CALL(pbStruct, cmdValue, description, command) _CMD_MATCHES_SET(pbStruct, cmdValue, command) _DESCRIPTION(cmdValue, description, pbStruct, rfquack_CmdInfo_CmdTypeEnum_METHOD)

class RFQModule {
public:
    RFQModule(const char *moduleName) {
      this->name = new char[strlen(moduleName) + 1];
      strcpy(this->name, moduleName);
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
    virtual bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) = 0;

    /**
     * Called as soon as a packet is popped from RX QUEUE.
     * Useful to perform non-time-sensitive operations.
     * Note: changes to 'pkt' will persist across modules calls.
     * @param pkt
     * @param whichRadio
     * @return 'false' will instantly trash the packet, 'true' will pass it to next module.
     */
    virtual bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) = 0;

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

      // Enable / Disable module:
      CMD_MATCHES_BOOL("enabled", "Enable or disable this module.", enabled)
    }

    char *getName() { return this->name; }

    bool isEnabled() const {
      return enabled;
    }

protected:
    char *name; // Name of the module.
    bool enabled = false; // Whatever the module is enabled when loaded.
};

#endif //RFQUACK_PROJECT_RFQMODULE_H
