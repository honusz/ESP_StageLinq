#ifndef Discovery_h
#define Discovery_h
#include "Arduino.h" 
#include <AsyncUDP.h>
#include "WriteContext.h"
#include "ReadContext.h"
#include "utils.h"
#include <map>
#include "freertos/timers.h"

static const uint32_t DISCO_MAGIC = 0x61697244;
static const uint16_t DISCO_PORT = 51337;
static const char * D_HOWDY = "DISCOVERER_HOWDY_";
static const char * D_EXIT = "DISCOVERER_EXIT_";

enum Action { UNKNOWN, HOWDY, EXIT };

struct DiscoMsg_t {
  deviceid_t deviceid;
  char unit[40];
  char software[40];
  char version[10];
  Action action;
  uint16_t port;
  IPAddress address;
  friend bool operator<(DiscoMsg_t const& lhs, DiscoMsg_t const& rhs) {
    return std::tie(lhs.port, lhs.address) < std::tie(rhs.port, rhs.address);
  }
  // friend bool operator==(DiscoMsg_t const& lhs, DiscoMsg_t const& rhs) {
  //   return std::tie(lhs.port, lhs.address, lhs.deviceid) == std::tie(rhs.port, rhs.address, rhs.deviceid);
  // }
};

Action discoAction(char * str);

typedef std::function<void(DiscoMsg_t *msg)> DiscoMsgCallbackFunction;

class Discovery {
    public:
      Discovery(uint16_t _port, deviceid_t _deviceID, char * _unit, char *  _software, char *  _version);
      ~Discovery();
      
      bool announce();
      bool unannounce();
      esp_err_t listen();

      bool start();
      bool stop();
      void reset();
      void onDiscoMsg(DiscoMsgCallbackFunction cb); 
      uint16_t getPort();
      deviceid_t * getDeviceID();
      
    protected:
      size_t broadcastDiscoMsg();
      AsyncUDPMessage * writeHowdyMsg(DiscoMsg_t * _disco_msg, Action _action, uint16_t _port);
      // void processDiscoMsg(AsyncUDPPacket packet); 
      // void processDiscoMsg(void * _data, size_t _len, IPAddress _remoteIP); 
      // void readDiscoMsg(AsyncUDPPacket packet); 
      DiscoMsg_t * handleDiscoMsg(DiscoMsg_t msg);
      DiscoMsg_t _processDiscoMsg(void * _data, size_t _len, IPAddress _remoteIP); 
      bool recv(AsyncUDPPacket _packet);
      static void s_discoTimerCallback(xTimerHandle pxTimer);
      static void s_recv(void * arg, AsyncUDPPacket _packet);
      
      std::set<unsigned long> msg_hashes;
      uint16_t port;
      deviceid_t deviceid;
      xTimerHandle discoBroadcastTimer;
      AsyncUDP *_udp;
      AsyncUDP *_udpOut;
      AsyncUDPMessage * howdy_msg;
      AsyncUDPMessage * exit_msg; 
      DiscoMsgCallbackFunction _handler;
      
      std::map<deviceid_t, DiscoMsg_t> disco_msgs, cmp_buf;
      DiscoMsg_t discoMsg;
};

#endif