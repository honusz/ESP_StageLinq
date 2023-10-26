#ifndef utils_h
#define utils_h
#include "Arduino.h" 
#include <set>
#include <map>

extern uint8_t DEVICE_ID[16];
extern uint8_t ZERO_DEVICE_ID[16];
extern unsigned int beatMsgCount;

struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};

struct cmp_buf
{
   bool operator()(uint8_t const *a, uint8_t const *b) const
   {
      return memcmp(a, b, 16) < 0;
   }
};

struct time_stat {
   double lastTime = 0.0;
   double startTime = 0.0;
   double packetsRecvd = 0.0;
   double msgsRecvd = 0.0;
   
   void trigger(double msgCount) {
      lastTime = esp_timer_get_time();
      if (!startTime) {
         startTime = esp_timer_get_time();
      }
      packetsRecvd++;
      msgsRecvd += msgCount; 
   }
};

struct deviceid_t {
   uint8_t data[16];
   deviceid_t() {
      memcpy(&data, DEVICE_ID, 16);
      for (int i=12; i<16; i++) {
         data[i] = (uint8_t) random(20, 255);
      }
   }
   deviceid_t(uint8_t * other) {
      memcpy(&data, other, 16);
   }
   bool operator==( const uint8_t* other ) {
      return (memcmp(&data, other, 16) == 0); 
   }
   bool operator==( const deviceid_t* other ) {
      return (memcmp(&data, other->data, 16) == 0); 
   }
   void operator=(const uint8_t* other ) {
      memcpy(&data, other, 16);
   }
   void operator=(const deviceid_t* other ) {
      memcpy(&data, other->data, 16);
   }
   friend bool operator<(deviceid_t const& lhs, deviceid_t const& rhs) {
      return memcmp(lhs.data, rhs.data, 16) < 0;
   }
};

typedef std::map<uint8_t*, char*, cmp_buf> DataCharMap;
typedef std::map<uint8_t*, std::string, cmp_buf> DataStringMap;


void printDeviceIDTo(deviceid_t _id, char * target);



// unsigned int hashpjw (const unsigned char *x, unsigned int len);
unsigned long getHash (const void *key, size_t keylen);

size_t stringLength(char * str);

uint16_t randomPort();

#endif