/*

OpenDECK library v1.3
File: OpenDeck.h
Last revision date: 2014-12-25
Author: Igor Petrovic

*/


#ifndef OpenDeck_h
#define OpenDeck_h

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "EEPROM.h"
#include "SysEx.h"

#define MAX_NUMBER_OF_POTS          16
#define MAX_NUMBER_OF_BUTTONS       64
#define MAX_NUMBER_OF_LEDS          64
#define MAX_NUMBER_OF_ENCODERS      16

#define PIN_A                       8
#define PIN_B                       9
#define PIN_C                       2
#define PIN_D                       3

#define NUMBER_OF_START_UP_ROUTINES 5

#define COLUMN_SCAN_TIME 1

class OpenDeck  {

    public:

    //constructor
    OpenDeck();

    //library initializer
    void init();

    //buttons
    void setHandleButtonNoteSend(void (*fptr)(uint8_t, bool, uint8_t));
    void setHandleButtonPPSend(void (*fptr)(uint8_t, uint8_t));
    void readButtons(uint8_t);

    //pots
    void setHandlePotCC(void (*fptr)(uint8_t, uint8_t, uint8_t));
    void setHandlePotNoteOn(void (*fptr)(uint8_t, uint8_t));
    void setHandlePotNoteOff(void (*fptr)(uint8_t, uint8_t));
    uint8_t getMuxPin(uint8_t);
    void readPots();

    //encoders
    void readEncoders(int32_t);
    void setHandlePitchBend(void (*fptr)(uint16_t, uint8_t));

    //LEDs
    void oneByOneLED(bool, bool, bool);
    void allLEDsOn();
    void allLEDsOff();
    void turnOnLED(uint8_t);
    void turnOffLED(uint8_t);
    void storeReceivedNoteOn(uint8_t, uint8_t, uint8_t);
    void checkReceivedNoteOn();
    void checkLEDs(uint8_t);

    //matrix
    void activateColumn(int8_t);
    void processMatrix();

    //getters
    uint8_t getInputMIDIchannel();
    bool standardNoteOffEnabled();
    uint8_t getNumberOfColumns();
    uint8_t getNumberOfMux();
    uint8_t getBoard();
    bool sysExRunning();

    //hardware control
    void ledRowOn(uint8_t);
    void ledRowsOff();
    void setMuxInput(uint8_t);
    void setUpSwitchTimer();
    void stopSwitchTimer();

    //sysex
    void setHandleSysExSend(void (*fptr)(uint8_t*, uint8_t));
    void processSysEx(uint8_t sysExArray[], uint8_t);
    bool sysExSetDefaultConf();

    private:

    //variables

    //hardware 

    uint8_t         hardwareEnabled;

    //features
    uint8_t         midiFeatures,
                    buttonFeatures,
                    ledFeatures,
                    potFeatures;

    //MIDI channels
    uint8_t         _buttonNoteChannel,
                    _longPressButtonNoteChannel,
                    _buttonPPchannel,
                    _potCCchannel,
                    _potPPchannel,
                    _potNoteChannel,
                    _inputChannel;

    //buttons
    uint8_t         buttonType[MAX_NUMBER_OF_BUTTONS/8],
                    buttonPPenabled[MAX_NUMBER_OF_BUTTONS/8],
                    buttonNote[MAX_NUMBER_OF_BUTTONS],
                    previousButtonState[MAX_NUMBER_OF_BUTTONS/8],
                    buttonPressed[MAX_NUMBER_OF_BUTTONS/8],
                    longPressSent[MAX_NUMBER_OF_BUTTONS/8],
                    longPressCounter[MAX_NUMBER_OF_BUTTONS],
                    longPressColumnPass,
                    lastColumnState[8],
                    columnPassCounter[8],
                    numberOfColumnPasses;

