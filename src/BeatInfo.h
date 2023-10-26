#ifndef BeatInfo_h
#define BeatInfo_h
#include "Arduino.h" 
#include "utils.h"
#include "Service.h"



struct beat_deck_msg_t {
    double beat;
    double totalBeats;
    double BPM;
    double samples;
    bool operator==( const beat_deck_msg_t& other ) {
        return beat == other.beat &&
               totalBeats == other.totalBeats &&
               BPM  == other.BPM  &&
               samples  == other.samples;
    }
    bool operator!=( const beat_deck_msg_t& other ) {
        return beat != other.beat ||
               totalBeats != other.totalBeats ||
               BPM  != other.BPM  ||
               samples  != other.samples;
    }
} ;

struct beat_msg_t {
    deviceid_t deviceid;
    uint64_t clock;
    int deckCount;
    beat_deck_msg_t deck[4];
    //beat_msg_t(): clock(0), deckCount(0) {
     //   for (int i=0; i<4; i++) deck[i] = {0,0,0,0};
    //};
    bool operator==( const beat_msg_t& other ) {
        return deck[0] == other.deck[0] &&
               deck[1] == other.deck[1]; 
            //    deck[2]  == other.deck[2]  &&
            //    deck[3]  == other.deck[3];
    }
} ;

typedef std::function<void(beat_msg_t * svc)> BeatMsgCallbackFunction;
typedef std::function<void(deviceid_t * deviceID)> BeatInfoReadyCallbackFunction;

class BeatInfo: public Service {
public:
	BeatInfo(char * _name, uint8_t *_deviceID, uint16_t _port);
    BeatInfo(char * _name, deviceid_t *_deviceID, uint16_t _port);
    bool start(deviceid_t *_deviceID);
    void onReady(BeatInfoReadyCallbackFunction);
    void onBeatMsg(BeatMsgCallbackFunction);
    size_t sendBeatInfoStartMsg(AsyncClient * client);
    
protected:
    void subscribe(AsyncClient* client) override;
    void readMessage(ReadContext *rdr, AsyncClient * client) override;
    void handleData(AsyncClient * client, void * _data, size_t _len) override;
    

    size_t sendIntroMsg(AsyncClient * _client) override;
    int readCommand(ReadContext *rdr, AsyncClient * client);
    int processBeatMsg(ReadContext *rdr, AsyncClient * client);
    void handleBeatMsg(AsyncClient * client, void * _data, size_t _len); 
    static void s_handleBeatMessage(void* arg, AsyncClient* client, void * _data, size_t _len);
    BeatMsgCallbackFunction _cb;
    BeatInfoReadyCallbackFunction readyCB;
    
};

#endif