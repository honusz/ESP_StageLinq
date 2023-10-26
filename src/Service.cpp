#include "Service.h"


Service::Service(char * _svcName, uint8_t * _deviceID, uint16_t _port) {
    this->svcName = _svcName;
    this->deviceid =_deviceID;
    this->port = _port;
    this->server = nullptr;
}

Service::Service(char * _svcName, deviceid_t *_deviceID, uint16_t _port) {
    this->deviceid =_deviceID;
    this->svcName = _svcName;
    this->port = _port;
    this->server = nullptr;
}

Service::~Service() {
    for (auto i = clientTaskHandles.begin(); i != clientTaskHandles.end(); i++) {
        vTaskDelete(i->second);
    }
    clientTaskHandles.clear();
    for (auto i = clientStreamBuffers.begin(); i != clientStreamBuffers.end(); i++) {
        vStreamBufferDelete(i->second);
    }
    clientStreamBuffers.clear();

     for (auto i = clientIDs.begin(); i != clientIDs.end(); i++) {
    }
    clientIDs.clear();
    idClients.clear();
}

size_t Service::send(AsyncClient* client, uint8_t * data, size_t len) {
    if (client->space() > len && client->canSend()) {
        client->add((char*)data, len);    
        if (client->send()) {
            return len;
        } else {
            log_e("%s client couldn't send %i bytes to %s:%i", svcName, len, client->remoteIP().toString(), client->remotePort());
            return 0;
        }       
    } else {    
        log_e("%s insufficient client space to send %i bytes to %s:%i", svcName, len, client->remoteIP().toString(), client->remotePort());
        return 0;
    }
}

size_t Service::sendIntroMsg(AsyncClient * _client) {
    WriteContext wrt;
    wrt.writeU32(2);
    wrt.writeBytes(deviceid.data, 16);
    return send(_client, wrt.getBuffer(), wrt.size());
}

size_t Service::sendWithLength(AsyncClient* client, uint8_t * data, size_t len){
    WriteContext wrt;
    wrt.writeU32(len);
    wrt.writeBytes(data, len);
    return send(client, wrt.getBuffer(), wrt.size());
}

bool Service::connect(IPAddress _address, uint16_t _port) {
    AsyncClient * client = new AsyncClient;
    client->setNoDelay(this->set_delay);
    log_d("%s connecting to %s:%d", svcName, _address.toString(), _port);
    client->onTimeout(&Service::s_handleTimeOut, this);
    client->onDisconnect(&Service::s_handleDisconnect, this);
    client->onError(&Service::s_handleError, this);
    client->onConnect(&Service::s_handleConnect, this);
    return client->connect(_address, _port);
}

void Service::handleData(AsyncClient * client, void * _data, size_t _len) {
}

void Service::handleConnect(AsyncClient * client) {    
    log_d("%s connected to %s:%d", this->svcName, client->remoteIP().toString(), client->remotePort());
    sendIntroMsg(client);
    client->onData(&Service::s_handleData, this);
}

bool Service::handleClient(AsyncClient * client) {    
    log_d("New %s client from %s:%d", this->svcName, client->remoteIP().toString(), client->remotePort());
    client->onData(&Service::s_handleData, this);
    if (useStreamBuffers) {
        StreamBufferHandle_t xStreamBuffer = xStreamBufferCreate( 4096, 4 );
        if (xStreamBuffer == NULL) {
            log_e("failed to create stream buffer");
            return 0;
        } 
        auto a = clientStreamBuffers.insert(std::make_pair(client, xStreamBuffer));
        return xStreamBuffer != NULL && a.second;
    }
    return true;
}

size_t Service::handleBufferedMessage(AsyncClient * client, void * _data, size_t _len) {
    stat.trigger(_len/84.0);
    auto buff = clientStreamBuffers.find(client);
    if (buff == clientStreamBuffers.end()) {
        log_e("Couldn't find client in clientStreamBuffer");
        return 0;
    }
        size_t written = xStreamBufferSend(buff->second, _data, _len, portMAX_DELAY);
        log_v("wrote %i bytes to streambuffer | buffer bytes available: %i", written, xStreamBufferBytesAvailable(buff->second));
        return written;
};

AsyncClient* Service::getClientFromID(deviceid_t _deviceID) {
    auto a = idClients.find(_deviceID);
    if (a != idClients.end()) {
        return a->second;
    } else {
        return NULL;
    }
}

deviceid_t * Service::getIDFromClient(AsyncClient * client) {
    auto a = clientIDs.find(client);
    if (a != clientIDs.end()) {
        return &a->second;
    } else {
        return NULL;
    }
}

void Service::startServer(bool _set_delay) {
    this->server = new AsyncServer(port);
    this->set_delay = _set_delay;
    this->server->setNoDelay(_set_delay);  
    server->onClient(&Service::s_handleClient, (void *)this);   
	server->begin();
    log_d("%s listening on %d", this->svcName, this->port);
}

