#ifndef States_h
#define States_h
struct States {  
    static char* CurrentDevice;// = "/Client/Librarian/DevicesController/CurrentDevice"; 
    static char* CurrentDeviceNetworkPath; 
    static char* HasSDCardConnected; 
    static char* HasUsbDeviceConnected; 
    static char* PreferencesLayerA; 
    static char* PreferencesLayerB; 
    static char* PreferencesPlayer; 
    static char* PlayerJogColorA; 
    static char* PlayerJogColorB; 
    static char* PlayerColor1; 
    static char* PlayerColor1A; 
    static char* PlayerColor1B; 
    static char* SyncMode; 
    static char* DeckCount; 
    static char* MasterTempo; 
    static char* MasterStatus; 
    static char* SyncType; 
    static char* AnyLocalAnchoredTrackLoaded; 
    static char* GUIActiveDeck; 
    static char* GUIViewLayerLayerB; 
};


struct DeckStates {  
    static char* DeckColor; 
    static char* DeckCurrentBPM; 
    static char* DeckDeckIsMaster; 
    static char* DeckExternalMixerVolume; 
    static char* DeckExternalScratchWheelTouch; 
    static char* DeckPadsView; 
    static char* DeckPlay; 
    static char* DeckPlayState; 
    static char* DeckPlayStatePath; 
    static char* DeckRequestUnsetSyncLead; 
    static char* DeckSpeed; 
    static char* DeckSpeedNeutral; 
    static char* DeckSpeedOffsetDown; 
    static char* DeckSpeedOffsetUp; 
    static char* DeckSpeedRange; 
    static char* DeckSpeedState; 
    static char* DeckSyncMode; 
    static char* DeckSyncPlayState; 
    static char* DeckTrackArtistName; 
    static char* DeckTrackBleep; 
    static char* DeckTrackCuePosition; 
    static char* DeckTrackCurrentBPM; 
    static char* DeckTrackCurrentKeyIndex; 
    static char* DeckTrackCurrentLoopInPosition; 
    static char* DeckTrackCurrentLoopOutPosition; 
    static char* DeckTrackCurrentLoopSizeInBeats; 
    static char* DeckTrackKeyLock; 
    static char* DeckTrackLoopEnableState; 
    static char* DeckTrackLoopQuickLoop1; 
    static char* DeckTrackLoopQuickLoop2; 
    static char* DeckTrackLoopQuickLoop3; 
    static char* DeckTrackLoopQuickLoop4; 
    static char* DeckTrackLoopQuickLoop5; 
    static char* DeckTrackLoopQuickLoop6; 
    static char* DeckTrackLoopQuickLoop7; 
    static char* DeckTrackLoopQuickLoop8; 
    static char* DeckTrackPlayPauseLEDState;
    static char* DeckTrackSampleRate; 
    static char* DeckTrackSongAnalyzed; 
    static char* DeckTrackSongLoaded; 
    static char* DeckTrackSongName; 
    static char* DeckTrackSoundSwitchGUID;
    static char* DeckTrackTrackBytes; 
    static char* DeckTrackTrackData; 
    static char* DeckTrackTrackLength; 
    static char* DeckTrackTrackName; 
    static char* DeckTrackTrackNetworkPath; 
    static char* DeckTrackTrackURI; 
    static char* DeckTrackTrackWasPlayed; 
};

#endif