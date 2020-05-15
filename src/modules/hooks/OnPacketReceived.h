#ifndef RFQUACK_PROJECT_ONPACKETRECEIVED_H
#define RFQUACK_PROJECT_ONPACKETRECEIVED_H

#include "../../rfquack_common.h"

class OnPacketReceived {
public:
    /**
     * Called  - if module is enabled - as soon as the packet is received, before entering the RX Queue,
     * Useful to perform filtering in order to trash packets before they are stored.
     * Useful to perform time-sensitive operations as well as fastly reply to a packet.
     * Note: changes to 'pkt' will persist across modules calls.
     * Note: avoid using 'delays()', try to be **really** as fast as possible.
     * @param pkt received packet
     * @param whichRadio which radio received the packet (RADIOA or RADIOB)
     * @return 'false' will instantly trash the packet, 'true' will pass it to next module.
     */
    virtual bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) = 0;
};

#endif //RFQUACK_PROJECT_ONPACKETRECEIVED_H
