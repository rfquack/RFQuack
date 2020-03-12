# RFQuack Modules

### Module Example

```c++
#include "../RFQModule.h"
#include "../../rfquack_common.h"
#include "../../rfquack_radio.h"

extern RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

class MyAwesomeModule : public RFQModule, public OnPacketReceived, 
                        public AfterPacketReceived, public OnLoop {
public:
    MyAwesomeModule() : RFQModule("AwesomeModuleSlug") {}

    void onInit() override {
        // onInit() is called once, when module is loaded.
        // here you can setup internal variables.
    }

    bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
       // onPacketReceived() is called when a packet is captured. 
       // This method is called by the driver itself; you should use this method
       // for tasks which should be performed as soon as a packet is received.
       
       // If this method returns 'true' the packet is passed to the next module.
       // If this method returns 'false' the packet is dropped and no further 
       // calls to modules will be performed.
       // If every module returns 'true' then the packet gets stored in memory.
 
       // Note: This method is called only if the module is enabled.

       // It you don't plan to use this hook, you can remove this method and stop extending OnPacketReceived
      return true;
    }

    bool afterPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
      // afterPacketReceived() is called when a packet gets popped from RFQuack's
      // internal RX queue. 
      // Here you should perform non-time-sensitive tasks as well as packet 
      // modifications, retransmissions etc.
      
      // If this method returns 'true' the packet is passed to the next module.
      // If this method returns 'false' the packet is dropped and no further 
      // calls to modules will be performed.
      // If every module returns 'true' then the packet will be sent to CLI.

      // Note: This method is called only if the module is enabled.

      // It you don't plan to use this hook, you can remove this method and stop extending AfterPacketReceived
      return true;
    }


    void onLoop() override {
      // onLoop(), as name suggests, is continuously called.
      // Here you can perform logic which does not fit in other hooks.
      // Note: This method is called only if the module is enabled.

      // It you don't plan to use this hook, you can remove this method and stop extending OnLoop
      return true;
    }

    void executeUserCommand(char *verb, char **args, uint8_t argsLen,
                            char *messagePayload, unsigned int messageLen) override {
                            
      // Use macros to handle incoming CLI messages:
      
      // Set this bool from cli using: q.AwesomeModuleSlug.bool1 = True;
      // Get this bool from cli using: q.AwesomeModuleSlug.bool1
      CMD_MATCHES_BOOL("bool1",
                        "Set this bool from cli ",
                        boolExample)
    
      // Same applies to CMD_MATCHES_FLOAT, CMD_MATCHES_INT, CMD_MATCHES_UINT, CMD_MATCHES_WHICHRADIO


      // Call a method from cli with a Void argument:
      // q.AwesomeModuleSlug.start()
      // You'll have access to two variables:
      //        'reply':  Reply to be sent to client
      //        'pkt':    Deserialized argument received from client (rfquack_Void, in this case)
      CMD_MATCHES_METHOD_CALL(rfquack_VoidValue, "start", "Starts something", start(reply))

      // Call a method from cli with a protobuf argument, example with: "rfquack_Register" 
      // q.AwesomeModuleSlug.set(address=2, value=3) # A rfquack_Register will be automatically created and sent.
      CMD_MATCHES_METHOD_CALL(rfquack_Register, "set", "Set something", set(pkt, reply))   
    }

    void start(rfquack_CmdReply &reply) {
      // Do something.
      
      // Optionally with a message and a return code (0)
      setReplyMessage(reply, F("optional message!"), 0);
    }

    void set(rfquack_Register pkt, rfquack_CmdReply &reply) {
      // Use pkt.address;
      // Use pkt.value;
      setReplyMessage(reply, F("Done!"), 0);
    }

private: 
    bool boolExample;
};
```