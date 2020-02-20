#ifndef RFQUACK_PROJECT_AFTERPACKETRECEIVED_H
#define RFQUACK_PROJECT_AFTERPACKETRECEIVED_H

#include "../../rfquack_common.h"

class AfterPacketReceived {
public:

    /**
     * Called - if module is enabled - as soon as a packet is popped from RX QUEUE.
     * Useful to perform non-time-sensitive operations.
     * Note: changes to 'pkt' will persist across modules calls.
     * Note: avoid using 'delays()'
     * @param pkt
     * @param whichRadio
     * @return 'false' will instantly trash the packet, 'true' will pass it to next module.
     */
    virtual bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) = 0;

};

#endif //RFQUACK_PROJECT_AFTERPACKETRECEIVED_H
