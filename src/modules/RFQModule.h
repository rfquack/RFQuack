#ifndef RFQUACK_PROJECT_RFQMODULE_H
#define RFQUACK_PROJECT_RFQMODULE_H

#include "../rfquack_common.h"

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
    strcpy_P(pkt.argumentType, (PGM_P) F(#pbStruct));\
    strcpy_P(pkt.description, (PGM_P) F(#cmdDescription)); \
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

// Use type (struct): rfquack_BytesValue_value_t to hold targetVariable.
#define CMD_MATCHES_BYTES(cmdValue, description, targetVariable) { \
  _CMD_MATCHES_PRIMITIVE_PB(rfquack_BytesValue, cmdValue, targetVariable, description) \
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
    /**
     * Called - once - when the module is instantiated.
     * Useful to perform one-time setup operations.
    */
    virtual void onInit() = 0;


    /**
     * Called when user sends a command to configure this module.
     * Use predefined macros to match an incoming command:
     *
     *      CMD_MATCHES_BOOL(cmdValue, description, target_bool_Variable)
     *      CMD_MATCHES_UINT(cmdValue, description, target_uint8_t_Variable)
     *      CMD_MATCHES_FLOAT(cmdValue, description, target_float_Variable)
     *      CMD_MATCHES_BYTES(cmdValue, description, target_**_Variable) **
     *      CMD_MATCHES_WHICHRADIO(cmdValue, description, target_rfquack_Whichradio_Variable)
     *          Params:
     *              cmdValue: How the variable will be called on CLI.
     *              description: Textual description that will be sent to CLI.
     *              target_XX_Variable: cpp binded variable.
     *          Example:
     *              CMD_MATCHES_BOOL("enabled", "Enable or disable this module.", _enabled)
     *          CLI Usage:
     *              q.moduleName.enabled = True  # Sets '_enabled' to true on CPP side.
     *              q.moduleName.enabled         # Retrieves '_enabled' from CPP side.
     *
     *      ** CMD_MATCHES_BYTES must be used with variables of type: rfquack_BytesValue_value_t,
     *         this struct holds a 'size' element and a 'bytes' one.
     *
     *      CMD_MATCHES_METHOD_CALL(pbStruct, cmdValue, description, command)
     *          Params:
     *              pbStruct: Protobuf structure passed as argument to method call.
     *              cmdValue: How the method will be called on CLI.
     *              description: Textual description that will be sent to CLI.
     *              command: Target cpp command that will be executed.
     *          Example (1):
     *              CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "start", "Start Roll Jam", start())
     *          CLI Usage (1):
     *              q.moduleName.start()  # start() will be called on the CPP side.
     *
     *          Example (2):
     *              CMD_MATCHES_METHOD_CALL(rfquack_Register, "set_register", "Sets register on underlying modem.",
     *                        set_register(pkt))
     *          CLI Usage(2):
     *              q.moduleName.set_register(address=2, value=3) # set_register(pkt) will be called on CPP side.
     *
     *
     * @param verb GET, SET or UNSET.
     * @param args Array of arguments.
     * @param argsLen  Length of arguments.
     * @param messagePayload Serialized protobuf payload.
     * @param messageLen Length of the protobuf payload.
     */
    virtual void executeUserCommand(char *verb, char **args, uint8_t argsLen,
                                    char *messagePayload, unsigned int messageLen) {

      // Modules should override this method.

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

    void setReplyMessage(rfquack_CmdReply &reply, const __FlashStringHelper *message) {
      reply.has_message = true;
      strncpy_P(reply.message, (PGM_P) message, sizeof(reply.message) - 1);
      reply.message[sizeof(reply.message)] = '\0';
      Log.trace(message);
    }

    void setReplyMessage(rfquack_CmdReply &reply, const char *message) {
      reply.has_message = true;
      strncpy(reply.message, message, sizeof(reply.message) - 1);
      reply.message[sizeof(reply.message)] = '\0';
      Log.trace(message);
    }

    void setReplyMessage(rfquack_CmdReply &reply, const __FlashStringHelper *message, int resultCode) {
      reply.result = resultCode;
      setReplyMessage(reply, message);
    }

    void setReplyMessage(rfquack_CmdReply &reply, char *message, int resultCode) {
      reply.result = resultCode;
      setReplyMessage(reply, message);
    }
};

#endif //RFQUACK_PROJECT_RFQMODULE_H
