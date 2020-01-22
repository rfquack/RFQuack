#ifndef RFQUACK_PROJECT_PACKETMODIFICATIONMODULE_H
#define RFQUACK_PROJECT_PACKETMODIFICATIONMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"


/**
 * @brief Array of packet modification rules along with compiled patterns
 */
typedef struct packet_modifications {
    /**
     * @brief set of packet modifications
     */
    rfquack_PacketModification rules[RFQUACK_MAX_PACKET_MODIFICATIONS];

    /**
     * @brief Pre-compiled patterns, one per rule
     */
    re_t patterns[RFQUACK_MAX_PACKET_MODIFICATIONS];

    /**
     * @brief Number of usable rules
     */
    uint8_t size = 0;
} packet_modifications_t;

/**
 * @TODO ADD CONFIGS:
 *   - How many repeat
 *   - Which radio to use to repeat
 *   - Whatever to stop or continue the chain after packet is repeated.
 */
class PacketModificationModule : public RFQModule {
public:
    PacketModificationModule() : RFQModule(RFQUACK_TOPIC_PACKET_MODIFICATION) {}

    void onInit() override {
      reset_packet_modification();
    }

    bool onPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) override {
      // apply all packet modifications
      apply_packet_modifications(&pkt);

      pkt.has_repeat = true;
      // Todo retrieve from module prop
      pkt.repeat = rfq.tx_repeat_default;

      // send packet
      rfqRadio->transmit(&pkt, whichRadio);

      // Return 'true', packet will be passed to subsequent modules.
      return true;
    }

    bool afterPacketReceived(rfquack_Packet &pkt, WhichRadio whichRadio) override {
      // Packet will be passed to the following module.
      return true;
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {
      // Handle base commands
      RFQModule::executeUserCommand(verb, args, argsLen, messagePayload, messageLen);

      // Add a new packet modification: "rfquack/in/set/packet_modification/rules"
      CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], RFQUACK_TOPIC_RULES,
                  add_packet_modification(messagePayload, messageLen))

      // Reset all packet modifications: "rfquack/in/unset/packet_modification/rules"
      CMD_MATCHES(verb, RFQUACK_TOPIC_UNSET, args[0], RFQUACK_TOPIC_RULES, reset_packet_modification())

      // Dump all packet modifications: "rfquack/in/get/packet_modification/rules"
      CMD_MATCHES(verb, RFQUACK_TOPIC_GET, args[0], RFQUACK_TOPIC_RULES, get_packet_modification())

      Log.warning(F("Don't know how to handle command."));
    }

    void add_packet_modification(char *payload, int payload_length) {
      rfquack_PacketModification pkt;
      PB_DECODE(pkt, rfquack_PacketModification_fields, payload, payload_length);

      int idx = pms.size;
      pms.size++;

      if (pkt.has_pattern) {
        // compile the pattern
        re_t cp = re_compile(pkt.pattern);

        // add pattern to ruleset
        memcpy(&(pms.patterns[idx]), &cp, sizeof(re_t));
      }

      // add rule to ruleset
      memcpy(&(pms.rules[idx]), &pkt, sizeof(rfquack_PacketModification));
      RFQUACK_LOG_TRACE(F("Added new packet modification"))
    }

    void reset_packet_modification() {
      RFQUACK_LOG_TRACE(F("Packet modification data initialized"))
      pms.size = 0;
    }

    void get_packet_modification() {
      RFQUACK_LOG_TRACE(F("Dumping all packet modifications"))
      for (uint8_t i = 0; i < pms.size; i++) {
        PB_ENCODE_AND_SEND(rfquack_PacketModification_fields, pms.rules[i],
                           RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP
                             RFQUACK_TOPIC_PACKET_MODIFICATION)
      }
    }


    /**
     * @brief Apply packet modifications to a packet
     *
     * We're not responsible for weird packet modification combos: we'll just apply
     * them. So, write those wisely 😉
     */
    void apply_packet_modifications(rfquack_Packet *pkt) {
      // for each packet modification rule in positional order
      for (uint8_t i = 0; i < pms.size; i++) {
        apply_packet_modification(i, pkt);
      }
    }

    void apply_packet_modification(uint8_t idx, rfquack_Packet *pkt) {

      rfquack_PacketModification rule = pms.rules[idx];

      // TODO understand why matching against a pre-compiled pattern fails
      // re_t cp = pms.patterns[idx];
      //
      // if (rule.has_pattern && !rfquack_packet_matches_p(cp, pkt)) {
      //
      if (rule.has_pattern && !rfquack_packet_matches(rule.pattern, pkt)) {
        return;
      }

      // either packet matches the signature or there's no signature (*)

      RFQUACK_LOG_TRACE(F("Applying packet modification rule #%d"), idx);

#ifdef RFQUACK_DEV
      rfquack_log_packet(pkt);
#endif

      // for all octects
      for (uint32_t i = 0; i < pkt->data.size; i++) {
        uint8_t octect = pkt->data.bytes[i];

        if (
          // Case 1: we have a position only, and it matches
          (!rule.has_content && rule.has_position && rule.position == i) ||

          // Case 2: we have both position and value, and both match
          (rule.has_content && rule.has_position && rule.content == octect &&
           rule.position == i) ||

          // Case 3: we have only value, and matches
          (rule.has_content && !rule.has_position && rule.content == octect)) {

          // Value assignment
          if (!rule.has_operation && rule.has_operand) {
            pkt->data.bytes[i] = rule.operand;

          } else {
            if (rule.has_operand)
              switch (rule.operation) {
                // packet[position] = packet[position] & operand
                case rfquack_PacketModification_Op_AND:
                  pkt->data.bytes[i] &= rule.operand;
                  break;

                  // packet[position] = packet[position] | operand
                case rfquack_PacketModification_Op_OR:
                  pkt->data.bytes[i] |= rule.operand;
                  break;

                  // packet[position] = packet[position] ^ operand
                case rfquack_PacketModification_Op_XOR:
                  pkt->data.bytes[i] ^= rule.operand;
                  break;

                  // packet[position] = packet[position] << operand
                case rfquack_PacketModification_Op_SLEFT:
                  pkt->data.bytes[i] <<= rule.operand;
                  break;

                  // packet[position] = packet[position] >> operand
                case rfquack_PacketModification_Op_SRIGHT:
                  pkt->data.bytes[i] >>= rule.operand;
                  break;

                case rfquack_PacketModification_Op_NOT:
                  // doesn't make a lot of sense, but let's handle this case
                  // so the compiler is hapy and doesn't throw a warning
                  pkt->data.bytes[i] = ~rule.operand;
                  break;
              }
            else
              // packet[position] = ~packet[position]
            if (rule.operation == rfquack_PacketModification_Op_NOT)
              pkt->data.bytes[i] = ~pkt->data.bytes[i];
          }
        }

      }
#ifdef RFQUACK_DEV
      rfquack_log_packet(pkt);
#endif
    }

private:
    packet_modifications_t pms;

};

#endif //RFQUACK_PROJECT_PACKETMODIFICATIONMODULE_H