void Service::stopServer() {
    server->end();
    this->server = nullptr;
    for (auto i = clientTaskHandles.begin(); i != clientTaskHandles.end(); i++) {
        vTaskDelete(i->second);
    }
    clientTaskHandles.clear();
    for (auto i = clientStreamBuffers.begin(); i != clientStreamBuffers.end(); i++) {
        vStreamBufferDelete(i->second);
    }
    clientStreamBuffers.clear();

    for (auto i = clientIDs.begin(); i != clientIDs.end(); i++) {
    }
    clientIDs.clear();
    idClients.clear();
    log_d("%s server stopped", this->svcName);    
}

uint16_t Service::getPort() {
    return this->port;
}

void Service::handleError(AsyncClient* client, int8_t error) {
	log_e("connection error %s from client %s", client->errorToString(error), client->remoteIP().toString().c_str());
}

void Service::handleDisconnect(AsyncClient* client) {
	log_d("client %s disconnected", client->remoteIP().toString().c_str());
    auto c = clientTaskHandles.find(client);
    if (c != clientTaskHandles.end()) {
        vTaskSuspend(c->second);
        vTaskDelete(c->second);
    }
    
    auto a = clientStreamBuffers.find(client);
    if (a != clientStreamBuffers.end()) {
        vStreamBufferDelete(a->second);
        clientStreamBuffers.erase(client);
    }
    auto b = clientIDs.find(client);
    if (b != clientIDs.end()) {
        idClients.erase(b->second);
        clientIDs.erase(client);
    }
}

void Service::handleTimeOut( AsyncClient* client, uint32_t time) {
	log_w("client ACK timeout ip: %s", client->remoteIP().toString().c_str());
}

void Service::s_handleBufferedMessage(void* arg, AsyncClient* client, void * _data, size_t _len) {
    (reinterpret_cast<Service*>(arg))->handleBufferedMessage(client, _data, _len);
}

void Service::processBufferedMessage(AsyncClient * client, void * _data, size_t _len) {
    ReadContext rdr((uint8_t*)_data, _len);
    free(_data);
    readMessage(&rdr, client);
}

void Service::readMessage(ReadContext *rdr, AsyncClient * client) {}

void Service::subscribe(AsyncClient * client) {}

void Service::s_handleConnect(void* arg, AsyncClient* client) {
    (reinterpret_cast<Service*>(arg))->handleConnect(client);
}

void Service::s_handleClient(void* arg, AsyncClient* client) {
    (reinterpret_cast<Service*>(arg))->handleClient(client);
}

void Service::s_handleData(void* arg, AsyncClient* client, void * _data, size_t _len) {
   (reinterpret_cast<Service*>(arg))->handleData(client, _data, _len);
}

void Service::s_handleError(void* arg, AsyncClient* client, int8_t error) {
	(reinterpret_cast<Service*>(arg))->handleError(client, error);
}

void Service::s_handleDisconnect(void* arg, AsyncClient* client) {
	(reinterpret_cast<Service*>(arg))->handleDisconnect(client);
}

void Service::s_handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
	(reinterpret_cast<Service*>(arg))->handleTimeOut(client, time);
}

void Service::s_handleTask(void * pvParams) {
    task_data_t * task_data = (task_data_t*) pvParams;
    auto a = (reinterpret_cast<Service*>(task_data->svc))->clientStreamBuffers.find(task_data->client);
    if (a == (reinterpret_cast<Service*>(task_data->svc))->clientStreamBuffers.end()) {
        log_e("couldn't find stream buffer");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    StreamBufferHandle_t stream = a->second;
    for (;;) {
        size_t bytesAvail = xStreamBufferBytesAvailable(stream);
        if (bytesAvail >= 4) {         
            union {
                uint32_t integer;
                uint8_t arr[4];
            } u_length;
            xStreamBufferReceive(stream, u_length.arr, 4, 0);
            uint32_t length = __bswap32(u_length.integer);
            log_v("length %i bytes available: %i", length, bytesAvail);
            uint8_t * data = (uint8_t*) malloc(length);
            if (data == NULL) {
                    log_e("can't alloct");
                    vTaskDelay(100);
                    continue;
            } 
            if (length <= bytesAvail) {
                xStreamBufferReceive(stream, data, length, 0);
                (reinterpret_cast<Service*>(task_data->svc))->processBufferedMessage(task_data->client, data, length);                
            } else {
                xStreamBufferReceive(stream, data, bytesAvail, 0);
                WriteContext wrt;
                wrt.writeU32(length);
                wrt.writeBytes(data, bytesAvail);
                log_v("sending %i bytes back to buffer", bytesAvail);
                log_buf_v(data, bytesAvail);
                xStreamBufferSend(stream, wrt.getBuffer(), wrt.size(), portMAX_DELAY);
                free(data);
            }
            vTaskDelay(1);
        } else {
            vTaskDelay(10);
        }
    }
}