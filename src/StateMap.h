#ifndef StateMap_h
#define StateMap_h
#include "Arduino.h" 
#include "utils.h"
#include "Service.h"
#include "States.h"

static const uint32_t STATE_MAGIC = 0x736d6161;
static const uint32_t STATE_SUB = 0x0000000a;
static const uint32_t STATE_REQ = 0x000007d2;

struct state_msg_t {
    uint8_t * deviceID;
    char *key;//[150];
    char *value;//[200];
    state_msg_t() {};
    state_msg_t(uint8_t* _deviceID, char* _key, char* _value): deviceID(_deviceID), key(_key), value(_value) {};
    ~state_msg_t(){
    };
};

struct state_msg_counter_t {
    int sent;
    int recv;
};

typedef std::function<void(deviceid_t * _deviceid, char * key, char *value)> StateMsgCallbackFunction;
typedef std::function<void(deviceid_t * deviceID)> StateMapReadyCallbackFunction;
typedef std::map<char*, char*, cmp_str> state_msg_register;


class StateMap: public Service {
public:
	StateMap(char * _name, uint8_t *_deviceID, uint16_t _port);
    StateMap(char * _name, deviceid_t *_deviceID, uint16_t _port);
    
    size_t subscribeState(AsyncClient *client, char* state);
    size_t subscribeState(AsyncClient *client, char* state, int deckNum);
    size_t subscribeState(deviceid_t* deviceID, char* state);
    size_t subscribeState(deviceid_t* deviceID, char* state, int deckNum);
    
    void onStateMsg(StateMsgCallbackFunction);
    void onReady(StateMapReadyCallbackFunction);

    size_t getStateRegSize();
   
protected:   
    std::map<AsyncClient*, state_msg_counter_t> state_msg_counter;
    std::map<AsyncClient*, state_msg_register> client_state_registers;

    void subscribe(AsyncClient * client) override;
    void handleData(AsyncClient * client, void * _data, size_t _len) override;
    size_t sendIntroMsg(AsyncClient * _client) override;
    int readCommand(ReadContext *rdr, AsyncClient * client);
    void readMessage(ReadContext *rdr, AsyncClient * client) override;

    StateMsgCallbackFunction _cb;
    StateMapReadyCallbackFunction _readyCB;
};

#endif