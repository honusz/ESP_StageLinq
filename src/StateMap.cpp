#include "StateMap.h"


StateMap::StateMap(char * _name, uint8_t *_deviceID, uint16_t _port):Service(_name, _deviceID, _port) {
}

StateMap::StateMap(char * _name, deviceid_t *_deviceID, uint16_t _port):Service(_name, _deviceID, _port) {
}

void StateMap::handleData(AsyncClient * client, void * _data, size_t _len) {
    ReadContext rdr((uint8_t*)_data, _len);
    while (rdr.sizeLeft()) {
        readCommand(&rdr, client);
    }
}

int StateMap::readCommand(ReadContext *rdr, AsyncClient * client) {
    uint32_t cmd = rdr->readU32();
    svc_msg_t svc;
    uint8_t _devID[16];
    rdr->readDeviceID(_devID);
    svc.deviceid = _devID;
    char idString[40];
    
    auto a = clientIDs.find(client);
    if (a == clientIDs.end()) {
        auto c = clientIDs.insert(std::make_pair(client,svc.deviceid));
        auto d = idClients.insert(std::make_pair(svc.deviceid, client));
    }
    
    svc.address = client->remoteIP();
    switch (cmd) {
        case 0u:
            rdr->readNetworkString(svc.name);     
            svc.port = rdr->readU16();
            printDeviceIDTo(svc.deviceid, idString);
            log_d("%s [CMD]%d [%s|%d]", idString, cmd, svc.name, svc.port);
            subscribe(client);
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
            break;
        default:
            break;
    }
    return rdr->sizeLeft();
}

void StateMap::readMessage(ReadContext *rdr, AsyncClient * client) {
    uint32_t magic = rdr->readU32();
    uint32_t action = rdr->readU32();
    
    char key[150];
    char value[200];
    deviceid_t * _deviceid = getIDFromClient(client);

    size_t keyLength = rdr->readNetworkString(key);
    size_t valueLength = rdr->readNetworkString(value);  
    
    auto reg = client_state_registers.find(client);
    if (reg == client_state_registers.end()) {
        log_e("client state reg not found");
        return;
    }
    auto res = reg->second.insert(std::make_pair(key, value)); 
    // if entry exists, update it
    if (!res.second) {
        res.first->second = value;
    } 
    //return stored msg
    if (_cb) {
        _cb(_deviceid, res.first->first, res.first->second);
     
    }
}

void StateMap::subscribe(AsyncClient * client) {
   deviceid_t * _deviceID = getIDFromClient(client);
    state_msg_register msg_reg;
    client_state_registers.insert(std::make_pair(client, msg_reg));
    task_data_t * task_data = new task_data_t(client, this);
    TaskHandle_t task;
    BaseType_t xTask = xTaskCreate(
        s_handleTask,    // The task
        "stateTaskData",  // A friendly "human readable" task name
        5000,             // Stack size (memory allocation)
        task_data,             // Parameters
        4,                // Task priority
        &task             // Task handle               
    );
    client->onData(&StateMap::s_handleBufferedMessage, this);
    if (_readyCB) _readyCB(_deviceID);
}

size_t StateMap::subscribeState(AsyncClient *client, char* state) {
    WriteContext wrt;
    wrt.writeU32(STATE_MAGIC);
    wrt.writeU32(STATE_REQ);
    wrt.writeNetworkString(state);
    wrt.writeU32(STATE_SUB);
    return sendWithLength(client, wrt.getBuffer(), wrt.size());
}

size_t StateMap::subscribeState(AsyncClient *client, char* state, int deckNum) {
    char str[150];
    sprintf(str, state, deckNum);
    size_t sendResult = subscribeState(client, str);
    if (!sendResult) {
        log_e("SubState couldn't send for client %s:%i", client->remoteIP().toString(), client->remotePort());
        return 0;
    }
    return sendResult;
}

size_t StateMap::subscribeState(deviceid_t* _deviceID, char* state) {
    auto a = idClients.find(*_deviceID);
    if (a == idClients.end()) {
        log_e("[size: %i]Can't find client for DeviceID ", idClients.size());
        return 0;
    } 
    return subscribeState(a->second, state);
}

size_t StateMap::subscribeState(deviceid_t* _deviceID, char* state, int deckNum) {
    char str[150];
    sprintf(str, state, deckNum);
    size_t sendResult = subscribeState(_deviceID, str);
    if (!sendResult) {
        log_e("SubState couldn't send for deviceID");
        return 0;
    }
    return sendResult;
}

size_t StateMap::sendIntroMsg(AsyncClient * _client) {
    WriteContext wrt;
    wrt.writeU32(0);
    wrt.writeBytes(deviceid.data, 16);
    wrt.writeNetworkString(svcName);
    wrt.writeU16(port);
    return send(_client, wrt.getBuffer(), wrt.size());
}

void StateMap::onStateMsg(StateMsgCallbackFunction cb)
{
    _cb = cb;
}

void StateMap::onReady(StateMapReadyCallbackFunction readyCB)
{
    _readyCB = readyCB;
}

size_t StateMap::getStateRegSize() {
    size_t totalSize = 0;
    // for (auto i = client_state_registers.begin(); i != client_state_registers.end(); i++) {
    //     // for (auto x = i->second.begin(); x != i->second.end(); x++) {
    //     //     totalSize += x->second;
    //     // }
        
    //     totalSize += sizeof(i->second);
    // }
    return totalSize;    

}