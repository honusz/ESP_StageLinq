#include "Directory.h"

Directory::Directory(char * _name, uint8_t *_deviceID, uint16_t _port):Service(_name, _deviceID, _port) {    
    this->useStreamBuffers = false;
}

Directory::Directory(char * _name, deviceid_t *_deviceID, uint16_t _port):Service(_name, _deviceID, _port) {    
    this->useStreamBuffers = false;
}

std::map<char *, uint16_t, cmp_str> service_ports;

void Directory::handleData(AsyncClient * client, void * _data, size_t _len) {
    ReadContext rdr((uint8_t*)_data, _len);
    while (rdr.sizeLeft()) {
        readCommand(&rdr, client);
    }
}

int Directory::readCommand(ReadContext *rdr, AsyncClient * client) {
    uint32_t cmd = rdr->readU32();
    svc_msg_t svc;
    uint8_t _devID[16];
    rdr->readDeviceID(_devID);
    svc.deviceid = _devID;
    char idString[40];
    svc.address = client->remoteIP();
    switch (cmd) {
        case 0u:
            rdr->readNetworkString(svc.name);     
            svc.port = rdr->readU16();
            printDeviceIDTo(svc.deviceid, idString);
            log_d("%s [CMD]%d [%s|%d]", idString, cmd, svc.name, svc.port);
            if (_cb) {
                _cb(svc);
            }
            break;
        case 1u: 
            int length; 
            length = rdr->sizeLeft();
            uint8_t incDeviceID[16];
            uint8_t incBytes[8];
            rdr->readDeviceID(incDeviceID);
            rdr->readBytes(incBytes, 8);
            break;
        case 2u:
            printDeviceIDTo(svc.deviceid, idString);
            log_d("%s [CMD]%d", idString, cmd);
            sendServiceAnnouncement(client);
            if (_cb) {
                _cb(svc);
            }
            break;
        default:
            break;
    }
    return rdr->sizeLeft();
}


size_t Directory::sendIntroMsg(AsyncClient * _client) {
    WriteContext wrt;
    wrt.writeU32(2);
    wrt.writeBytes(deviceid.data, 16);
    return send(_client, wrt.getBuffer(), wrt.size());
}

void Directory::sendServiceAnnouncement(AsyncClient * client) {
    WriteContext wrt;
    wrt.writeU32(2);
    wrt.writeBytes(deviceid.data, 16);
    for (auto const& x : service_ports) {
        wrt.writeU32(0u);
        wrt.writeBytes(this->deviceid.data,16);
        wrt.writeNetworkString(x.first);
        wrt.writeU16(x.second);
    }
    send(client, wrt.getBuffer(), wrt.size());
}

bool Directory::addService(char* name, uint16_t port) {
     auto a = service_ports.insert(std::make_pair(name, port));
     return (a.second);
}

void Directory::onServiceMsg(SvcMsgCallbackFunction cb)
{
    _cb = cb;
}

