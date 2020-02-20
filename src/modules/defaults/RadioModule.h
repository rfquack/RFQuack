#ifndef RFQUACK_PROJECT_RADIOMODULE_H
#define RFQUACK_PROJECT_RADIOMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"

extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

class RadioModule : public RFQModule, public AfterPacketReceived {
public:
    RadioModule(const char *moduleName, rfquack_WhichRadio whichRadio) : RFQModule(moduleName) {
      _whichRadio = whichRadio;
    }

    void onInit() override {
      this->enabled = true;
    }


    bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // Send to transport all packets received from the radio controlled by this module.
      if (sendToTransport && pkt.rxRadio == _whichRadio) {
        PB_ENCODE_AND_SEND(rfquack_Packet, pkt, RFQUACK_TOPIC_GET, this->name, "packet")
      }

      // Packet will be passed to the transport module, even if this module is - usually - the last one.
      return true;
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {

      // Set modem configuration:
      CMD_MATCHES_METHOD_CALL(rfquack_ModemConfig, "set_modem_config", "Apply configuration to modem.",
                              set_modem_config(pkt, reply))

      // Set packet len:
      CMD_MATCHES_METHOD_CALL(rfquack_PacketLen, "set_packet_len", "Set packet length configuration (fixed/variable).",
                              set_packet_len(pkt, reply))

      // Set register value:
      CMD_MATCHES_METHOD_CALL(rfquack_Register, "set_register", "Sets register on underlying modem.",
                              set_register(pkt, reply))

      // Get register value:
      CMD_MATCHES_METHOD_CALL(rfquack_UintValue, "get_register", "Retrieve register value from underlying modem.",
                              get_register(pkt, reply))

      // Send received packets to transport:
      CMD_MATCHES_BOOL("send_to_transport", "Whatever to send received packets to transport",
                       sendToTransport)

      // Send packet over the air:
      CMD_MATCHES_METHOD_CALL(rfquack_Packet, "send", "Send a packet over the air",
                              {
                                rfquack_log_packet(&pkt);
                                reply.result = rfqRadio->transmit(&pkt, _whichRadio);
                              })

      // RX Mode
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "rx", "Puts modem in RX mode",
                              reply.result = rfqRadio->setMode(rfquack_Mode_RX, _whichRadio))
      // TX Mode
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "tx", "Puts modem in TX mode",
                              reply.result = rfqRadio->setMode(rfquack_Mode_TX, _whichRadio))
      // Idle Mode
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "idle", "Puts modem in IDLE mode",
                              reply.result = rfqRadio->setMode(rfquack_Mode_IDLE, _whichRadio))
    }

    void set_modem_config(rfquack_ModemConfig pkt, rfquack_CmdReply &reply) {
      uint8_t changes = 0;
      uint8_t failures = 0;
      rfqRadio->setModemConfig(pkt, _whichRadio, changes, failures);

      if (failures > 0) reply.result = ERR_UNKNOWN;
      reply.has_message = true;
      snprintf(reply.message, sizeof(reply.message), "%d changes applied and %d failed.", changes, failures);
    }

    void set_packet_len(rfquack_PacketLen pkt, rfquack_CmdReply &reply) {
      reply.result = rfqRadio->setPacketLen(pkt, _whichRadio);
    }

    void get_register(rfquack_UintValue pkt, rfquack_CmdReply &reply) {
      // Most of the time, this is just 1 byte (uint8_t)
      rfquack_register_address_t addr = (rfquack_register_address_t) pkt.value;
      rfquack_register_value_t value = rfqRadio->readRegister(addr, _whichRadio);

      RFQUACK_LOG_TRACE("Reading register "
                          RFQUACK_REGISTER_HEX_FORMAT
                          " = "
                          RFQUACK_REGISTER_VALUE_HEX_FORMAT,
                        addr, value);

      sendRegister(addr, value);
    }

    void set_register(rfquack_Register pkt, rfquack_CmdReply &reply) {
      // most of the time, this is just 1 byte (uint8_t)
      rfquack_register_address_t addr = (rfquack_register_address_t) pkt.address;
      rfquack_register_value_t value = (rfquack_register_value_t) pkt.value;
      rfqRadio->writeRegister(addr, value, _whichRadio);
      RFQUACK_LOG_TRACE("Register "
                          RFQUACK_REGISTER_HEX_FORMAT
                          " = "
                          RFQUACK_REGISTER_VALUE_HEX_FORMAT,
                        addr, value)

      reply.result = ERR_NONE;
      PB_ENCODE_AND_SEND(rfquack_CmdReply, reply, RFQUACK_TOPIC_GET, this->name, "set_register")
    }


    /**
     * @brief Send register value to the transport.
     *
     * @param addr Address of the register
     * @param value Value of the register
     */
    void sendRegister(rfquack_register_address_t addr, rfquack_register_address_t value) {
      rfquack_Register reg;
      reg.address = addr;
      reg.value = value;
      reg.has_value = true;

      PB_ENCODE_AND_SEND(rfquack_Register, reg, RFQUACK_TOPIC_GET, this->name, "get_register")
    }

private:
    rfquack_WhichRadio _whichRadio;
    bool sendToTransport = true;
};

#endif //RFQUACK_PROJECT_RADIOMODULE_H
