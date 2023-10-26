#ifndef Service_h
#define Service_h
#include "Arduino.h" 
#include <AsyncTCP.h>
#include <stdio.h>
#include <map>
// #include <iterator>
// #include <functional>
#include "freertos/stream_buffer.h"
#include "freertos/message_buffer.h"
#include "utils.h"
#include "ReadContext.h"
#include "WriteContext.h"

struct task_data_t {
    void * svc;
    AsyncClient * client;  
    task_data_t(AsyncClient * _client, void * _svc): svc(_svc), client(_client){};
};

struct svc_msg_t {
  char name[20];
  deviceid_t deviceid;
  IPAddress address;
  uint16_t port;
} ;

class Service {
    public:
        Service(char * _name, uint8_t *_deviceID, uint16_t _port);
        Service(char * _name, deviceid_t *_deviceID, uint16_t _port);
        ~Service();

        virtual void startServer(bool _set_delay);
        virtual void stopServer();
        virtual bool connect(IPAddress _address, uint16_t _port);
        virtual uint16_t getPort();

        time_stat stat;
        time_stat proc_stat;

    protected:
        char * svcName;
        deviceid_t deviceid;
        uint16_t port;   
        AsyncServer* server;
        bool set_delay = false;
        bool useStreamBuffers = true;
        
        std::map<AsyncClient *, deviceid_t> clientIDs;
        std::map<deviceid_t, AsyncClient *> idClients;
        std::map<AsyncClient *, TaskHandle_t> clientTaskHandles;
        std::map<AsyncClient *, StreamBufferHandle_t> clientStreamBuffers;
        
        size_t send(AsyncClient* client, uint8_t * data, size_t len);
        size_t sendWithLength(AsyncClient* client, uint8_t * data, size_t len);
            
        AsyncClient* getClientFromID(deviceid_t _deviceid);
        deviceid_t * getIDFromClient(AsyncClient* client);
        
        virtual void subscribe(AsyncClient * client);
        virtual void readMessage(ReadContext *rdr, AsyncClient * client); 
        virtual void processBufferedMessage(AsyncClient * client, void * _data, size_t _len);
        virtual size_t sendIntroMsg(AsyncClient * _client);

        virtual size_t handleBufferedMessage(AsyncClient * client, void * _data, size_t _len);
        virtual bool handleClient(AsyncClient * _client);
        virtual void handleConnect(AsyncClient * _client);    
        virtual void handleData(AsyncClient * client, void * _data, size_t _len); 
        virtual void handleError(AsyncClient* client, int8_t error);
        virtual void handleDisconnect(AsyncClient* client);
        virtual void handleTimeOut(AsyncClient* client, uint32_t time);
        
        static void s_handleBufferedMessage(void* arg, AsyncClient* client, void * _data, size_t _len);
        static void s_handleClient(void* arg, AsyncClient* client);
        static void s_handleConnect(void* arg, AsyncClient* client);
        static void s_handleData(void* arg, AsyncClient* client, void * _data, size_t _len);
        static void s_handleError(void* arg, AsyncClient* client, int8_t error);
        static void s_handleDisconnect(void* arg, AsyncClient* client); 
        static void s_handleTimeOut(void* arg, AsyncClient* client, uint32_t time);  
        
        static void s_handleTask(void * pvParams);
};

#endif