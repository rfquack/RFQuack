#ifndef RFQUACK_PROJECT_PACKETFILTERMODULE_H
#define RFQUACK_PROJECT_PACKETFILTERMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"

extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

class PacketFilterModule : public RFQModule, public OnPacketReceived {
public:
    PacketFilterModule() : RFQModule(RFQUACK_TOPIC_PACKET_FILTER) {}

    void onInit() override {
      // Nothing to do :)
    }

    bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // Check the packet against loaded filters. Will go to next module only if matches all rules.
      return isAllowedByRules(&pkt);
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {
      // Handle base commands
      RFQModule::executeUserCommand(verb, args, argsLen, messagePayload, messageLen);

      // Add a new packet filter:
      CMD_MATCHES_METHOD_CALL(rfquack_PacketFilter, "add", "Adds a packet filtering rule on HEX values",
                              add(pkt, reply))

      // Reset all packet filter:
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "reset", "Removes all packet filtering rules",
                              reset(reply))

      // Dump all packet filter:
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "dump", "Dumps all packet filtering rules",
                              dump(reply))
    }

    void add(rfquack_PacketFilter &pkt, rfquack_CmdReply &reply) {
      int idx = pfs.size;
      pfs.size++;

      // compile the pattern
      re_t cp = re_compile(pkt.pattern);

      // add pattern to ruleset
      memcpy(&(pfs.patterns[idx]), &cp, sizeof(re_t));

      // add rule to ruleset
      memcpy(&(pfs.filters[idx]), &pkt, sizeof(rfquack_PacketFilter));
      RFQUACK_LOG_TRACE(F("Added pattern %s to filters."), pfs.filters[idx].pattern);

      // Reply for client
      char message[50];
      snprintf(message, sizeof(reply.message), "Rule added, there are %d filtering rule(s).", pfs.size);
      setReplyMessage(reply, message);
    }

    void reset(rfquack_CmdReply &reply) {
      RFQUACK_LOG_TRACE(F("Packet filters data initialized"))
      pfs.size = 0;

      // Reply for client
      setReplyMessage(reply, F("All rules were deleted"));
    }

    /**
     * @brief Loop through all packet filters and send them
     * to the nodes.
     */
    void dump(rfquack_CmdReply &reply) {
      // Send dumps
      RFQUACK_LOG_TRACE(F("Dumping all packet filters"))
      for (uint8_t i = 0; i < pfs.size; i++)
        sendPacketFilter(i);
    }

    /**
     * @brief Send a packet filter on the transport
     *
     * @param index Position in the list of packet filter list
     */
    void sendPacketFilter(uint8_t index) {
      PB_ENCODE_AND_SEND(rfquack_PacketFilter, pfs.filters[index], RFQUACK_TOPIC_GET, this->name, "dump")
    }

    /**
    * @brief Checks if a packet matches all the filters
    *
    * @param pkt Pointer to packet
    *
    * @return Whether the packet matches all the filters
    */
    bool isAllowedByRules(rfquack_Packet *pkt) {
      if (pfs.size == 0) {
        RFQUACK_LOG_TRACE(F("No filters"));
        return true;
      }

      for (uint8_t i = 0; i < pfs.size; i++) {
        rfquack_PacketFilter packetFilterRule = pfs.filters[i];
        bool matches = rfquack_packet_matches(packetFilterRule.pattern, pkt);

        if (packetFilterRule.negateRule)
          matches = !matches;

        if (!matches)
          return false;
      }
      return true;
    }


private:

/**
 * @brief Array of packet-filtering rules along with compiled patterns
 */
    typedef struct packet_filters {
        /**
         * @brief set of packet filters
         */
        rfquack_PacketFilter filters[RFQUACK_MAX_PACKET_FILTERS];

        /**
         * @brief Pre-compiled patterns, one per rule
         */
        re_t patterns[RFQUACK_MAX_PACKET_FILTERS];

        /**
         * @brief Number of usable rules
         */
        uint8_t size = 0;
    } packet_filters_t;

    packet_filters_t pfs;
};

#endif //RFQUACK_PROJECT_PACKETFILTERMODULE_H
