#include <Arduino.h>
#include <WiFi.h>
#include <esp_StageLinq.h>
#include "esp_log.h"

const char *ssid = ENV_WIFI_SSID;
const char *password = ENV_WIFI_PASS;

static uint32_t heapReg = 0;
xTimerHandle memStatusTimer;
xTimerHandle beatCountTimer;

deviceid_t disco_id;

uint16_t disco_port = randomPort();

Discovery discovery(
  disco_port,
  disco_id,
  (char*) "jmh9000",
  (char*) "JH06",
  (char*) "6.6.6"
);

Directory directory((char *)"Directory", &disco_id, discovery.getPort());
StateMap stateMap((char *)"StateMap", &disco_id, randomPort());
BeatInfo beatInfo((char *)"BeatInfo", &disco_id, randomPort());

void memStatus(void * parameter);
void beatMsgCounter(void * pvParam);
void stagelinq_start();
void stagelinq_stop();

static void onBeatInfoReady(deviceid_t * _deviceID);
static void onBeatMsg(beat_msg_t * msg);
static void onStateMapReady(deviceid_t * _deviceID);
static void onStateMsg(deviceid_t * _deviceID, char* key, char* value);
static void onDiscoMsg(DiscoMsg_t * msg);


void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000);
  
  Serial.printf("WiFi Connecting | wifi_ssid %s | wifi_pass %s", ssid, password);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){   
    stagelinq_start();
}, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){   
    Serial.printf("%s\n", WiFi.disconnectReasonName((wifi_err_reason_t)info.wifi_sta_disconnected.reason));
}, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  memStatusTimer = xTimerCreate(
      "MemStatusTimer", /* name */
      pdMS_TO_TICKS(1000), /* period/time */
      pdTRUE, /* auto reload */
      (void*)0, /* timer ID */
      memStatus);  

 beatCountTimer = xTimerCreate(
      "BeatMsgCounter", /* name */
      pdMS_TO_TICKS(5000), /* period/time */
      pdTRUE, /* auto reload */
      (void*)0, /* timer ID */
      beatMsgCounter);
}

void loop() {
  
}

void memStatus(void * pvParam) {
  uint32_t current = esp_get_free_heap_size();
  if (current != heapReg) {
    log_d("ESP HEAP SIZE: %i", current);
  }
  heapReg = current;
}

void beatMsgCounter(void * pvParam) {
  int64_t currentTime = esp_timer_get_time();
  
  double runningTime = (currentTime - beatInfo.stat.startTime) / 1000000.0;
  double pktAvg = beatInfo.stat.packetsRecvd / runningTime; 
  double msgAvg = beatInfo.stat.msgsRecvd /runningTime; 
  double msgPerPkt = beatInfo.stat.msgsRecvd / beatInfo.stat.packetsRecvd;
  double proc_runningTime = (currentTime - beatInfo.proc_stat.startTime) / 1000000.0;
  double proc_pktAvg = beatInfo.proc_stat.packetsRecvd / proc_runningTime; 
  double latencyMS = (beatInfo.proc_stat.lastTime - beatInfo.stat.lastTime) / 1000;

  Serial.printf("proc latency(ms): %.4lf | time(s): %.2lf | pkts %.0lf pkts/sec: %.2lf | msgs: %.0lf msgs/sec: %.2lf | msgs/pkt: %.2lf\n", latencyMS, runningTime, beatInfo.stat.packetsRecvd, pktAvg, beatInfo.stat.msgsRecvd, msgAvg, msgPerPkt);
}


void stagelinq_start() {
  char _deviceid[40];
  printDeviceIDTo(disco_id, _deviceid);
  Serial.printf("Starting StageLinq | DeviceID: %s\n", _deviceid);

  discovery.onDiscoMsg(&onDiscoMsg);
  stateMap.onStateMsg(&onStateMsg);
  stateMap.onReady(&onStateMapReady);
  beatInfo.onBeatMsg(&onBeatMsg);
  beatInfo.onReady(&onBeatInfoReady);
  
  directory.addService((char *)"StateMap", stateMap.getPort());
  directory.addService((char *)"BeatInfo", beatInfo.getPort());
  
  directory.startServer(false);
  stateMap.startServer(false);
  beatInfo.startServer(true);
  
  discovery.listen();  
  discovery.announce();
  xTimerStart(memStatusTimer, 0);
}


static void onBeatInfoReady(deviceid_t * _deviceID) {
  log_i("Starting BeatInfo");
  beatInfo.start(_deviceID);
  xTimerStart(beatCountTimer, 0);
}

static void onStateMapReady(deviceid_t * _deviceID) {
    char _id[40];
    printDeviceIDTo(*_deviceID, _id);
    Serial.printf("StateMap Ready %s\n", _id);
    int sent = 0;
    sent += stateMap.subscribeState(_deviceID, States::CurrentDevice);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckCurrentBPM, 1); 
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckDeckIsMaster, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckPlayState, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckSpeedNeutral, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckSpeedOffsetDown, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckSpeedOffsetUp, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckSpeedRange, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckSpeedState, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackSongName, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackArtistName, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackTrackName, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackTrackURI, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackTrackData, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackTrackBytes, 1);
    sent += stateMap.subscribeState(_deviceID, DeckStates::DeckTrackSoundSwitchGUID, 1);    
    log_v("sent %i bytes of sub msgs", sent);
}

static void onStateMsg(deviceid_t * _deviceID, char* key, char*value) {
  char _id[40];
  printDeviceIDTo(*_deviceID, _id);
  unsigned long hash = getHash(key, stringLength(key));
  log_i("[%s] %s | %s | size: %i | hash: %x \n", _id, key, value, stateMap.getStateRegSize(), hash);
}

static void onBeatMsg(beat_msg_t * msg) {
  // if(Serial.availableForWrite()) {
  //   // Serial.printf("[%s] Clock: %llu | #Decks: %i | BPM %lf | %lf/%lf | Samples: %lf\n", idToString(msg->deviceID), msg->clock, msg->deckCount, msg->deck[0].BPM, msg->deck[0].beat, msg->deck[0].totalBeats, msg->deck[0].samples);
  // }
}

static void onDiscoMsg(DiscoMsg_t *msg) {
  char _id[40];
  printDeviceIDTo(msg->deviceid, _id);
  Serial.printf("\nDeviceID: %s\n", _id);
  Serial.printf("Unit:     %s\n", msg->unit);
  Serial.printf("Action:   %i\n", msg->action);
  Serial.printf("Software: %s\n", msg->software);
  Serial.printf("Version:  %s\n", msg->version);
  Serial.printf("Address:  %s\n", msg->address.toString());
  Serial.printf("Port:     %i\n\n", msg->port);
}

