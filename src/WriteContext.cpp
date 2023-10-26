#include "WriteContext.h"

WriteContext::WriteContext() {
    this->wrt_length = 200*sizeof(uint8_t);
    this->wrt_buffer = (uint8_t*) heap_caps_malloc(wrt_length, MALLOC_CAP_8BIT);
    this->pos =0;
}

WriteContext::~WriteContext() {
    heap_caps_free(wrt_buffer);
}

int WriteContext::size() {
    return pos;
}

u_int8_t * WriteContext::getBuffer() {
    return wrt_buffer;
}

size_t WriteContext::getBufferSize() {
    return wrt_length;
}

int WriteContext::writeBytes(uint8_t * bytes, int length) {    
     if (!checkSize(length * sizeof(uint8_t))) {
        return -1;
    }   
    int written = 0;
    for (int i=0;i<length;i++) {
        written += writeU8Unsafe(bytes[i]);
    }
    return written;
}

int WriteContext::writeByteFill(uint8_t b, int length){
    if (!checkSize(length)) {
        return -1;
    }
    int written = 0;
    for (int i=0;i<length;i++) {
        written += writeU8Unsafe(b);
    }
    return written;
}

int WriteContext::writeU32(uint32_t num){
    if (!checkSize(sizeof(uint32_t))) {
        return -1;
    }
    uint32_t x = __bswap32(num);
    int written = 0;
    written += writeU8Unsafe((x >> 0)  & 0xFF);
    written += writeU8Unsafe((x >> 8)  & 0xFF);
    written += writeU8Unsafe((x >> 16) & 0xFF);
    written += writeU8Unsafe((x >> 24) & 0xFF);
    return written;
}

int WriteContext::writeU16(uint16_t num){
    if (!checkSize(sizeof(uint16_t))) {
        return -1;
    }
    uint16_t x = __bswap16(num);
    int written = 0;
    written += writeU8Unsafe((x >> 0)  & 0xFF);
    written += writeU8Unsafe((x >> 8)  & 0xFF);
    return written;
}

int WriteContext::writeU8(uint8_t num){
    if (!checkSize(sizeof(uint8_t))) {
        return -1;
    }
    return writeU8Unsafe(num);
}

int WriteContext::writeU8Unsafe(u_int8_t num) {
    wrt_buffer[pos++] = num;
    return 1;
}

int WriteContext::writeNetworkString(char * nString, int length) {
    if (!checkSize((length*2)+4)) {
        return -1;
    }
    int written = 0;
    written += writeU32((length-1)*2);
    for (int i=0;i<length;i++) {
        if (nString[i] == '\0') break;
        written += writeU8Unsafe(0x00);
        written += writeU8Unsafe(nString[i]);
    }
    return written;
}

int WriteContext::writeNetworkString(char * nString) {
    int strLen = strSize(nString);
    if (!checkSize((strLen*2)+4)) {
        return -1;
    }
    writeU32((strLen)*2);
    for (int i=0;i<(strLen+1);i++) {
        if (nString[i] == '\0') break;
        writeU8(0x00);
        writeU8(nString[i]);
    }
    return (strLen*2)+4;
}

int WriteContext::strSize(char * str) {
    char *e;
    int index;
    e = strchr(str, '\0');
    index = (int)(e - str);
    return index;
}

bool WriteContext::checkSize(int _length) {
    if (!wrt_buffer) {
        log_e("Buffer didn't alloc");
        return false;
    }
    if (pos + _length < wrt_length) {
        return true;
    } else {     
        return (grow(_length) > 0);
    }
}

int WriteContext::grow(int growLength) {
    int growSize = wrt_length + growLength + (100*sizeof(uint8_t));
    void* newBuff = heap_caps_realloc(wrt_buffer, growSize, MALLOC_CAP_8BIT);
    if (newBuff == NULL) {
        log_e("WRT grow() failed to realloc");
        return -1;
    } 
    this->wrt_buffer = (uint8_t*) newBuff;
    this->wrt_length = growSize;
    log_d("WriteContext new buffer size: %i", growSize);
    return growSize;
}