#ifndef RFQUACK_PROJECT_ONLOOP_H
#define RFQUACK_PROJECT_ONLOOP_H

#include "../../rfquack_common.h"

class OnLoop {
public:
    /**
     * Does exactly what the name suggests, loops consecutively when your module is enabled.
     * You can use this to execute logic which does not fit on other hooks.
     */
    virtual void onLoop() = 0;
};

#endif //RFQUACK_PROJECT_ONLOOP_H
