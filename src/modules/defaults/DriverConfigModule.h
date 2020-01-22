#ifndef RFQUACK_PROJECT_DRIVERCONFIGMODULE_H
#define RFQUACK_PROJECT_DRIVERCONFIGMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"
#include "../../rfquack_radio.h"


extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.
extern rfquack_Status rfq; // Status of RFQuack


class DriverConfigModule : public RFQModule {
public:
    DriverConfigModule() : RFQModule("driver") {}

    void onInit() override {
      // Nothing to init.
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

      // Set modem configuration: "rfquack/in/set/driver/modem_config"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], RFQUACK_TOPIC_MODEM_CONFIG,
                  set_modem_config(messagePayload, messageLen))

      // Reset modem configuration: "rfquack/in/unset/driver/modem_config"
      CMD_MATCHES(verb, RFQUACK_TOPIC_UNSET, args[0], RFQUACK_TOPIC_MODEM_CONFIG, reset())

      // Set register value: "rfquack/in/set/driver/register"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], RFQUACK_TOPIC_REGISTER, set_register(messagePayload, messageLen))

      // Get register value: "rfquack/in/get/driver/register"
      CMD_MATCHES(verb, RFQUACK_TOPIC_GET, args[0], RFQUACK_TOPIC_REGISTER, get_register(messagePayload, messageLen))

      // Send packet over the air:  "rfquack/in/set/driver/packet"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], RFQUACK_TOPIC_PACKET, set_packet(messagePayload, messageLen))

      Log.warning(F("Don't know how to handle command."));
    }

    void set_modem_config(char *messagePayload, unsigned int messageLen) {
      rfquack_ModemConfig pkt = rfquack_ModemConfig_init_default;
      PB_DECODE(pkt, rfquack_ModemConfig_fields, messagePayload, messageLen);
      rfqRadio->setModemConfig(pkt);
    }

    void reset() {
      rfqRadio->begin();
    }

    void get_register(char *payload, int payload_length) {
      rfquack_Register pkt;
      PB_DECODE(pkt, rfquack_Register_fields, payload, payload_length);

      // Most of the time, this is just 1 byte (uint8_t)
      rfquack_register_address_t addr = (rfquack_register_address_t) pkt.address;
      rfquack_register_value_t value = rfqRadio->readRegister(addr);

      RFQUACK_LOG_TRACE("Reading register "
                          RFQUACK_REGISTER_HEX_FORMAT
                          " = "
                          RFQUACK_REGISTER_VALUE_HEX_FORMAT,
                        addr, value);

      sendRegister(addr, value);
    }

    void set_register(char *payload, int payload_length) {
      rfquack_Register pkt;
      PB_DECODE(pkt, rfquack_Register_fields, payload, payload_length);

      // most of the time, this is just 1 byte (uint8_t)
      rfquack_register_address_t addr = (rfquack_register_address_t) pkt.address;
      rfquack_register_value_t value = (rfquack_register_value_t) pkt.value;
      rfqRadio->writeRegister(addr, value);
      RFQUACK_LOG_TRACE("Register "
                          RFQUACK_REGISTER_HEX_FORMAT
                          " = "
                          RFQUACK_REGISTER_VALUE_HEX_FORMAT,
                        addr, value)
    }


    /**
     * Trasmit the received packet over the air.
     * @param payload
     * @param payload_length
     */
    void set_packet(char *payload, int payload_length) {
      rfquack_Packet pkt = rfquack_Packet_init_default;
      PB_DECODE(pkt, rfquack_Packet_fields, payload, payload_length);
      rfqRadio->transmit(&pkt);
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

      // Send reg
      PB_ENCODE_AND_SEND(rfquack_Register_fields, reg,
                         RFQUACK_OUT_TOPIC
                           RFQUACK_TOPIC_SEP
                           RFQUACK_TOPIC_REGISTER);
    }


    /*
     * Send statistics about TX/RX packets.
     */
    void sendStats() {
      rfqRadio->updateRadiosStats();

      // Send rfq.stats
      PB_ENCODE_AND_SEND(rfquack_Stats_fields, rfq.stats,
                         RFQUACK_OUT_TOPIC
                           RFQUACK_TOPIC_SEP
                           "driver"
                           RFQUACK_TOPIC_SEP
                           RFQUACK_TOPIC_STATS)

      RFQUACK_LOG_TRACE("TX packets = %d (TX errors = %d), "
                        "RX packets = %d (RX errors = %d)",
                        "TX queue = %d, "
                        "RX queue = %d",
                        rfq.stats.tx_packets, rfq.stats.tx_failures, rfq.stats.rx_packets,
                        rfq.stats.rx_failures, rfq.stats.tx_queue, rfq.stats.rx_queue);
    }
};

#endif //RFQUACK_PROJECT_DRIVERCONFIGMODULE_H