    //pots
    uint8_t         potEnabled[MAX_NUMBER_OF_POTS/8],
                    potPPenabled[MAX_NUMBER_OF_POTS/8],
                    potInverted[MAX_NUMBER_OF_POTS/8],
                    ccppNumber[MAX_NUMBER_OF_POTS],
                    ccLowerLimit[MAX_NUMBER_OF_POTS],
                    ccUpperLimit[MAX_NUMBER_OF_POTS],
                    lastPotNoteValue[MAX_NUMBER_OF_POTS];

    uint16_t        lastAnalogueValue[MAX_NUMBER_OF_POTS];

    //LEDs
    uint8_t         ledActNote[MAX_NUMBER_OF_LEDS];
    uint16_t        _blinkTime;
    uint8_t         totalNumberOfLEDs;
    uint8_t         ledState[MAX_NUMBER_OF_LEDS];

    bool            blinkState,
                    blinkEnabled;

    uint32_t        blinkTimerCounter;

    //input
    bool            receivedNoteOnProcessed;

    uint8_t         receivedChannel,
                    receivedNote,
                    receivedVelocity;

    //hardware
    uint8_t         _board,
                    _numberOfColumns,
                    _numberOfButtonRows,
                    _numberOfLEDrows,
                    _numberOfMux;

    uint8_t         analogueEnabledArray[8];

    //sysex
    bool            sysExEnabled,
                    _sysExRunning;

    //general
    uint8_t         i;

    //functions

    //init
    void initVariables();
    bool initialEEPROMwrite();

    //configuration retrieval from EEPROM
    void getConfiguration();
    void getHardwareConfig();
    void getFeatures();
    void getMIDIchannels();
    void getButtonsType();
    void getPPenabledButtons();
    void getButtonNotes();
    void getEnabledPots();
    void getPPenabledPots();
    void getPotInvertStates();
    void getCCPPnumbers();
    void getCCPPlowerLimits();
    void getCCPPupperLimits();
    void getLEDnotes();
    void getLEDHwParameters();

    //buttons
    void (*sendButtonNoteDataCallback)(uint8_t, bool, uint8_t);
    void (*sendButtonPPDataCallback)(uint8_t, uint8_t);
    uint8_t getRowPassTime();
    void setNumberOfColumnPasses();
    void procesButtonReading(uint8_t buttonNumber, uint8_t buttonState);
    uint8_t getButtonType(uint8_t);
    uint8_t getButtonNote(uint8_t);
    bool getButtonPPenabled(uint8_t);
    bool getButtonPressed(uint8_t);
    bool getButtonLongPressed(uint8_t);
    void setButtonPressed(uint8_t, bool);
    void setButtonLongPressed(uint8_t, bool);
    void processMomentaryButton(uint8_t, bool);
    void processLatchingButton(uint8_t, bool);
    void updateButtonState(uint8_t, uint8_t);
    bool getPreviousButtonState(uint8_t);
    void setNumberOfLongPressPasses();
    void resetLongPress(uint8_t);
    void handleLongPress(uint8_t, bool);

    //pots
    void (*sendPotCCDataCallback)(uint8_t, uint8_t, uint8_t);
    void (*sendPotNoteOnDataCallback)(uint8_t, uint8_t);
    void (*sendPotNoteOffDataCallback)(uint8_t, uint8_t);
    uint8_t getPotNumber(uint8_t, uint8_t);
    void readPotsMux(uint8_t, uint8_t);
    bool checkPotReading(int16_t, uint8_t);
    void processPotReading(int16_t, uint8_t);
    bool getPotEnabled(uint8_t);
    bool getPotPPenabled(uint8_t);
    bool getPotInvertState(uint8_t);
    uint8_t getCCnumber(uint8_t);
    uint8_t getPotNoteValue(uint8_t, uint8_t);
    bool checkPotNoteValue(uint8_t, uint8_t);
    int8_t getActiveMux();
    void readPotsInitial();

    //encoders
    uint8_t getEncoderPairEnabled(uint8_t);
    bool checkMemberOfEncPair(uint8_t, uint8_t);
    void processEncoderPair(uint8_t, uint8_t, uint8_t);
    uint8_t getEncoderPairNumber(uint8_t, uint8_t);
    void (*sendPitchBendDataCallback)(uint16_t, uint8_t);

