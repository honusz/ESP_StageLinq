#include "BeatInfo.h"

static std::map<deviceid_t, beat_msg_t> msgReg;

BeatInfo::BeatInfo(char * _name, uint8_t *_deviceID, uint16_t _port):Service(_name, _deviceID, _port) {    
}

BeatInfo::BeatInfo(char * _name, deviceid_t *_deviceID, uint16_t _port):Service(_name, _deviceID, _port) {    
}

void BeatInfo::handleData(AsyncClient * client, void * _data, size_t _len) {
    log_buf_d((uint8_t*)_data, _len);
    ReadContext rdr((uint8_t*)_data, _len);
    while (rdr.sizeLeft()) {
        readCommand(&rdr, client);
    }
}

int BeatInfo::readCommand(ReadContext *rdr, AsyncClient * client) {
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
            log_d("%s [CMD]%d [%s|%d]", idString, cmd, svc.name, svc.port);
            break;
        default:
            break;
    }
    return rdr->sizeLeft();
}

void BeatInfo::s_handleBeatMessage(void* arg, AsyncClient* client, void * _data, size_t _len) {
    (reinterpret_cast<BeatInfo*>(arg))->handleBeatMsg(client, _data, _len);
}

void BeatInfo::handleBeatMsg(AsyncClient * client, void * _data, size_t _len) {
    ReadContext rdr((uint8_t*)_data, _len);
    while (rdr.sizeLeft()) {
        processBeatMsg(&rdr, client);
    }
}



void BeatInfo::subscribe(AsyncClient * client) {
    deviceid_t * _deviceID = getIDFromClient(client);
    task_data_t * task_data = new task_data_t(client, this);
    TaskHandle_t task;
    auto a = clientTaskHandles.insert(std::make_pair(client, task));
    
    BaseType_t xTask = xTaskCreate(
        s_handleTask,    // The task
        "beatTaskData",  // A friendly "human readable" task name
        5000,             // Stack size (memory allocation)
        task_data,             // Parameters
        4,                // Task priority
        &task             // Task handle               
    );
    client->onData(&BeatInfo::s_handleBufferedMessage, this);
    // sendBeatInfoStartMsg(client);
    if (readyCB) readyCB(_deviceID);
}

void BeatInfo::readMessage(ReadContext *rdr, AsyncClient * client) {
    proc_stat.trigger(1.0);
    beat_msg_t msg;
    msg.deviceid = getIDFromClient(client);
    int id = rdr->readU32();
    msg.clock = rdr->readU64();
    msg.deckCount = rdr->readU32();
    for (int i=0; i<msg.deckCount; i++) {
        msg.deck[i].beat = rdr->readDouble();
        msg.deck[i].totalBeats = rdr->readDouble();
        msg.deck[i].BPM = rdr->readDouble();
    }
    for (int i=0; i<msg.deckCount; i++) msg.deck[i].samples = rdr->readDouble();
    auto prvMsg = msgReg.find(msg.deviceid);
    if (prvMsg != msgReg.end()) {
        bool hasChanged = false;
        for (int i =0; i<msg.deckCount; i++) hasChanged = hasChanged || !(prvMsg->second.deck[i] == msg.deck[i]);  
        if (!hasChanged) {
        
        } else {    
            prvMsg->second = msg;
            if (_cb) _cb(&prvMsg->second);
        }
    } else {
        auto newMsg = msgReg.insert(std::make_pair(msg.deviceid, msg));
        if (_cb) _cb(&newMsg.first->second);
    }
}

bool BeatInfo::start(deviceid_t * _deviceID) {
    auto a = idClients.find(*_deviceID);
    if (a == idClients.end()) {
        log_e("[size: %i]Can't find client for DeviceID ", idClients.size());
        return 0;
    } 
    return (sendBeatInfoStartMsg(a->second));
}

size_t BeatInfo::sendBeatInfoStartMsg(AsyncClient * client) {
    WriteContext wrt;
    wrt.writeU32(4);
    wrt.writeU32(0);
    return send(client, wrt.getBuffer(), wrt.size());
}

size_t BeatInfo::sendIntroMsg(AsyncClient * _client) {
    WriteContext wrt;
    wrt.writeU32(0);
    wrt.writeBytes(deviceid.data, 16);
    wrt.writeNetworkString(svcName);
    wrt.writeU16(port);
    return send(_client, wrt.getBuffer(), wrt.size());
}

void BeatInfo::onBeatMsg(BeatMsgCallbackFunction cb)
{
    _cb = cb;
}

void BeatInfo::onReady(BeatInfoReadyCallbackFunction cb)
{
    readyCB = cb;
}