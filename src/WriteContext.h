#ifndef WriteContext_h
#define WriteContext_h
#include "Arduino.h"
 
class WriteContext {
public:
	WriteContext();
   ~WriteContext();
    int writeBytes(uint8_t * bytes, int length);
    int writeByteFill(uint8_t b, int length);
	int writeU32(uint32_t num);
    int writeU16(uint16_t num);
    int writeU8(uint8_t num);
    int writeNetworkString(char nString[], int length);
    int writeNetworkString(char * nString);
    int size();
    size_t getBufferSize(); 
    uint8_t * getBuffer();
    
protected:
    int pos;
    uint8_t * wrt_buffer;
    size_t wrt_length;
    bool checkSize(int _length);
    int grow(int growLength);
    int strSize(char * str);
    int writeU8Unsafe(u_int8_t num);
};
#endif