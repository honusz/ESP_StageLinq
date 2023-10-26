#ifndef Directory_h
#define Directory_h
#include "Arduino.h" 
#include "utils.h"
#include "Service.h"
#include "esp_log.h"


typedef std::function<void(svc_msg_t svc)> SvcMsgCallbackFunction;

class Directory: public Service {
public:
	Directory(char * _name, uint8_t *_deviceID, uint16_t _port);
    Directory(char * _name, deviceid_t *_deviceID, uint16_t _port);
    void onServiceMsg(SvcMsgCallbackFunction);
    bool addService(char* name, uint16_t port);
protected:
    void handleData(AsyncClient * client, void * _data, size_t _len) override;
    size_t sendIntroMsg(AsyncClient * _client) override;
    int readCommand(ReadContext *rdr, AsyncClient * client);
    void sendServiceAnnouncement(AsyncClient * client);
    SvcMsgCallbackFunction _cb;
};

#endif