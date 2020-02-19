#ifndef RFQUACK_PROJECT_PACKETMODIFICATIONMODULE_H
#define RFQUACK_PROJECT_PACKETMODIFICATIONMODULE_H

#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../radio/RadioLibWrapper.h"
#include "../../rfquack.pb.h"
#include "../../rfquack_config.h"

class PacketModificationModule : public RFQModule {
public:
    PacketModificationModule() : RFQModule(RFQUACK_TOPIC_PACKET_MODIFICATION) {}

    void onInit() override {
    }

    bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // apply all packet modifications
      apply_packet_modifications(&pkt);

      return true;
    }

    bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // Packet will be passed to the following module.
      return true;
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen, char *messagePayload,
                            unsigned int messageLen) override {
      // Handle base commands
      RFQModule::executeUserCommand(verb, args, argsLen, messagePayload, messageLen);

      // Add a new packet modification rule:
      CMD_MATCHES_METHOD_CALL(rfquack_PacketModification, "add", "Adds a packet modification rule",
                              add(pkt, reply))

      // Reset all packet modification rules:
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "reset", "Removes all packet modification rules",
                              reset(reply))

      // Dump all packet modification rules:
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "dump", "Dumps all packet modification rules",
                              dump(reply))
    }

    void add(rfquack_PacketModification &pkt, rfquack_CmdReply &reply) {

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

      // Reply to client
      char message[50];
      snprintf(message, sizeof(reply.message), "Rule added, there are %d modification rule(s).", pms.size);
      setReplyMessage(reply, message);
    }

    void reset(rfquack_CmdReply &reply) {
      RFQUACK_LOG_TRACE(F("Packet modification data initialized"))
      pms.size = 0;

      // Reply to client
      setReplyMessage(reply, F("All modifications rules were deleted"));
    }

    void dump(rfquack_CmdReply &reply) {
      RFQUACK_LOG_TRACE(F("Dumping all packet modifications"))
      for (uint8_t i = 0; i < pms.size; i++) {
        PB_ENCODE_AND_SEND(rfquack_PacketModification, pms.rules[i], RFQUACK_TOPIC_GET, this->name, "dump")
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

      // packet prepend / append
      if (rule.has_payload) {
        if ((pkt->data.size + rule.payload.size > RFQUACK_RADIO_MAX_MSG_LEN)) {
          RFQUACK_LOG_TRACE(F("Packet will be truncated since will exceed the packet len after prepend/append."))
        }
        if (rule.operation == rfquack_PacketModification_Op_PREPEND) {
          // Copy the original payload
          rfquack_Packet_data_t data;
          memcpy(&data, &(pkt->data), sizeof(rfquack_Packet_data_t));

          // Re-assemble the packet
          memcpy(&(pkt->data.bytes[0]), rule.payload.bytes, rule.payload.size);
          int bytesToAdd = min(data.size, RFQUACK_RADIO_MAX_MSG_LEN - rule.payload.size);
          memcpy(&(pkt->data.bytes[rule.payload.size]), data.bytes, bytesToAdd);
          pkt->data.size += bytesToAdd;
        }
        if (rule.operation == rfquack_PacketModification_Op_APPEND) {
          // Re-assemble the packet
          int bytesToAdd = min(rule.payload.size, RFQUACK_RADIO_MAX_MSG_LEN - pkt->data.size);
          memcpy(&(pkt->data.bytes[pkt->data.size]), rule.payload.bytes, bytesToAdd);
          pkt->data.size += bytesToAdd;
        }
      }

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
                  // so the compiler is happy and doesn't throw a warning
                  pkt->data.bytes[i] = ~rule.operand;
                  break;
              }
            else {
              // packet[position] = ~packet[position]
              if (rule.operation == rfquack_PacketModification_Op_NOT) {
                pkt->data.bytes[i] = ~pkt->data.bytes[i];
              }

              // packet = packet[0 : position] + payload + packet[position : packet.size]
              if (rule.operation == rfquack_PacketModification_Op_INSERT) {
                if ((pkt->data.size + rule.payload.size > RFQUACK_RADIO_MAX_MSG_LEN)) {
                  RFQUACK_LOG_TRACE(F("Unable to insert payload, message will exceed allowed len."))
                  return;
                }
                RFQUACK_LOG_TRACE(F("Inserting payload in position %d"), i)
                // Copy the original payload
                rfquack_Packet_data_t data;
                memcpy(&data, &(pkt->data), sizeof(rfquack_Packet_data_t));

                // Keep data up to "i".

                // Then append payload:
                memcpy(&(pkt->data.bytes[i]), rule.payload.bytes, rule.payload.size);

                // Then append remaining data (if any)
                if ((data.size - i) > 0)
                  memcpy(&(pkt->data.bytes[rule.payload.size + i]), &(data.bytes[i]), (data.size - i));

                // Update len
                pkt->data.size += rule.payload.size;
              }
            }
          }
        }

      }
#ifdef RFQUACK_DEV
      rfquack_log_packet(pkt);
#endif
    }

private:
    uint8_t min(uint8_t a, uint8_t b) {
      if (a < b) return a;
      return b;
    }

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

    packet_modifications_t pms;
};

#endif //RFQUACK_PROJECT_PACKETMODIFICATIONMODULE_H
