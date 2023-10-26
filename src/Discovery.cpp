#include "Discovery.h"


Action readDiscoAction(char * str) {
  if (strcmp(str, "DISCOVERER_HOWDY_") == 0) return HOWDY;
  if (strcmp(str, "DISCOVERER_EXIT_") == 0) return EXIT;
  return UNKNOWN;
}

Discovery::Discovery(uint16_t _port, deviceid_t _deviceID, char * _unit, char * _software, char * _version) {
  discoMsg.deviceid = _deviceID;
  this->deviceid = _deviceID;
  this->port = _port;
  discoMsg.port = _port;
  strcpy(discoMsg.unit, _unit);
  strcpy(discoMsg.software, _software);
  strcpy(discoMsg.version, _version);
  this->howdy_msg = writeHowdyMsg(&discoMsg, HOWDY, _port);
  this->exit_msg = writeHowdyMsg(&discoMsg, EXIT, _port);
  this->_udp = NULL; 
  this->_udpOut = NULL;
  
  discoBroadcastTimer = xTimerCreate(
      "DiscoBroadcastTimer", /* name */
      pdMS_TO_TICKS(1000), /* period/time */
      pdTRUE, /* auto reload */
      this,//(void*)0, /* timer ID */
      Discovery::s_discoTimerCallback); /* callback */
       
  if (discoBroadcastTimer==NULL) {
    log_e("Broadcast Timer create failed");
  }   
}
Discovery::~Discovery() {
}

size_t Discovery::broadcastDiscoMsg() {
  size_t sent = _udpOut->broadcastTo(*howdy_msg, DISCO_PORT);
  _udpOut->close();
  return sent;
}

void Discovery::s_discoTimerCallback(xTimerHandle pxTimer) {
  (reinterpret_cast<Discovery*>(pvTimerGetTimerID( pxTimer )))->broadcastDiscoMsg();
}

bool Discovery::announce() {
  this->_udpOut = new AsyncUDP;
  log_d("[] Discovery Broadcasting on port %d", DISCO_PORT);
  size_t _sent = broadcastDiscoMsg();
  return (_sent && xTimerStart(discoBroadcastTimer, 0)==pdPASS); 
}

bool Discovery::unannounce() {
  log_d("Discovery Broadcast Unannounce");
  size_t written = _udpOut->broadcastTo(*exit_msg, DISCO_PORT);
  delete _udpOut;
  this->_udpOut = NULL;
  return (xTimerStop(discoBroadcastTimer, 0)==pdPASS && written); 
}

esp_err_t Discovery::listen() {
  this->_udp = new AsyncUDP;
  _udp->onPacket(&Discovery::s_recv, this);
  bool listenResult = _udp->listen(DISCO_PORT);
  if (!listenResult) {
    esp_err_t err = _udp->lastErr();
    log_e("UDP fail %s", esp_err_to_name(err));
    return err;
  } else {
    log_d("UDP Listening on port %i", DISCO_PORT);
    return ESP_OK;
  } 
}


uint16_t Discovery::getPort() {
  return port;
}

deviceid_t * Discovery::getDeviceID() {
  return &this->deviceid;
}


bool Discovery::start() {
  return (announce() && listen() == ESP_OK);
}

bool Discovery::stop() {
  bool unannounceResult = unannounce();
  _udp->close();
  disco_msgs.clear();
  return unannounceResult;
}

void Discovery::reset() {
  xTimerStop(discoBroadcastTimer, 0);
  _udp->close();
  disco_msgs.clear();
}

AsyncUDPMessage * Discovery::writeHowdyMsg(DiscoMsg_t * _disco_msg, Action _action, uint16_t _port) {
  char action[25];
  switch (_action) {
    case HOWDY:
      sprintf(action, D_HOWDY, sizeof(D_HOWDY));
      break;
    case EXIT:
      sprintf(action, D_EXIT, sizeof(D_EXIT));
      break;
    default:
      log_e("invalid action: %i", _action);
      return NULL;
  }
  WriteContext wrt;
  wrt.writeU32(DISCO_MAGIC);
  wrt.writeBytes(_disco_msg->deviceid.data, 16);
  wrt.writeNetworkString(_disco_msg->unit);
  wrt.writeNetworkString(action);
  wrt.writeNetworkString(_disco_msg->software);
  wrt.writeNetworkString(_disco_msg->version);
  wrt.writeU16(_disco_msg->port);
  AsyncUDPMessage * retMsg = new AsyncUDPMessage(wrt.size()+10);
  retMsg->write(wrt.getBuffer(), wrt.size());
  return retMsg;
}

DiscoMsg_t Discovery::_processDiscoMsg(void * _data, size_t _len, IPAddress _remoteIP) {
  ReadContext rdr((uint8_t*)_data, _len);
  uint32_t magic = rdr.readU32();
  DiscoMsg_t msg; 
  char _action[25];
  rdr.readDeviceID(msg.deviceid.data);
  rdr.readNetworkString(msg.unit);
  rdr.readNetworkString(_action);
  rdr.readNetworkString(msg.software);
  rdr.readNetworkString(msg.version);
  msg.port = rdr.readU16();
  msg.address = _remoteIP;
  msg.action = readDiscoAction(_action);
  return msg;
}

// void Discovery::processDiscoMsg(AsyncUDPPacket packet) {
//   ReadContext rdr(packet.data(), packet.length());
//   uint32_t magic = rdr.readU32();
//   DiscoMsg_t msg; 
//   char _action[25];
//   rdr.readDeviceID(msg.deviceid.data);
//   rdr.readNetworkString(msg.unit);
//   rdr.readNetworkString(_action);
//   rdr.readNetworkString(msg.software);
//   rdr.readNetworkString(msg.version);
//   msg.port = rdr.readU16();
//   msg.address = packet.remoteIP();
//   msg.action = readDiscoAction(_action);

//   auto a = disco_msgs.find(msg.deviceid); 
//   if (a == disco_msgs.end()) {
//     auto b = disco_msgs.insert(std::make_pair(msg.deviceid, msg));
    
//     if (_handler) _handler(&b.first->second);
//   } else if (!(a->second.port == msg.port && a->second.address == msg.address )) {
//     a->second = msg;
//     if (_handler) _handler(&a->second);
//   }
// }

DiscoMsg_t* Discovery::handleDiscoMsg(DiscoMsg_t msg) {
  auto b = disco_msgs.insert(std::make_pair(msg.deviceid, msg));
  return (b.second) ? &b.first->second : NULL;
}


bool Discovery::recv(AsyncUDPPacket _packet) {
  auto a = msg_hashes.insert(getHash(_packet.data(), _packet.length()));
  if (!a.second) {
    return 1;
  }
  DiscoMsg_t msg = _processDiscoMsg(_packet.data(), _packet.length(), _packet.remoteIP());
  DiscoMsg_t * msgPtr = handleDiscoMsg(msg);
  if (msgPtr != NULL && _handler) {
    _handler(msgPtr);
  }
  return msgPtr != NULL;
}

void Discovery::s_recv(void * arg, AsyncUDPPacket _packet) {
  // reinterpret_cast<Discovery*>(arg)->processDiscoMsg(_packet);
  reinterpret_cast<Discovery*>(arg)->recv(_packet);
}

void Discovery::onDiscoMsg(DiscoMsgCallbackFunction cb) {
    _handler = cb;
}