    //LEDs
    void startUpRoutine();
    bool ledOn(uint8_t);
    bool checkLEDsOn();
    bool checkLEDsOff();
    void checkBlinkLEDs();
    bool checkBlinkState(uint8_t);
    void handleLED(bool, bool, uint8_t);
    void setLEDState();
    void setConstantLEDstate(uint8_t);
    void setBlinkState(uint8_t, bool);
    void switchBlinkState();
    uint8_t getLEDnumber();
    uint8_t getLEDnote(uint8_t);

    //columns
    int8_t getActiveColumn();
    bool columnStable(uint8_t, uint8_t);

    //sysex
    //callback
    void (*sendSysExDataCallback)(uint8_t*, uint8_t);
    //message check
    bool sysExCheckMessageValidity(uint8_t*, uint8_t);
    bool sysExCheckID(uint8_t, uint8_t, uint8_t);
    bool sysExCheckWish(uint8_t);
    bool sysExCheckAmount(uint8_t);
    bool sysExCheckMessageType(uint8_t);
    bool sysExCheckMessageSubType(uint8_t, uint8_t);
    bool sysExCheckParameterID(uint8_t, uint8_t, uint8_t);
    bool sysExCheckNewParameterID(uint8_t, uint8_t, uint8_t, uint8_t);
    bool sysExCheckSpecial(uint8_t, uint8_t, uint8_t);
    uint8_t sysExGenerateMinMessageLenght(uint8_t, uint8_t, uint8_t, uint8_t);
    //sysex response
    void sysExGenerateError(uint8_t);
    void sysExGenerateAck();
    void sysExGenerateResponse(uint8_t*, uint8_t);
    //getters
    uint8_t sysExGet(uint8_t, uint8_t, uint8_t);
    uint8_t sysExGetMIDIchannel(uint8_t);
    uint8_t sysExGetHardwareParameter(uint8_t);
    bool sysExGetFeature(uint8_t, uint8_t);
    uint8_t sysExGetButtonHwParameter(uint8_t);
    uint8_t sysExGetLEDHwParameter(uint8_t);
    uint8_t sysExGetHardwareConfig(uint8_t);
    //setters
    bool sysExSet(uint8_t, uint8_t, uint8_t, uint8_t);
    bool sysExSetMIDIchannel(uint8_t, uint8_t);
    bool sysExSetHardwareParameter(uint8_t, uint8_t);
    bool sysExSetFeature(uint8_t, uint8_t, bool);
    bool sysExSetButtonType(uint8_t, bool);
    bool sysExSetButtonNote(uint8_t, uint8_t);
    bool sysExSetPotEnabled(uint8_t, bool);
    bool sysExSetPotInvertState(uint8_t, bool);
    bool sysExSetCCPPnumber(uint8_t, uint8_t);
    bool sysExSetCClimit(uint8_t, uint8_t, uint8_t);
    bool sysExSetLEDnote(uint8_t, uint8_t);
    bool sysExSetLEDstartNumber(uint8_t, uint8_t);
    bool sysExSetEncoderPair(uint8_t, bool);
    bool sysExSetButtonHwParameter(uint8_t, uint8_t);
    bool sysExSetButtonPPenabled(uint8_t, bool);
    bool sysExSetHardwareConfig(uint8_t, uint8_t);
    bool sysExSetLEDHwParameter(uint8_t, uint8_t);
    bool sysExSetPotPPEnabled(uint8_t, bool);
    //restore
    bool sysExRestore(uint8_t, uint8_t, uint16_t, int16_t);

    //hardware control
    void initBoard();
    void initPins();
    void readButtonColumn(uint8_t &);
    void enableAnalogueInput(uint8_t, uint8_t);

    //new
    bool checkSameLEDvalue(uint8_t, uint8_t);

};

extern OpenDeck openDeck;

#endif