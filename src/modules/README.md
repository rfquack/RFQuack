# RFQuack Modules

### Module Example

```c++
    class MyAwesomeModule : public RFQModule {
    public:
        MyAwesomeModule() : RFQModule("AwesomeModuleSlug") {}
    
        void onInit() override {
        	// onInit() is called once when module is loaded.
        }
    
        bool onPacketReceived(rfquack_Packet &pkt, rfquack_WhichRadio whichRadio) override {
           // onPacketReceived() is called when a packet is captured. 
           // This method is called by the driver itself; you should use this method
           // for tasks which should be performed as soon as a packet is received.
           
           // If this method returns 'true' the packet is passed to the next module.
           // If this method returns 'false' the packet is dropped and no further 
           // calls to modules will be performed.
           // If every module returns 'true' then the packet gets stored in memory.
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
          return true;
        }
    
        void executeUserCommand(char *verb, char **args, uint8_t argsLen,
                                char *messagePayload, unsigned int messageLen) override {
                                
          // executeUserCommand() is called when an incoming message from RFQuack's CLI 
          // matches the pattern: "rfquack/in/<set|get|unset>/<AwesomeModuleSlug>/*"
    
    	  // You should use this method to set module's config parameters.
    	  
    	  // Example:
          // Set modem configuration: "rfquack/in/set/AwesomeModuleSlug/ARG0"
          CMD_MATCHES(verb, RFQUACK_TOPIC_SET, args[0], "ARG0",
                      set_modem_config(messagePayload, messageLen))
    
        }
    };
```