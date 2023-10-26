#ifndef ReadContext_h
#define ReadContext_h
#include "Arduino.h" 
#include "utils.h"

class ReadContext {
    public:
        ReadContext(uint8_t * _source, int length);
        ~ReadContext();
        
        void readDeviceID(uint8_t * target);
        void readBytes(uint8_t * bytes, int length);
        bool checkBuffer();
        uint32_t readU32();
        uint16_t readU16();
        uint8_t readU8();
        double readDouble();
        uint64_t readU64();
        size_t readNetworkString(char * target);
        uint8_t * getBuffer();
        uint8_t seek(int length);
        void setPos(int _pos);
        int sizeLeft();
        int position();
        int size();
        bool isEOF();
        
    protected:
        int uniqueID;
        int pos;
        uint8_t * src_buffer;
        int src_length;
};
#endif