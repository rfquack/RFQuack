#ifndef RFQUACK_PROJECT_PACKETFILTERMODULE_H
#define RFQUACK_PROJECT_PACKETFILTERMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"

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

class PacketFilterModule : public RFQModule {
public:
    PacketFilterModule() : RFQModule(RFQUACK_TOPIC_PACKET_FILTER) {}

    virtual void onInit() {
      reset_packet_filter();
    }

    virtual bool onPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) {
      // Check the packet against loaded filters.
      return matchesAllRules(&pkt);
    }

    bool afterPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) override {
      // Packet will be passed to the following module.
      return true;
    }

    virtual void
    executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload, unsigned int messageLen) {
      RFQModule::executeUserCommand(verb, args, argsLen, messagePayload, messageLen);

      // Add a new packet filter: "rfquack/in/set/packet_filter/rules"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], RFQUACK_TOPIC_RULES,
                  add_packet_filter(messagePayload, messageLen))

      // Reset all packet filter: "rfquack/in/unset/packet_filter/rules"
      CMD_MATCHES(verb, RFQUACK_TOPIC_UNSET, args[0], RFQUACK_TOPIC_RULES, reset_packet_filter())

      // Dump all packet filter: "rfquack/in/get/packet_filter/rules"
      CMD_MATCHES(verb, RFQUACK_TOPIC_GET, args[0], RFQUACK_TOPIC_RULES, get_packet_filters())

      Log.warning(F("Don't know how to handle command."));
    }

    void add_packet_filter(char *payload, int payload_length) {
      rfquack_PacketFilter pkt;
      PB_DECODE(pkt, rfquack_PacketFilter_fields, payload, payload_length);

      int idx = pfs.size;
      pfs.size++;

      // compile the pattern
      re_t cp = re_compile(pkt.pattern);

      // add pattern to ruleset
      memcpy(&(pfs.patterns[idx]), &cp, sizeof(re_t));

      // add rule to ruleset
      memcpy(&(pfs.filters[idx]), &pkt, sizeof(rfquack_PacketFilter));
      RFQUACK_LOG_TRACE(F("Added pattern %s to filters."), pfs.filters[idx].pattern);
    }

    void reset_packet_filter() {
      RFQUACK_LOG_TRACE(F("Packet filters data initialized"))
      pfs.size = 0;
    }

    /**
     * @brief Loop through all packet filters and send them
     * to the nodes.
     */
    void get_packet_filters() {
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
      PB_ENCODE_AND_SEND(rfquack_PacketFilter_fields, pfs.filters[index],
                         RFQUACK_OUT_TOPIC
                           RFQUACK_TOPIC_SEP
                           RFQUACK_TOPIC_PACKET_FILTER);
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
    packet_filters_t pfs;
};

#endif //RFQUACK_PROJECT_PACKETFILTERMODULE_H
