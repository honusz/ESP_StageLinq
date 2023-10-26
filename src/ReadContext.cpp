#include "ReadContext.h"

union arrayTo64 {
    uint8_t array[8];
    double_t fpoint;
    uint64_t integer;
};

union ArrayTo32 {
  uint8_t array[4];
  uint32_t integer;
};

ReadContext::ReadContext(uint8_t * _source, int _length) {
	uniqueID = 0;
    this->src_length = _length * sizeof(uint8_t);
    this->src_buffer = (uint8_t*) malloc(src_length); 
    
    if (src_buffer == NULL) {
        log_e("ReadContext failed to init");
        // throw std::bad_alloc();
    } else {
        memcpy(src_buffer, _source, _length);
    }
    src_length = _length;
    this->pos =0;
}

ReadContext::~ReadContext() {
    free(src_buffer);
}

bool ReadContext::checkBuffer() {
    return (src_buffer != NULL);
}

uint8_t * ReadContext::getBuffer() {
    return src_buffer;
}

bool ReadContext::isEOF() {
    return src_length >= pos;
}

int ReadContext::sizeLeft() {
    return (src_length - pos);
}

int ReadContext::position() {
    return pos;
}

void ReadContext::setPos(int _pos) {
    pos = _pos;
}

int ReadContext::size() {
    return src_length;
}

uint8_t ReadContext::seek(int length) {
    pos += length;
    return (uint8_t) 0x00;
}

void ReadContext::readBytes(uint8_t * target, int length) {
    for (int i=0;i<length;i++) {
        target[i] += readU8();
    }
}

void ReadContext::readDeviceID(uint8_t * target) {
    memcpy(target, pos + src_buffer, 16);
    pos+=16;
}

double ReadContext::readDouble() {
    arrayTo64 value;
    value.array[7] = src_buffer[pos++];
    value.array[6] = src_buffer[pos++];
    value.array[5] = src_buffer[pos++];
    value.array[4] = src_buffer[pos++];
    value.array[3] = src_buffer[pos++];
    value.array[2] = src_buffer[pos++];
    value.array[1] = src_buffer[pos++];
    value.array[0] = src_buffer[pos++];
    return value.fpoint;
}

uint64_t ReadContext::readU64() {   
    uint64_t value;
    memcpy(&value, pos + src_buffer, 8);
    pos +=8;
    return __bswap64(value);
}

uint32_t ReadContext::readU32(){
    ArrayTo32 value;
    value.array[0] = src_buffer[pos++];
    value.array[1] = src_buffer[pos++];
    value.array[2] = src_buffer[pos++];
    value.array[3] = src_buffer[pos++];
    return __bswap32(value.integer);
}

uint16_t ReadContext::readU16(){
    uint16_t value;
    memcpy(&value, pos + src_buffer, 2);
    pos += 2;
    return __bswap16(value);
}

uint8_t ReadContext::readU8(){
    return src_buffer[pos++];
}

size_t ReadContext::readNetworkString(char * target) {
    uint32_t length = readU32();
    for (int i = 0; i<(length/2); i++) {
        seek(1);
        target[i] = readU8();
    }
    target[(length/2)] = '\0';
    return (length/2)+1;
}