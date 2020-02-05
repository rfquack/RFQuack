#ifndef RFQUACK_PROJECT_PACKETFILTERMODULE_H
#define RFQUACK_PROJECT_PACKETFILTERMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"

class PacketFilterModule : public RFQModule {
public:
    PacketFilterModule() : RFQModule(RFQUACK_TOPIC_PACKET_FILTER) {}

    virtual void onInit() {
    }

    virtual bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) {
      // Check the packet against loaded filters. Will go to next module only if matches all rules.
      return matchesAllRules(&pkt);
    }

    bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // Packet will be passed to the following module.
      return true;
    }

    virtual void
    executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload, unsigned int messageLen) {
      // Handle base commands
      RFQModule::executeUserCommand(verb, args, argsLen, messagePayload, messageLen);

      // Add a new packet filter:
      CMD_MATCHES_METHOD_CALL(rfquack_PacketFilter, "add", "Adds a packet filtering rule",
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
      reply.has_message = true;
      snprintf(reply.message, sizeof(reply.message), "Rule added, there are %d filtering rule(s).", pfs.size);
    }

    void reset(rfquack_CmdReply &reply) {
      RFQUACK_LOG_TRACE(F("Packet filters data initialized"))
      pfs.size = 0;

      // Reply for client
      reply.has_message = true;
      strcpy(reply.message, "All rules were deleted");
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
    bool matchesAllRules(rfquack_Packet *pkt) {
      if (pfs.size == 0) {
        RFQUACK_LOG_TRACE(F("No filters"));
        return true;
      }

      for (uint8_t i = 0; i < pfs.size; i++)
        if (!rfquack_packet_matches(pfs.filters[i].pattern, pkt)) {
          //
          // TODO understand why matching against a pre-compiled pattern fails
          // if (!rfquack_packet_matchesP(pfs.patterns[i], pkt)) {
          //

#ifdef RFQUACK_DEV
          rfquack_log_packet(pkt);
          Log.trace("Packet doesn't match pattern '%s'", pfs.filters[i].pattern);
#endif

          return false;
        }

#ifdef RFQUACK_DEV
      rfquack_log_packet(pkt);
      Log.verbose("Packet matches all %d patterns", pfs.size);
#endif

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
