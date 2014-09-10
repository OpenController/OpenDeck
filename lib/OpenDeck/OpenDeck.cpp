/*

OpenDECK library v1.97
File: OpenDeck.cpp
Last revision date: 2014-09-10
Author: Igor Petrovic

*/


#include "OpenDeck.h"
#include "Ownduino.h"
#include <avr/io.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "SysEx.h"
#include "EEPROM.h"

OpenDeck::OpenDeck()    {

    //initialization
    initVariables();

    //set all callbacks to NULL pointer

    sendButtonDataCallback      =   NULL;
    sendLEDrowOnCallback        =   NULL;
    sendLEDrowsOffCallback      =   NULL;
    sendButtonReadCallback      =   NULL;
    sendSwitchMuxOutCallback    =   NULL;
    sendInitPinsCallback        =   NULL;
    sendColumnSwitchCallback    =   NULL;
    sendPotCCDataCallback       =   NULL;
    sendPotNoteOnDataCallback   =   NULL;
    sendPotNoteOffDataCallback  =   NULL;
    sendSysExDataCallback       =   NULL;

}

//init

//public
void OpenDeck::init(uint8_t board)   {
    
    _board = board;

    switch (_board) {

        case BOARD_TANNIN:
        HCTannin::initPins();
        _numberOfMux = 2;
        _numberOfColumns = 5;
        _numberOfButtonRows = 4;
        _numberOfLEDrows = 1;
        enableAnalogueInput(0);
        enableAnalogueInput(1);
        break;

        case BOARD_OPEN_DECK_1:
        HCOpenDeck1::initPins();
        _numberOfMux = 2;
        _numberOfColumns = 8;
        _numberOfButtonRows = 4;
        _numberOfLEDrows = 4;
        enableAnalogueInput(6);
        enableAnalogueInput(7);
        break;

        default:
        //custom board, define hardware control inside main program
        sendInitPinsCallback();
        break;

    }

    setNumberOfColumnPasses();

    for (int i=0; i<MAX_NUMBER_OF_BUTTONS; i++)     previousButtonState[i] = buttonDebounceCompare;

    //initialize lastPotNoteValue to 128, which is impossible value for MIDI,
    //to avoid sending note off for that value on first read
    for (int i=0; i<MAX_NUMBER_OF_POTS; i++)        lastPotNoteValue[i] = 128;

    blinkState              = true;
    receivedNoteProcessed   = true;

    //make initial pot reading to avoid sending all data on startup
    readPots();

    initialEEPROMwrite();

    //get all values from EEPROM
    getConfiguration();

}

void OpenDeck::initialEEPROMwrite()  {

    //if ID bytes haven't been written to EEPROM on specified address,
    //write default configuration to EEPROM
    if  (!

        ((eeprom_read_byte((uint8_t*)EEPROM_ID_BYTE_1) == 0x4F) &&
        (eeprom_read_byte((uint8_t*)EEPROM_ID_BYTE_2) == 0x44))
        
    )   sysExSetDefaultConf();

}

//private
void OpenDeck::initVariables()  {

    //reset all variables

    //MIDI channels
    _buttonNoteChannel              = 0;
    _longPressButtonNoteChannel     = 0;
    _potCCchannel                   = 0;
    _encCCchannel                   = 0;
    _inputChannel                   = 0;

    //hardware params
    _longPressTime                  = 0;
    _blinkTime                      = 0;
    _startUpLEDswitchTime           = 0;

    //software features
    softwareFeatures                = 0;
    startUpRoutinePattern           = 0;

    //hardware features
    hardwareFeatures                = 0;

    //buttons
    for (i=0; i<MAX_NUMBER_OF_BUTTONS; i++)     {

        buttonNote[i]               = 0;
        previousButtonState[i]      = 0;
        longPressState[i]           = 0;

    }

    for (i=0; i<MAX_NUMBER_OF_BUTTONS/8; i++)   {

        buttonType[i]               = 0;
        buttonPressed[i]            = 0;
        longPressSent[i]            = 0;

    }

    buttonDebounceCompare           = 0;

    //pots
    for (i=0; i<MAX_NUMBER_OF_POTS; i++)        {

        ccNumber[i]                 = 0;
        lastPotNoteValue[i]         = 0;
        lastAnalogueValue[i]        = 0;
        potTimer[i]                 = 0;

    }

    for (i=0; i<MAX_NUMBER_OF_POTS/8; i++)      {

        potInverted[i]              = 0;
        potEnabled[i]               = 0;

    }

    _analogueIn                     = 0;

    //LEDs
    for (i=0; i<MAX_NUMBER_OF_LEDS; i++)        {

        ledState[i]                 = 0;
        ledNote[i]                    = 0;

    }

    totalNumberOfLEDs               = 0;

    blinkState                      = false;
    blinkEnabled                    = false;
    blinkTimerCounter               = 0;

    //input
    receivedNoteProcessed           = false;
    receivedChannel                 = 0;
    receivedNote                    = 0;
    receivedVelocity                = 0;

    //column counter
    column                          = 0;
    
    //sysex
    sysExEnabled                    = false;

    //board type
    _board                          = 0;

}

void OpenDeck::startUpRoutine() {

    switch (startUpRoutinePattern)  {

        case 1:
        openDeck.oneByOneLED(true, true, true);
        openDeck.oneByOneLED(false, false, true);
        openDeck.oneByOneLED(true, false, false);
        openDeck.oneByOneLED(false, true, true);
        openDeck.oneByOneLED(true, false, true);
        openDeck.oneByOneLED(false, false, false);
        openDeck.allLEDsOff();
        break;

        default:
        break;

    }

}


//configuration retrieve

//global configuration getter
void OpenDeck::getConfiguration()   {

    //get configuration from EEPROM
    getMIDIchannels();
    getHardwareParams();
    getSoftwareFeatures();
    getStartUpRoutinePattern();
    getHardwareFeatures();
    getEnabledPots();
    getPotInvertStates();
    getCCnumbers();
    getButtonsType();
    getButtonNotes();
    getLEDnotes();
    getTotalLEDnumber();

}

//individual configuration getters
void OpenDeck::getMIDIchannels()        {

    _buttonNoteChannel          = eeprom_read_byte((uint8_t*)EEPROM_MC_BUTTON_NOTE);
    _longPressButtonNoteChannel = eeprom_read_byte((uint8_t*)EEPROM_MC_LONG_PRESS_BUTTON_NOTE);
    _potCCchannel               = eeprom_read_byte((uint8_t*)EEPROM_MC_POT_CC);
    _encCCchannel               = eeprom_read_byte((uint8_t*)EEPROM_MC_ENC_CC);
    _inputChannel               = eeprom_read_byte((uint8_t*)EEPROM_MC_INPUT);

}

void OpenDeck::getHardwareParams()      {

    _longPressTime              = eeprom_read_byte((uint8_t*)EEPROM_HW_P_LONG_PRESS_TIME) * 100;
    _blinkTime                  = eeprom_read_byte((uint8_t*)EEPROM_HW_P_BLINK_TIME) * 100;
    _startUpLEDswitchTime       = eeprom_read_byte((uint8_t*)EEPROM_HW_P_START_UP_SWITCH_TIME) * 10;

}

void OpenDeck::getSoftwareFeatures()    {

    softwareFeatures = eeprom_read_byte((uint8_t*)EEPROM_SOFTWARE_FEATURES_START);

}

void OpenDeck::getStartUpRoutinePattern()   {
    
    startUpRoutinePattern = eeprom_read_byte((uint8_t*)EEPROM_START_UP_ROUTINE_PATTERN);
    
}

void OpenDeck::getHardwareFeatures()    {

    hardwareFeatures = eeprom_read_byte((uint8_t*)EEPROM_HARDWARE_FEATURES_START);

}

void OpenDeck::getButtonsType()         {

    uint16_t eepromAddress = EEPROM_BUTTON_TYPE_START;

    for (int i=0; i<MAX_NUMBER_OF_BUTTONS/8; i++)   {

        buttonType[i] = eeprom_read_byte((uint8_t*)eepromAddress);
        eepromAddress++;

    }

}

void OpenDeck::getButtonNotes()         {

    uint16_t eepromAddress = EEPROM_BUTTON_NOTE_START;

    for (int i=0; i<MAX_NUMBER_OF_BUTTONS; i++) {

        buttonNote[i] = eeprom_read_byte((uint8_t*)eepromAddress);
        eepromAddress++;

    }

}

void OpenDeck::getEnabledPots()         {

    uint16_t eepromAddress = EEPROM_POT_ENABLED_START;

    for (int i=0; i<(MAX_NUMBER_OF_POTS/8); i++)    {

        potEnabled[i] = eeprom_read_byte((uint8_t*)eepromAddress);
        eepromAddress++;

    }

}

void OpenDeck::getPotInvertStates()     {

    uint16_t eepromAddress = EEPROM_POT_INVERSION_START;

    for (int i=0; i<(MAX_NUMBER_OF_POTS/8); i++)    {

        potInverted[i] = eeprom_read_byte((uint8_t*)eepromAddress);
        eepromAddress++;

    }

}

void OpenDeck::getCCnumbers()           {

    uint16_t eepromAddress = EEPROM_POT_CC_NUMBER_START;

    for (int i=0; i<MAX_NUMBER_OF_POTS; i++)    {

        ccNumber[i] = eeprom_read_byte((uint8_t*)eepromAddress);
        eepromAddress++;

    }

}

void OpenDeck::getLEDnotes()              {

    uint16_t eepromAddress = EEPROM_LED_ACT_NOTE_START;

    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)    {

        ledNote[i] = eeprom_read_byte((uint8_t*)eepromAddress);
        eepromAddress++;

    }

}

void OpenDeck::getTotalLEDnumber()      {

    totalNumberOfLEDs = eeprom_read_byte((uint8_t*)EEPROM_TOTAL_LED_NUMBER);

}


//hardware configuration

//public
void OpenDeck::setHandlePinInit(void (*fptr)()) {

    sendInitPinsCallback = fptr;

}

void OpenDeck::setHandleColumnSwitch(void (*fptr)(uint8_t)) {

    sendColumnSwitchCallback = fptr;

}

void OpenDeck::setNumberOfColumns(uint8_t numberOfColumns)  {

    _numberOfColumns = numberOfColumns;

}

void OpenDeck::setHandleButtonRead(void (*fptr)(uint8_t &buttonColumnState))    {

    sendButtonReadCallback = fptr;

}

void OpenDeck::setHandleMuxOutput(void (*fptr)(uint8_t muxInput))   {

    sendSwitchMuxOutCallback = fptr;

}

void OpenDeck::setHandleLEDrowOn(void (*fptr)(uint8_t ledRow))  {

    sendLEDrowOnCallback = fptr;

}

void OpenDeck::setHandleLEDrowsOff(void (*fptr)())  {

    sendLEDrowsOffCallback = fptr;

}

void OpenDeck::setNumberOfButtonRows(uint8_t numberOfButtonRows)    {

    _numberOfButtonRows = numberOfButtonRows;

}

void OpenDeck::setNumberOfLEDrows(uint8_t numberOfLEDrows)  {

    _numberOfLEDrows = numberOfLEDrows;

}

void OpenDeck::setNumberOfMux(uint8_t numberOfMux)  {

    _numberOfMux = numberOfMux;

}

void OpenDeck::enableAnalogueInput(uint8_t adcChannel)  {

    if ((adcChannel >=0) && (adcChannel < 8))   bitWrite(_analogueIn, adcChannel, 1);

}


//buttons

//private
void OpenDeck::setNumberOfColumnPasses() {

    /*

        Algorithm calculates how many times does it need to read whole row
        before it can declare button reading stable.

    */

    uint8_t rowPassTime = getTimedLoopTime()*_numberOfColumns;
    uint8_t mod = 0;

    if ((BUTTON_DEBOUNCE_TIME % rowPassTime) > 0)   mod = 1;

    uint8_t numberOfColumnPasses = ((BUTTON_DEBOUNCE_TIME / rowPassTime) + mod);

    setButtonDebounceCompare(numberOfColumnPasses);

}

void OpenDeck::setButtonDebounceCompare(uint8_t numberOfColumnPasses)   {

    //depending on numberOfColumnPasses, button state gets shifted into
    //different buttonDebounceCompare variable

    switch(numberOfColumnPasses)    {

        case 1:
        buttonDebounceCompare = 0b11111110;
        break;

        case 2:
        buttonDebounceCompare = 0b11111100;
        break;

        case 3:
        buttonDebounceCompare = 0b11111000;
        break;

        case 4:
        buttonDebounceCompare = 0b11110000;
        break;

        default:
        break;

    }

}

bool OpenDeck::checkButton(uint8_t buttonNumber, uint8_t buttonState)  {

    //shift new button reading into buttonState
    buttonState = (previousButtonState[buttonNumber] << 1) | buttonState | buttonDebounceCompare;

    //if buttonState is changed or if button is debounced, return true
    if ((buttonState != previousButtonState[buttonNumber]) || (buttonState == 0xFF))
        return true;

    return false;

}

void OpenDeck::procesButtonReading(uint8_t buttonNumber, uint8_t buttonState)  {

    if (buttonState)    buttonState = 0xFF;
    else                buttonState = buttonDebounceCompare;
    
    if (buttonState != previousButtonState[buttonNumber])    {

        if (buttonState == 0xFF)    {

            //button is pressed
            //if button is configured as toggle
            if (getButtonType(buttonNumber))    {

                //if a button has been already pressed
                if (getButtonPressed(buttonNumber)) {

                    //if longPress is enabled and longPressNote has already been sent
                    if (getFeature(EEPROM_SOFTWARE_FEATURES_START, EEPROM_SW_LONG_PRESS) && getButtonLongPressed(buttonNumber))   {

                        //send both regular and long press note off
                        sendButtonDataCallback(buttonNote[buttonNumber], false, _buttonNoteChannel);
                        sendButtonDataCallback(buttonNote[buttonNumber], false, _longPressButtonNoteChannel);

                    }   else    sendButtonDataCallback(buttonNote[buttonNumber], false, _buttonNoteChannel);

                    //reset pressed state
                    setButtonPressed(buttonNumber, false);

                    }   else    {

                    //send note on on press
                    sendButtonDataCallback(buttonNote[buttonNumber], true, _buttonNoteChannel);

                    //toggle buttonPressed flag to true
                    setButtonPressed(buttonNumber, true);

                }

            }   else    sendButtonDataCallback(buttonNote[buttonNumber], true, _buttonNoteChannel);

            //start long press timer
            if (getFeature(EEPROM_SOFTWARE_FEATURES_START, EEPROM_SW_LONG_PRESS)) longPressState[buttonNumber] = millis();

            }   else    if ((buttonState == buttonDebounceCompare) && (!getButtonType(buttonNumber)))   {

            //button is released
            //check button on release only if it's momentary

            if (getFeature(EEPROM_SOFTWARE_FEATURES_START, EEPROM_SW_LONG_PRESS)) {

                if (getButtonLongPressed(buttonNumber)) {

                    //send both regular and long press note off
                    sendButtonDataCallback(buttonNote[buttonNumber], false, _buttonNoteChannel);
                    sendButtonDataCallback(buttonNote[buttonNumber], false, _longPressButtonNoteChannel);

                }   else    sendButtonDataCallback(buttonNote[buttonNumber], false, _buttonNoteChannel);

                longPressState[buttonNumber] = 0;
                setButtonLongPressed(buttonNumber, false);

            }   else    sendButtonDataCallback(buttonNote[buttonNumber], false, _buttonNoteChannel);

        }

        //update previous reading with current
        previousButtonState[buttonNumber] = buttonState;

    }

    if (getFeature(EEPROM_SOFTWARE_FEATURES_START, EEPROM_SW_LONG_PRESS)) {

        //send long press note if button has been pressed for defined time and note hasn't already been sent
        if ((millis() - longPressState[buttonNumber] >= _longPressTime) &&
            (!getButtonLongPressed(buttonNumber)) && (buttonState == 0xFF))  {

                sendButtonDataCallback(buttonNote[buttonNumber], true, _longPressButtonNoteChannel);
                setButtonLongPressed(buttonNumber, true);

        }

    }

}

//public
void OpenDeck::setHandleButtonSend(void (*fptr)(uint8_t buttonNumber, bool buttonState, uint8_t channel))   {

    sendButtonDataCallback = fptr;

}

bool OpenDeck::buttonsEnabled() {

    return getFeature(EEPROM_HARDWARE_FEATURES_START, EEPROM_HW_F_BUTTONS);

}

void OpenDeck::readButtons()    {

    uint8_t columnState = 0;

    switch (_board) {

            case BOARD_TANNIN:
            HCTannin::readButtons(columnState);
            break;
            
            case BOARD_OPEN_DECK_1:
            HCOpenDeck1::readButtons(columnState);
            break;

            default:
            sendButtonReadCallback(columnState);
            break;

    }

    //iterate over rows
    for (int i=0; i<_numberOfButtonRows; i++)   {

        //extract current bit from �olumnState variable
        //invert extracted bit because of pull-up resistors
        uint8_t buttonState = !((columnState >> i) & 0x01);

        uint8_t buttonNumber = getActiveColumn()+i*_numberOfColumns;
        
        if (checkButton(buttonNumber, buttonState))
            procesButtonReading(buttonNumber, buttonState);

    }

}


//pots

//public
void OpenDeck::setHandlePotCC(void (*fptr)(uint8_t potNumber, uint8_t ccValue, uint8_t channel))    {

    sendPotCCDataCallback = fptr;

}

void OpenDeck::setHandlePotNoteOn(void (*fptr)(uint8_t note, uint8_t potNumber, uint8_t channel))   {

    sendPotNoteOnDataCallback = fptr;

}

void OpenDeck::setHandlePotNoteOff(void (*fptr)(uint8_t note, uint8_t potNumber, uint8_t channel))  {

    sendPotNoteOffDataCallback = fptr;

}

bool OpenDeck::potsEnabled()    {

    return getFeature(EEPROM_HARDWARE_FEATURES_START, EEPROM_HW_F_POTS);

}

void OpenDeck::readPots()   {

    uint8_t muxNumber = 0;

    //check 8 analogue inputs on ATmega328p
    for (int i=0; i<8; i++) {

        //read mux on selected input if connected
        if (adcConnected(i))    {

            readPotsMux(i, muxNumber);
            muxNumber++;

        }

    }

}

//private
bool OpenDeck::adcConnected(uint8_t adcChannel) {

    //_analogueIn stores 8 variables, for each analogue pin on ATmega328p
    //if variable is true, analogue input is enabled
    //else code doesn't check specified input
    return bitRead(_analogueIn, adcChannel);

}

void OpenDeck::readPotsMux(uint8_t adcChannel, uint8_t muxNumber)  {

    uint8_t potNumber;

    //iterate over 8 inputs on 4051 mux
    for (int i=0; i<8; i++) {

        //enable selected input
        switch (_board) {

            case BOARD_TANNIN:
            HCTannin::setMuxOutput(i);
            break;
                
            case BOARD_OPEN_DECK_1:
            HCOpenDeck1::setMuxOutput(i);
            break;

            default:
            sendSwitchMuxOutCallback(i);
            break;

        }

        //add small delay between setting select pins and reading the input
        NOP;

        //read analogue value from mux
        int16_t tempValue = analogRead(adcChannel);
        
        //calculate pot number
        potNumber = muxNumber*8+i;

        //if new reading is stable, send new MIDI message
        if (checkPotReading(tempValue, potNumber))
        processPotReading(tempValue, potNumber);

    }

}

bool OpenDeck::checkPotReading(int16_t tempValue, uint8_t potNumber) {

    //calculate difference between current and previous reading
    int8_t analogueDiff = tempValue - lastAnalogueValue[potNumber];

    //get absolute difference
    if (analogueDiff < 0)   analogueDiff *= -1;

    uint32_t timeDifference = millis() - potTimer[potNumber];

        /*

            When value from pot hasn't changed for more than POTENTIOMETER_MOVE_TIMEOUT value (time in ms), pot must
            exceed MIDI_CC_STEP_TIMEOUT value. If the value has changed during POTENTIOMETER_MOVE_TIMEOUT, it must
            exceed MIDI_CC_STEP value.

        */

        if (timeDifference < POTENTIOMETER_MOVE_TIMEOUT)    {

            if (analogueDiff >= MIDI_CC_STEP)   return true;

        }   else    {

                if (analogueDiff >= MIDI_CC_STEP_TIMEOUT)   return true;

            }

    return false;

}

void OpenDeck::processPotReading(int16_t tempValue, uint8_t potNumber)  {

    uint8_t ccValue;
    uint8_t potNoteChannel = _longPressButtonNoteChannel+1;

    //invert CC data if potInverted is true
    if (getPotInvertState(potNumber))   ccValue = 127 - (tempValue >> 3);
    else                                ccValue = tempValue >> 3;

    //only send data if pot is enabled and function isn't called in setup
    if ((sendPotCCDataCallback != NULL) && (getPotEnabled(potNumber)))
        sendPotCCDataCallback(ccNumber[potNumber], ccValue, _potCCchannel);

    if (getFeature(EEPROM_SOFTWARE_FEATURES_START, EEPROM_SW_POT_NOTES))  {

        uint8_t noteCurrent = getPotNoteValue(ccValue, ccNumber[potNumber]);

        //maximum number of notes per MIDI channel is 128, with 127 being final
        if (noteCurrent > 127)  {

            //if calculated note is bigger than 127, assign next midi channel
            potNoteChannel += noteCurrent/128;

            //substract 128*number of overflown channels from note
            noteCurrent -= 128*(noteCurrent/128);

        }

        if (checkPotNoteValue(potNumber, noteCurrent))  {

            //always send note off for previous value, except for the first read
            if ((lastPotNoteValue[potNumber] != 128) && (sendPotNoteOffDataCallback != NULL) && (getPotEnabled(potNumber)))
                sendPotNoteOffDataCallback(lastPotNoteValue[ccNumber[potNumber]], ccNumber[potNumber], (_longPressButtonNoteChannel+1));

            //send note on
            if ((sendPotNoteOnDataCallback != NULL) && (getPotEnabled(potNumber)))
                sendPotNoteOnDataCallback(noteCurrent, ccNumber[potNumber], (_longPressButtonNoteChannel+1));

            //update last value with current
            lastPotNoteValue[potNumber] = noteCurrent;;

        }

    }

    //update values
    lastAnalogueValue[potNumber] = tempValue;
    potTimer[potNumber] = millis();

}

uint8_t OpenDeck::getPotNoteValue(uint8_t analogueMIDIvalue, uint8_t potNumber) {

    /*

    Each potentiometer alongside regular CC messages sends 6 different MIDI notes,
    depending on it's position. In the following table x represents the reading from
    pot and right column the MIDI note number:

    x=0:        0
    0<x<32:     1
    32<=x<64:   2
    64<=x<96:   3
    96<=x<127:  4
    x=127:      5

    */

    //variable to hold current modifier value
    uint8_t modifierValue = 6*potNumber;

    switch (analogueMIDIvalue)  {

    case 0:
    modifierValue += 0;
    break;

    case 127:
    modifierValue += 5;
    break;

    default:
    modifierValue += 1 + (analogueMIDIvalue >> 5);
    break;

    }

    return modifierValue;

}

bool OpenDeck::checkPotNoteValue(uint8_t potNumber, uint8_t noteCurrent)    {

    //make sure that modifier value is sent only once while the pot is in specified range
    if (lastPotNoteValue[potNumber] != noteCurrent) return true;

    return false;

}


//LEDs

//public
bool OpenDeck::ledsEnabled()    {

    return getFeature(EEPROM_HARDWARE_FEATURES_START, EEPROM_HW_F_LEDS);

}

void OpenDeck::oneByOneLED(bool ledDirection, bool singleLED, bool turnOn)  {

    /*

    Function accepts three boolean arguments.

    ledDirection:   true means that LEDs will go from left to right, false from right to left
    singleLED:      true means that only one LED will be active at the time, false means that LEDs
                    will turn on one by one until they're all lighted up

    turnOn:         true means that LEDs will be turned on, with all previous LED states being 0
                    false means that all LEDs are lighted up and they turn off one by one, depending
                    on second argument

    */

    //remember last time column was switched
    static uint32_t columnTime = 0;

    //while loop counter
    uint8_t passCounter = 0;

    //reset the timer on each function call
    uint32_t startUpTimer = 0;

    //index of LED to be processed next
    uint8_t ledNumber,
            _ledNumber[MAX_NUMBER_OF_LEDS];
            
    //get LED order for start-up routine
    for (int i=0; i<totalNumberOfLEDs; i++)    _ledNumber[i] = eeprom_read_byte((uint8_t*)EEPROM_LED_START_UP_NUMBER_START+i);

    //if second and third argument of function are set to false or
    //if second argument is set to false and all the LEDs are turned off
    //light up all LEDs
    if ((!singleLED && !turnOn) || (checkLEDsOff() && !turnOn)) allLEDsOn();

    if (turnOn) {

    //This part of code deals with situations when previous function call has been
    //left direction and current one is right and vice versa.

    //On first function call, let's assume the direction was left to right. That would mean
    //that LEDs had to be processed in this order:

    //LED 1
    //LED 2
    //LED 3
    //LED 4

    //Now, when function is finished, LEDs are not reset yet with allLEDsOff() function to keep
    //track of their previous states. Next function call is right to left. On first run with
    //right to left direction, the LED order would be standard LED 4 to LED 1, however, LED 4 has
    //been already turned on by first function call, so we check if its state is already set, and if
    //it is we increment or decrement ledNumber by one, depending on previous and current direction.
    //When function is called second time with direction different than previous one, the number of
    //times it needs to execute is reduced by one, therefore passCounter is incremented.

        //right-to-left direction
        if (!ledDirection)  {
            
            //if last LED is turned on
            if (ledOn(_ledNumber[totalNumberOfLEDs-1]))  {

                //LED index is penultimate LED number
                ledNumber = _ledNumber[totalNumberOfLEDs-2];
                //increment counter since the loop has to run one cycle less
                passCounter++;

            }   else    ledNumber = _ledNumber[totalNumberOfLEDs-1]; //led index is last one if last one isn't already on
            
        }   else //left-to-right direction
        
                //if first LED is already on
                if (ledOn(_ledNumber[0]))    {

                //led index is 1
                ledNumber = _ledNumber[1];
                //increment counter
                passCounter++;

                }   else    ledNumber = _ledNumber[0];

    }   else    {

                    //This is situation when all LEDs are turned on and we're turning them off one by one. Same
                    //logic applies in both cases (see above). In this case we're not checking for whether the LED
                    //is already turned on, but whether it's already turned off.

                    //right-to-left direction
                    if (!ledDirection)  {
                        
                        if (!(ledOn(_ledNumber[totalNumberOfLEDs-1])))   {

                            ledNumber = _ledNumber[totalNumberOfLEDs-2];
                            passCounter++;

                        }   else ledNumber = _ledNumber[totalNumberOfLEDs-1];
                        
                    }   else

                            if (!(ledOn(_ledNumber[0]))) {   //left-to-right direction

                                ledNumber = _ledNumber[1];
                                passCounter++;

                            }   else ledNumber = _ledNumber[0];

        }

    //on first function call, the while loop is called TOTAL_NUMBER_OF_LEDS+1 times
    //to get empty cycle after processing last LED
    while (passCounter < totalNumberOfLEDs+1)   {

        if ((millis() - columnTime) > getTimedLoopTime())   {

            //activate next column
            nextColumn();

            //only process LED after defined time
            if ((millis() - startUpTimer) > _startUpLEDswitchTime)  {

                if (passCounter < totalNumberOfLEDs)    {

                    //if we're turning LEDs on one by one, turn all the other LEDs off
                    if (singleLED && turnOn)            allLEDsOff();

                    //if we're turning LEDs off one by one, turn all the other LEDs on
                    else    if (!turnOn && singleLED)   allLEDsOn();

                    //set LED state depending on turnOn parameter
                    if (turnOn) turnOnLED(ledNumber);
                        else    turnOffLED(ledNumber);

                    //make sure out-of-bound index isn't requested from ledArray
                    if (passCounter < totalNumberOfLEDs-1)  {

                        //right-to-left direction
                        if (!ledDirection)  ledNumber = _ledNumber[totalNumberOfLEDs - 2 - passCounter];

                        //left-to-right direction
                        else    if (passCounter < totalNumberOfLEDs-1)  ledNumber = _ledNumber[passCounter+1];

                    }

                }

            //always increment pass counter
            passCounter++;

            //update timer
            startUpTimer = millis();

        }

            //check if there is any LED to be turned on
            checkLEDs();

            //update last time column was switched
            columnTime = millis();

        }

    }

}

void OpenDeck::allLEDsOn()  {

    //turn on all LEDs
    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)    setConstantLEDstate(i);

}

void OpenDeck::allLEDsOff() {

    //turn off all LEDs
    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)    ledState[i] = 0x00;

}

void OpenDeck::turnOnLED(uint8_t ledNumber) {

    setConstantLEDstate(ledNumber);

}

void OpenDeck::turnOffLED(uint8_t ledNumber)    {

    ledState[ledNumber] = 0x00;

}

void OpenDeck::storeReceivedNote(uint8_t channel, uint8_t note, uint8_t velocity)  {

    receivedChannel = channel;
    receivedNote = note;
    receivedVelocity = velocity;

    receivedNoteProcessed = false;

}

void OpenDeck::checkReceivedNote()  {

    if (!receivedNoteProcessed) setLEDState();

}

void OpenDeck::checkLEDs()  {

    //get currently active column
    uint8_t currentColumn = getActiveColumn();

    if (blinkEnabled)   switchBlinkState();

    //if there is an active LED in current column, turn on LED row
    for (int i=0; i<_numberOfLEDrows; i++)
    if (ledOn(currentColumn+i*_numberOfColumns))    {

        switch (_board) {

            case BOARD_TANNIN:
            HCTannin::ledRowOn(i);
            break;
            
            case BOARD_OPEN_DECK_1:
            HCOpenDeck1::ledRowOn(i);
            break;

            default:
            sendLEDrowOnCallback(i);
            break;

        }

    }

}

//private
bool OpenDeck::ledOn(uint8_t ledNumber) {

    if  (

        ledState[ledNumber] == 0x05 ||
        ledState[ledNumber] == 0x15 ||
        ledState[ledNumber] == 0x16 ||
        ledState[ledNumber] == 0x1D ||
        ledState[ledNumber] == 0x0D ||
        ledState[ledNumber] == 0x17

    )   return true;

    return false;

}

bool OpenDeck::checkLEDsOn()    {

    //return true if all LEDs are on
    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)    if (ledState[i] != 0)   return false;
    return true;

}

bool OpenDeck::checkLEDsOff()   {

    //return true if all LEDs are off
    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)    if (ledState[i] == 0)   return false;
    return true;

}

void OpenDeck::checkBlinkLEDs() {

    //this function will disable blinking
    //if none of the LEDs is in blinking state

    //else it will enable it

    bool _blinkEnabled = false;

    //if any LED is blinking, set timerState to true and exit the loop
    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)
    
    if (checkBlinkState(i)) {

        _blinkEnabled = true;
        break;

    }

    if (_blinkEnabled)  blinkEnabled = true;

    //don't bother reseting variables if blinking is already disabled
    else    if (!_blinkEnabled && blinkEnabled) {

                //reset blinkState to default value
                blinkState = true;
                blinkTimerCounter = 0;
                blinkEnabled = false;

            }

}

bool OpenDeck::checkBlinkState(uint8_t ledNumber)   {

    //function returns true if blinking bit in ledState is set
    return ((ledState[ledNumber] >> 1) & (0x01));

}

void OpenDeck::handleLED(bool currentLEDstate, bool blinkMode) {

    /*

    LED state is stored into one byte (ledState). The bits have following meaning (7 being the MSB bit):

    7: x
    6: x
    5: x
    4: Blink bit (timer changes this bit)
    3: "Remember" bit, used to restore previous LED state
    2: LED is active (either it blinks or it's constantly on), this bit is OR function between bit 0 and 1
    1: LED blinks
    0: LED is constantly turned on

    */

    uint8_t ledNumber = getLEDnumber();
    
        if (ledNumber < 128)    {

            switch (currentLEDstate) {

                case false:
                //note off event
                
                //if remember bit is set
                if ((ledState[ledNumber] >> 3) & (0x01))   {

                    //if note off for blink state is received
                    //clear remember bit and blink bits
                    //set constant state bit
                    if (blinkMode)  ledState[ledNumber] = 0x05;
                    //else clear constant state bit and remember bit
                    //set blink bits
                    else            ledState[ledNumber] = 0x16;

                    }   else    {

                            if (blinkMode)  /*clear blink bit */            ledState[ledNumber] &= 0x15;
                            else            /* clear constant state bit */  ledState[ledNumber] &= 0x16;

                        }

                //if bits 0 and 1 are 0, LED is off so we set ledState to zero
                if (!(ledState[ledNumber] & 3))   ledState[ledNumber] = 0x00;

                break;

                case true:
                //note on event

                //if constant note on is received and LED is already blinking
                //clear blinking bits and set remember bit and constant bit
                if ((!blinkMode) && checkBlinkState(ledNumber))    ledState[ledNumber] = 0x0D;

                //set bit 2 to 1 in any case (constant/blink state)
                else    ledState[ledNumber] |= (0x01 << blinkMode) | 0x04 | (blinkMode << 4);

            }

        }

}

void OpenDeck::setLEDState()    {

    bool currentLEDstate;

    //if blinkMode is 1, the LED is blinking
    uint8_t blinkMode = 0;

    if ((receivedVelocity == SYS_EX_LED_C_OFF_VELOCITY) || (receivedVelocity == SYS_EX_LED_B_OFF_VELOCITY))
        currentLEDstate = false;
        
    else    if (
    
                ((receivedVelocity > SYS_EX_LED_C_OFF_VELOCITY) && (receivedVelocity < SYS_EX_LED_B_ON_VELOCITY)) ||
                ((receivedVelocity > SYS_EX_LED_B_OFF_VELOCITY) && (receivedVelocity < 128))

            )    currentLEDstate = true;

    else return;
    
    if ((receivedVelocity >= SYS_EX_LED_B_ON_VELOCITY) && (receivedVelocity < 128))
        blinkMode = 1;

    handleLED(currentLEDstate, blinkMode);

    if (blinkMode && currentLEDstate)   blinkEnabled = true;
    else    checkBlinkLEDs();

    receivedNoteProcessed = true;

}

void OpenDeck::setConstantLEDstate(uint8_t ledNumber)   {

    ledState[ledNumber] = 0x05;

}

void OpenDeck::setBlinkState(uint8_t ledNumber, bool blinkState)    {

    switch (blinkState) {

        case true:
        ledState[ledNumber] |= 0x10;
        break;

        case false:
        ledState[ledNumber] &= 0xEF;
        break;

    }

}

void OpenDeck::switchBlinkState()   {

    if ((millis() - blinkTimerCounter) >= _blinkTime)   {

        //change blinkBit state and write it into ledState variable if LED is in blink state
        for (int i = 0; i<MAX_NUMBER_OF_LEDS; i++)
        if (checkBlinkState(i)) setBlinkState(i, blinkState);

        //invert blink state
        blinkState = !blinkState;

        //update blink timer
        blinkTimerCounter = millis();

    }

}

uint8_t OpenDeck::getLEDnumber()   {

    //match LED activation note with its index
    for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)
        if (ledNote[i] == receivedNote) return i;

    //since 128 is impossible note, return it in case
    //that received note doesn't match any LED
    return 128;

}


//columns

//public
void OpenDeck::nextColumn() {

    //turn off all LED rows before switching to next column
    switch (_board) {

        case BOARD_TANNIN:
        HCTannin::ledRowsOff();
        break;
        
        case BOARD_OPEN_DECK_1:
        HCOpenDeck1::ledRowsOff();
        break;

        default:
        sendLEDrowsOffCallback();
        break;

    }

    if (column == _numberOfColumns) column = 0;

    switch (_board) {

        case BOARD_TANNIN:
        HCTannin::activateColumn(column);
        break;
        
        case BOARD_OPEN_DECK_1:
        HCOpenDeck1::activateColumn(column);
        break;

        default:
        sendColumnSwitchCallback(column);
        break;

    }

    //increment column
    column++;

}

//private
uint8_t OpenDeck::getActiveColumn() {

    //return currently active column
    return (column - 1);

}


//getters

//public
uint8_t OpenDeck::getInputMIDIchannel() {

    return _inputChannel;

}

bool OpenDeck::standardNoteOffEnabled() {

    return getFeature(EEPROM_SOFTWARE_FEATURES_START, EEPROM_SW_STANDARD_NOTE_OFF);

}

//private
bool OpenDeck::getFeature(uint8_t featureType, uint8_t feature) {

    if (feature >= 8)   return false;

    switch (featureType)    {

        case EEPROM_SOFTWARE_FEATURES_START:
        //software feature
        return bitRead(softwareFeatures, feature);
        break;

        case EEPROM_HARDWARE_FEATURES_START:
        //hardware feature
        return bitRead(hardwareFeatures, feature);
        break;

        default:
        return false;
        break;

    }   return false;

}

bool OpenDeck::getButtonType(uint8_t buttonNumber)  {

    uint8_t arrayIndex = buttonNumber/8;
    uint8_t buttonIndex = buttonNumber - 8*arrayIndex;

    return bitRead(buttonType[arrayIndex], buttonIndex);

}

uint8_t OpenDeck::getButtonNote(uint8_t buttonNumber)   {

    return buttonNote[buttonNumber];

}

bool OpenDeck::getButtonPressed(uint8_t buttonNumber)   {

    uint8_t arrayIndex = buttonNumber/8;
    uint8_t buttonIndex = buttonNumber - 8*arrayIndex;

    return bitRead(buttonPressed[arrayIndex], buttonIndex);

}

bool OpenDeck::getButtonLongPressed(uint8_t buttonNumber)   {

    uint8_t arrayIndex = buttonNumber/8;
    uint8_t buttonIndex = buttonNumber - 8*arrayIndex;

    return bitRead(longPressSent[arrayIndex], buttonIndex);

}

bool OpenDeck::getPotEnabled(uint8_t potNumber) {

    uint8_t arrayIndex = potNumber/8;
    uint8_t potIndex = potNumber - 8*arrayIndex;

    return bitRead(potEnabled[arrayIndex], potIndex);

}

bool OpenDeck::getPotInvertState(uint8_t potNumber) {

    uint8_t arrayIndex = potNumber/8;
    uint8_t potIndex = potNumber - 8*arrayIndex;

    return bitRead(potInverted[arrayIndex], potIndex);

}

uint8_t OpenDeck::getCCnumber(uint8_t potNumber)    {

    return ccNumber[potNumber];

}

uint8_t OpenDeck::getLEDnote(uint8_t ledNumber)   {

    return ledNote[ledNumber];

}


//setters

//private
void OpenDeck::setButtonPressed(uint8_t buttonNumber, bool state)   {

    uint8_t arrayIndex = buttonNumber/8;
    uint8_t buttonIndex = buttonNumber - 8*arrayIndex;

    bitWrite(buttonPressed[arrayIndex], buttonIndex, state);

}

void OpenDeck::setButtonLongPressed(uint8_t buttonNumber, bool state)   {

    uint8_t arrayIndex = buttonNumber/8;
    uint8_t buttonIndex = buttonNumber - 8*arrayIndex;

    bitWrite(longPressSent[arrayIndex], buttonIndex, state);

}


//sysex

//public
void OpenDeck::setHandleSysExSend(void (*fptr)(uint8_t *sysExArray, uint8_t size))  {

    sendSysExDataCallback = fptr;

}

void OpenDeck::processSysEx(uint8_t sysExArray[], uint8_t arrSize)  {

    if (sysExCheckMessageValidity(sysExArray, arrSize))
        sysExGenerateResponse(sysExArray, arrSize);

}

//private
bool OpenDeck::sysExCheckMessageValidity(uint8_t sysExArray[], uint8_t arrSize)  {

    //don't respond to sysex message if device ID is wrong
    if (sysExCheckID(sysExArray[SYS_EX_MS_MID_0], sysExArray[SYS_EX_MS_MID_1], sysExArray[SYS_EX_MS_MID_2]))  {

        //only check rest of the message if it's not just a ID check and controller has received handshake
        if ((arrSize >= SYS_EX_ML_REQ_DATA) && (sysExEnabled))    {

            //check wish validity
            if (sysExCheckWish(sysExArray[SYS_EX_MS_WISH]))    {

                //check if wanted data is for single or all parameters
                if (sysExCheckSingleAll(sysExArray[SYS_EX_MS_SINGLE_ALL])) {

                    //check if message type is correct
                    if (sysExCheckMessageType(sysExArray[SYS_EX_MS_MESSAGE_TYPE])) {

                        //determine minimum message length based on asked parameters
                        if (arrSize < sysExGenerateMinMessageLenght(sysExArray[SYS_EX_MS_WISH],
                                                                    sysExArray[SYS_EX_MS_SINGLE_ALL],
                                                                    sysExArray[SYS_EX_MS_MESSAGE_TYPE]))    {

                            sysExGenerateError(SYS_EX_ERROR_MESSAGE_LENGTH);
                            return false;

                        }

                        //check if subtype is correct
                        if (sysExCheckMessageSubType(sysExArray[SYS_EX_MS_MESSAGE_TYPE], sysExArray[SYS_EX_MS_MESSAGE_SUBTYPE]))   {

                            //check if wanted parameter is valid only if single parameter is specified
                            if (!sysExArray[SYS_EX_MS_SINGLE_ALL]) {

                                if (sysExCheckParameterID(sysExArray[SYS_EX_MS_MESSAGE_TYPE], sysExArray[SYS_EX_MS_PARAMETER_ID]))  {

                                    //if message wish is set, check new parameter
                                    if (sysExArray[SYS_EX_MS_WISH] == SYS_EX_SET) {

                                        if (!sysExCheckNewParameterID(  sysExArray[SYS_EX_MS_MESSAGE_TYPE],
                                                                        sysExArray[SYS_EX_MS_PARAMETER_ID],
                                                                        sysExArray[SYS_EX_MS_NEW_PARAMETER_ID_SINGLE]))   {

                                            sysExGenerateError(SYS_EX_ERROR_NEW_PARAMETER);
                                            return false;

                                        }

                                    }

                            }   else {

                                    //error 5: wrong parameter ID
                                    sysExGenerateError(SYS_EX_ERROR_PARAMETER);
                                    return false;

                                }

                            }   //all parameters
                            else    {

                                //check each new parameter for set command
                                if (sysExArray[SYS_EX_MS_WISH] == SYS_EX_SET) {

                                    uint8_t arrayIndex = SYS_EX_MS_NEW_PARAMETER_ID_ALL;

                                    do  {

                                        if (!sysExCheckNewParameterID(  sysExArray[SYS_EX_MS_MESSAGE_TYPE],
                                                                        sysExArray[SYS_EX_MS_PARAMETER_ID],
                                                                        sysExArray[arrayIndex])) {

                                            sysExGenerateError(SYS_EX_ERROR_NEW_PARAMETER);
                                            return false;

                                        }

                                        arrayIndex++;

                                    } while (arrayIndex != (arrSize-1));

                                }

                            }

                            }   else {

                            sysExGenerateError(SYS_EX_ERROR_MESSAGE_SUBTYPE);
                            return false;

                        }

                        }   else {

                        sysExGenerateError(SYS_EX_ERROR_MESSAGE_TYPE);
                        return false;

                    }

                    }   else {

                    sysExGenerateError(SYS_EX_ERROR_SINGLE_ALL);
                    return false;

                }

                }   else {

                sysExGenerateError(SYS_EX_ERROR_WISH);
                return false;

            }

            }   else    {

                    if (arrSize != SYS_EX_ML_REQ_HANDSHAKE) {

                        if (sysExEnabled)   sysExGenerateError(SYS_EX_ERROR_MESSAGE_LENGTH);    //message is too short
                        else                sysExGenerateError(SYS_EX_ERROR_HANDSHAKE);         //handshake hasn't been received
                        return false;

                    }

                }

        return true;

    }

    return false;

}

bool OpenDeck::sysExCheckID(uint8_t firstByte, uint8_t secondByte, uint8_t thirdByte)   {

    return  (

        (firstByte == SYS_EX_MANUFACTURER_ID_0)     &&
        (secondByte == SYS_EX_MANUFACTURER_ID_1)    &&
        (thirdByte == SYS_EX_MANUFACTURER_ID_2)

    );

}

bool OpenDeck::sysExCheckWish(uint8_t wish)   {

    if ((wish == SYS_EX_GET) || (wish == SYS_EX_SET) || (wish == SYS_EX_RESTORE)) return true;
    return false;

}

bool OpenDeck::sysExCheckSingleAll(uint8_t parameter)    {

    return ((parameter == SYS_EX_GET_SET_ALL) || (parameter == SYS_EX_GET_SET_SINGLE));

}

bool OpenDeck::sysExCheckMessageType(uint8_t messageID) {

    if  (
    
        (messageID == SYS_EX_MIDI_CHANNEL_START)    ||
        (messageID == SYS_EX_HW_PARAMETER_START)    ||
        (messageID == SYS_EX_SW_FEATURE_START)      ||
        (messageID == SYS_EX_HW_FEATURE_START)      ||
        (messageID == SYS_EX_BUTTON_START)          ||
        (messageID == SYS_EX_POT_START)             ||
        (messageID == SYS_EX_ENC_START)             ||
        (messageID == SYS_EX_LED_START)

    )   return true;

    return false;

}

bool OpenDeck::sysExCheckMessageSubType(uint8_t messageType, uint8_t messageSubType)    {

    switch (messageType)    {

        case SYS_EX_MIDI_CHANNEL_START:
        return (messageSubType == 0);
        break;

        case SYS_EX_HW_PARAMETER_START:
        return (messageSubType == 0);
        break;

        case SYS_EX_SW_FEATURE_START:
        return (messageSubType == 0);
        break;

        case SYS_EX_HW_FEATURE_START:
        return (messageSubType == 0);
        break;

        case SYS_EX_LED_START:
        return (messageSubType == SYS_EX_GET_SET_LED_ID);
        break;

        case SYS_EX_POT_START:
        return  (

            (messageSubType == SYS_EX_GET_SET_POT_ENABLED)  ||
            (messageSubType == SYS_EX_GET_SET_POT_INVERTED) ||
            (messageSubType == SYS_EX_GET_SET_POT_CC_NUMBER)

        );
        break;

        case SYS_EX_BUTTON_START:
        return ((messageSubType == SYS_EX_GET_SET_BUTTON_TYPE) || (messageSubType == SYS_EX_GET_SET_BUTTON_NOTE));
        break;

        case SYS_EX_ENC_START:
        //to-do
        return false;
        break;

        default:
        return false;

    }

}

bool OpenDeck::sysExCheckParameterID(uint8_t messageType, uint8_t parameter)   {

    switch (messageType)    {

        case SYS_EX_MIDI_CHANNEL_START:
        return  (

            (parameter == SYS_EX_MC_BUTTON_NOTE)               ||
            (parameter == SYS_EX_MC_LONG_PRESS_BUTTON_NOTE)    ||
            (parameter == SYS_EX_MC_POT_CC)                    ||
            (parameter == SYS_EX_MC_ENC_CC)                    ||
            (parameter == SYS_EX_MC_INPUT)

        );

        break;

        case SYS_EX_HW_PARAMETER_START:
        return  (

           (parameter == SYS_EX_HW_P_LONG_PRESS_TIME)         ||
           (parameter == SYS_EX_HW_P_BLINK_TIME)              ||
           (parameter == SYS_EX_HW_P_START_UP_SWITCH_TIME)
           
        );

        break;

        case SYS_EX_SW_FEATURE_START:
        return  (

            (parameter == SYS_EX_SW_F_RUNNING_STATUS)          ||
            (parameter == SYS_EX_SW_F_STANDARD_NOTE_OFF)       ||
            (parameter == SYS_EX_SW_F_ENC_NOTES)               ||
            (parameter == SYS_EX_SW_F_POT_NOTES)               ||
            (parameter == SYS_EX_SW_F_LONG_PRESS)              ||
            (parameter == SYS_EX_SW_F_LED_BLINK)               ||
            (parameter == SYS_EX_SW_F_START_UP_ROUTINE)
        );

        break;

        case SYS_EX_HW_FEATURE_START:
        return  (

            (parameter == SYS_EX_HW_F_BUTTONS)                 ||
            (parameter == SYS_EX_HW_F_POTS)                    ||
            (parameter == SYS_EX_HW_F_ENC)                     ||
            (parameter == SYS_EX_HW_F_LEDS)
        ); 

        break;

        case SYS_EX_BUTTON_START:
        return   (parameter < MAX_NUMBER_OF_BUTTONS);
        break;

        case SYS_EX_POT_START:
        return   (parameter < MAX_NUMBER_OF_POTS);
        break;

        case SYS_EX_ENC_START:
        return   (parameter < (MAX_NUMBER_OF_BUTTONS/2));
        break;

        case SYS_EX_LED_START:
        return   (parameter < MAX_NUMBER_OF_LEDS);
        break;

        default:
        return false;
        break;

    }

    return false;

}

bool OpenDeck::sysExCheckNewParameterID(uint8_t messageType, uint8_t parameter, uint8_t newParameter) {

    switch (messageType)    {

        case SYS_EX_MIDI_CHANNEL_START:
        return ((newParameter >= 1) && (newParameter <= 16));   //there are only 16 MIDI channels
        break;

        case SYS_EX_HW_PARAMETER_START:

        switch (parameter)    {
                
                case SYS_EX_HW_P_LONG_PRESS_TIME:
                return ((newParameter >= 4) && (newParameter <= 15));
                break;

                case SYS_EX_HW_P_BLINK_TIME:
                return ((newParameter >= 1) && (newParameter <= 15));
                break;

                case SYS_EX_HW_P_START_UP_SWITCH_TIME:
                return ((newParameter >= 1) && (newParameter <= 150));
                break;

                default:
                return false;
                break;

            }

        break;

        case SYS_EX_SW_FEATURE_START:
        case SYS_EX_HW_FEATURE_START:
        return ((newParameter == 0) || (newParameter == 1));
        break;

        case SYS_EX_BUTTON_START:
        case SYS_EX_POT_START:
        case SYS_EX_ENC_START:
        case SYS_EX_LED_START:
        return (newParameter < 128);
        break;

        default:
        return false;
        break;

    }

    return false;
}

void OpenDeck::sysExGenerateError(uint8_t errorNumber)  {

    uint8_t sysExResponse[5];

    sysExResponse[0] = SYS_EX_MANUFACTURER_ID_0;
    sysExResponse[1] = SYS_EX_MANUFACTURER_ID_1;
    sysExResponse[2] = SYS_EX_MANUFACTURER_ID_2;
    sysExResponse[3] = SYS_EX_ERROR;
    sysExResponse[4] = errorNumber;

    sendSysExDataCallback(sysExResponse, 5);

}

void OpenDeck::sysExGenerateAck()   {
    
    uint8_t sysExAckResponse[4];
    
    sysExAckResponse[0] = SYS_EX_MANUFACTURER_ID_0;
    sysExAckResponse[1] = SYS_EX_MANUFACTURER_ID_1;
    sysExAckResponse[2] = SYS_EX_MANUFACTURER_ID_2;
    sysExAckResponse[3] = SYS_EX_ACK;
    
    sysExEnabled = true;

    sendSysExDataCallback(sysExAckResponse, 4);
    
}

uint8_t OpenDeck::sysExGenerateMinMessageLenght(bool wish, bool singleAll, uint8_t messageType)    {

    //single parameter
    if (!singleAll)  {

        if (!wish)      return SYS_EX_ML_REQ_DATA + 1;      //get   //add 1 to length for parameter
            else        return SYS_EX_ML_REQ_DATA + 2;      //set   //add 2 to length for parameter and new value

    }   else    {

            if (!wish)    return SYS_EX_ML_REQ_DATA;      //get
                else    {                                   //set

                    switch (messageType)    {

                        case SYS_EX_MIDI_CHANNEL_START:
                        return SYS_EX_ML_REQ_DATA + NUMBER_OF_MIDI_CHANNELS;
                        break;

                        case SYS_EX_HW_PARAMETER_START:
                        return SYS_EX_ML_REQ_DATA + NUMBER_OF_HW_P;
                        break;

                        case SYS_EX_SW_FEATURE_START:
                        return SYS_EX_ML_REQ_DATA + NUMBER_OF_SW_F;
                        break;

                        case SYS_EX_HW_FEATURE_START:
                        return SYS_EX_ML_REQ_DATA + NUMBER_OF_HW_F;
                        break;

                        default:
                        return 0;
                        break;

                    }

                }

        }   return 0;

}

void OpenDeck::sysExGenerateResponse(uint8_t sysExArray[], uint8_t arrSize)  {

    if (arrSize == SYS_EX_ML_REQ_HANDSHAKE) {

        sysExGenerateAck();
        return;

    }

    uint8_t sysExWish           = sysExArray[SYS_EX_MS_WISH],
            sysExSingleAll      = sysExArray[SYS_EX_MS_SINGLE_ALL],
            sysExMessageType    = sysExArray[SYS_EX_MS_MESSAGE_TYPE],
            sysExMessageSubType = sysExArray[SYS_EX_MS_MESSAGE_SUBTYPE],
            sysExParameterID    = sysExArray[SYS_EX_MS_PARAMETER_ID],
            componentNr         = 1,
            maxComponentNr      = 0,
            _parameter          = 0;

    //create basic response
    uint8_t sysExResponse[128+SYS_EX_ML_REQ_DATA];

    sysExResponse[0] = SYS_EX_MANUFACTURER_ID_0;
    sysExResponse[1] = SYS_EX_MANUFACTURER_ID_1;
    sysExResponse[2] = SYS_EX_MANUFACTURER_ID_2;

    sysExResponse[3] = SYS_EX_ACK;

    sysExResponse[4] = sysExWish;
    sysExResponse[5] = sysExSingleAll;
    
    sysExResponse[6] = sysExMessageType;
    sysExResponse[7] = sysExMessageSubType;
    
    switch (sysExMessageType)   {
        
        case SYS_EX_MIDI_CHANNEL_START:
        maxComponentNr = NUMBER_OF_MIDI_CHANNELS;
        break;
        
        case SYS_EX_HW_PARAMETER_START:
        maxComponentNr = NUMBER_OF_HW_P;
        break;
        
        case SYS_EX_SW_FEATURE_START:
        maxComponentNr = NUMBER_OF_SW_F;
        break;
        
        case SYS_EX_HW_FEATURE_START:
        maxComponentNr = NUMBER_OF_HW_F;
        break;
        
        case SYS_EX_BUTTON_START:
        maxComponentNr = MAX_NUMBER_OF_BUTTONS;
        break;
        
        case SYS_EX_POT_START:
        maxComponentNr = MAX_NUMBER_OF_POTS;
        break;
        
        case SYS_EX_ENC_START:
        maxComponentNr = MAX_NUMBER_OF_BUTTONS/2;
        break;
        
        case SYS_EX_LED_START:
        maxComponentNr = MAX_NUMBER_OF_LEDS;
        break;
        
        default:
        break;
        
    }

    if (sysExSingleAll) componentNr = maxComponentNr;

    //create response based on wanted message type
    if (sysExWish == SYS_EX_GET)    {

        if (sysExSingleAll) _parameter = 0;
        else                _parameter = sysExParameterID;

        for (int i=0; i<componentNr; i++) {

            sysExResponse[i+SYS_EX_ML_RES_BASIC] = sysExGet(sysExMessageType, sysExMessageSubType, _parameter);
            _parameter++;

        }

        sendSysExDataCallback(sysExResponse, SYS_EX_ML_RES_BASIC+componentNr);
        return;

    }   else    if (sysExWish == SYS_EX_SET)   {

                    uint8_t arrayIndex;

                    if (sysExSingleAll) {
                        
                        _parameter = i;
                        arrayIndex = SYS_EX_MS_NEW_PARAMETER_ID_ALL;
                        
                    }   else    {

                            _parameter = sysExParameterID;
                            arrayIndex = SYS_EX_MS_NEW_PARAMETER_ID_SINGLE;

                        }

                    for (int i=0; i<componentNr; i++)   {

                        if (!sysExSet(sysExMessageType, sysExMessageSubType, _parameter, sysExArray[arrayIndex+i]))  {

                            sysExGenerateError(SYS_EX_ERROR_EEPROM);
                            return;

                        }
                        
                        _parameter++;

                    }

                    sendSysExDataCallback(sysExResponse, SYS_EX_ML_RES_BASIC);
                    return;

                }
}

//getters
bool OpenDeck::sysExGetFeature(uint8_t featureType, uint8_t feature)    {

    uint8_t _featureType;

    switch (featureType)    {

        case SYS_EX_SW_FEATURE_START:
        _featureType = EEPROM_SOFTWARE_FEATURES_START;
        break;
        
        case SYS_EX_HW_FEATURE_START:
        _featureType = EEPROM_HARDWARE_FEATURES_START;
        break;

        default:
        return false;
        break;

    }

    return getFeature(_featureType, feature);

}

uint8_t OpenDeck::sysExGetHardwareParameter(uint8_t parameter)  {

    switch (parameter)  {

        case SYS_EX_HW_P_LONG_PRESS_TIME:
        //long press time
        return _longPressTime/100;
        break;

        case SYS_EX_HW_P_BLINK_TIME:
        //blink time
        return _blinkTime/100;
        break;

        case SYS_EX_HW_P_START_UP_SWITCH_TIME:
        //start-up led switch time
        return _startUpLEDswitchTime/10;
        break;

        default:
        break;

    }   return 0;

}

uint8_t OpenDeck::sysExGetMIDIchannel(uint8_t channel)  {

    switch (channel)    {

        case SYS_EX_MC_BUTTON_NOTE:
        return _buttonNoteChannel;
        break;

        case SYS_EX_MC_LONG_PRESS_BUTTON_NOTE:
        return _longPressButtonNoteChannel;
        break;

        case SYS_EX_MC_POT_CC:
        return _potCCchannel;
        break;

        case SYS_EX_MC_ENC_CC:
        return _encCCchannel;
        break;

        case SYS_EX_MC_INPUT:
        return _inputChannel;
        break;

        default:
        return 0;
        break;

    }

}

uint8_t OpenDeck::sysExGet(uint8_t messageType, uint8_t messageSubType, uint8_t parameter)  {

    switch (messageType)    {

        case SYS_EX_MIDI_CHANNEL_START:
        return sysExGetMIDIchannel(parameter);
        break;

        case SYS_EX_HW_PARAMETER_START:
        return sysExGetHardwareParameter(parameter);
        break;

        case SYS_EX_SW_FEATURE_START:
        case SYS_EX_HW_FEATURE_START:
        return getFeature(messageType, parameter);
        break;
        
        case SYS_EX_BUTTON_START:
        if (messageSubType == SYS_EX_GET_SET_BUTTON_TYPE)       return getButtonType(parameter);
        else                                                    return buttonNote[parameter];
        break;

        case SYS_EX_POT_START:
        if (messageSubType == SYS_EX_GET_SET_POT_ENABLED)       return getPotEnabled(parameter);
        else if (messageSubType == SYS_EX_GET_SET_POT_INVERTED) return getPotInvertState(parameter);
        else                                                    return ccNumber[parameter];
        break;

        default:
        return false;
        break;

    }

}

bool OpenDeck::sysExSet(uint8_t messageType, uint8_t messageSubType, uint8_t parameter, uint8_t newParameter)    {

    switch (messageType)    {
        
        case SYS_EX_MIDI_CHANNEL_START:
        return sysExSetMIDIchannel(parameter, newParameter);
        break;
        
        case SYS_EX_HW_PARAMETER_START:
        return sysExSetHardwareParameter(parameter, newParameter);
        break;
        
        case SYS_EX_SW_FEATURE_START:
        case SYS_EX_HW_FEATURE_START:
        return sysExSetFeature(messageType, parameter, newParameter);
        break;
        
        case SYS_EX_BUTTON_START:
        if (messageSubType == SYS_EX_GET_SET_BUTTON_TYPE)       return sysExSetButtonType(parameter, newParameter);
        else                                                    return sysExSetButtonNote(parameter, newParameter);
        break;
        
        case SYS_EX_POT_START:
        if (messageSubType == SYS_EX_GET_SET_POT_ENABLED)       return sysExSetPotEnabled(parameter, newParameter);
        else if (messageSubType == SYS_EX_GET_SET_POT_INVERTED) return sysExSetPotInvertState(parameter, newParameter);
        else                                                    return sysExSetCCnumber(parameter, newParameter);

        default:
        return 0;
        break;

    }
    
}

//setters
bool OpenDeck::sysExSetFeature(uint8_t featureType, uint8_t feature, bool state)    {

    switch (featureType)    {

        case SYS_EX_SW_FEATURE_START:
        //software feature
        bitWrite(softwareFeatures, feature, state);
        eeprom_update_byte((uint8_t*)EEPROM_SOFTWARE_FEATURES_START, softwareFeatures);
        return (eeprom_read_byte((uint8_t*)EEPROM_SOFTWARE_FEATURES_START) == softwareFeatures);
        break;

        case SYS_EX_HW_FEATURE_START:
        //hardware feature
        bitWrite(hardwareFeatures, feature, state);
        eeprom_update_byte((uint8_t*)EEPROM_HARDWARE_FEATURES_START, hardwareFeatures);
        return (eeprom_read_byte((uint8_t*)EEPROM_HARDWARE_FEATURES_START) == hardwareFeatures);
        break;

        default:
        break;

    }   return false;

}

bool OpenDeck::sysExSetHardwareParameter(uint8_t parameter, uint8_t value)  {

    switch (parameter)  {

        case SYS_EX_HW_P_LONG_PRESS_TIME:
        //long press time
        _longPressTime = value*100;
        eeprom_update_byte((uint8_t*)EEPROM_HW_P_LONG_PRESS_TIME, value);
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_LONG_PRESS_TIME) == value);
        return true;
        break;

        case SYS_EX_HW_P_BLINK_TIME:
        //blink time
        _blinkTime = value*100;
        eeprom_update_byte((uint8_t*)EEPROM_HW_P_BLINK_TIME, value);
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_BLINK_TIME) == value);
        break;

        case SYS_EX_HW_P_START_UP_SWITCH_TIME:
        //start-up led switch time
        _startUpLEDswitchTime = value*10;
        eeprom_update_byte((uint8_t*)EEPROM_HW_P_START_UP_SWITCH_TIME, value);
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_START_UP_SWITCH_TIME) == value);
        break;

        default:
        break;

    }   return false;

}

bool OpenDeck::sysExSetButtonType(uint8_t buttonNumber, bool type)  {

    uint8_t arrayIndex = buttonNumber/8;
    uint8_t buttonIndex = buttonNumber - 8*arrayIndex;
    uint16_t eepromAddress = EEPROM_BUTTON_TYPE_START+arrayIndex;

    bitWrite(buttonType[arrayIndex], buttonIndex, type);
    eeprom_update_byte((uint8_t*)eepromAddress, buttonType[arrayIndex]);

    return (buttonType[arrayIndex] == eeprom_read_byte((uint8_t*)eepromAddress));

}

bool OpenDeck::sysExSetButtonNote(uint8_t buttonNumber, uint8_t _buttonNote)    {

    uint16_t eepromAddress = EEPROM_BUTTON_NOTE_START+buttonNumber;

    buttonNote[buttonNumber] = _buttonNote;
    eeprom_update_byte((uint8_t*)eepromAddress, _buttonNote);
    return (_buttonNote == eeprom_read_byte((uint8_t*)eepromAddress));

}

bool OpenDeck::sysExSetPotEnabled(uint8_t potNumber, bool state)    {

    uint8_t arrayIndex = potNumber/8;
    uint8_t potIndex = potNumber - 8*arrayIndex;
    uint16_t eepromAddress = EEPROM_POT_ENABLED_START+arrayIndex;

    bitWrite(potEnabled[arrayIndex], potIndex, state);
    eeprom_update_byte((uint8_t*)eepromAddress, potEnabled[arrayIndex]);

    return (potEnabled[arrayIndex] == eeprom_read_byte((uint8_t*)eepromAddress));

}

bool OpenDeck::sysExSetPotInvertState(uint8_t potNumber, bool state)    {

    uint8_t arrayIndex = potNumber/8;
    uint8_t potIndex = potNumber - 8*arrayIndex;
    uint16_t eepromAddress = EEPROM_POT_INVERSION_START+arrayIndex;

    bitWrite(potInverted[arrayIndex], potIndex, state);
    eeprom_update_byte((uint8_t*)eepromAddress, potInverted[arrayIndex]);

    return (potInverted[arrayIndex] == eeprom_read_byte((uint8_t*)eepromAddress));

}

bool OpenDeck::sysExSetCCnumber(uint8_t potNumber, uint8_t _ccNumber)   {

    uint16_t eepromAddress = EEPROM_POT_CC_NUMBER_START+potNumber;

    ccNumber[potNumber] = _ccNumber;
    eeprom_update_byte((uint8_t*)eepromAddress, _ccNumber);
    return (_ccNumber == eeprom_read_byte((uint8_t*)eepromAddress));

}

bool OpenDeck::sysExSetAllPotsEnable()  {

    uint16_t eepromAddres = EEPROM_POT_ENABLED_START;

    for (int i=0; i<MAX_NUMBER_OF_POTS/8; i++)  {

        potEnabled[i] = 0xFF;
        eeprom_update_byte((uint8_t*)eepromAddres+i, potEnabled[i]);
        if (!(eeprom_read_byte((uint8_t*)eepromAddres+i) == potEnabled[i])) return false;

    }   return true;

}

bool OpenDeck::sysExSetAllPotsDisable() {

    uint16_t eepromAddres = EEPROM_POT_ENABLED_START;

    for (int i=0; i<MAX_NUMBER_OF_POTS/8; i++)  {

        potEnabled[i] = 0;
        eeprom_update_byte((uint8_t*)eepromAddres+i, potEnabled[i]);
        if (!(eeprom_read_byte((uint8_t*)eepromAddres+i) == potEnabled[i])) return false;

    }   return true;

}

bool OpenDeck::sysExSetLEDnote(uint8_t ledNumber, uint8_t _ledNote) {

    uint16_t eepromAddress = EEPROM_LED_ACT_NOTE_START+ledNumber;

    ledNote[ledNumber] = _ledNote;
    eeprom_update_byte((uint8_t*)eepromAddress, _ledNote);
    return _ledNote == eeprom_read_byte((uint8_t*)eepromAddress);

}

bool OpenDeck::sysExSetMIDIchannel(uint8_t channel, uint8_t channelNumber)  {

    switch (channel)    {

        case SYS_EX_MC_BUTTON_NOTE:
        _buttonNoteChannel = channelNumber;
        eeprom_update_byte((uint8_t*)EEPROM_MC_BUTTON_NOTE, channelNumber);
        return (channelNumber == eeprom_read_byte((uint8_t*)EEPROM_MC_BUTTON_NOTE));
        break;

        case SYS_EX_MC_LONG_PRESS_BUTTON_NOTE:
        _longPressButtonNoteChannel = channelNumber;
        eeprom_update_byte((uint8_t*)EEPROM_MC_LONG_PRESS_BUTTON_NOTE, channelNumber);
        return (channelNumber == eeprom_read_byte((uint8_t*)EEPROM_MC_LONG_PRESS_BUTTON_NOTE));
        break;

        case SYS_EX_MC_POT_CC:
        _potCCchannel = channelNumber;
        eeprom_update_byte((uint8_t*)EEPROM_MC_POT_CC, channelNumber);
        return (channelNumber == eeprom_read_byte((uint8_t*)EEPROM_MC_POT_CC));
        break;

        case SYS_EX_MC_ENC_CC:
        _encCCchannel = channelNumber;
        eeprom_update_byte((uint8_t*)EEPROM_MC_ENC_CC, channelNumber);
        return (channelNumber == eeprom_read_byte((uint8_t*)EEPROM_MC_ENC_CC));
        break;

        case SYS_EX_MC_INPUT:
        _inputChannel = channelNumber;
        eeprom_update_byte((uint8_t*)EEPROM_MC_INPUT, channelNumber);
        return (channelNumber == eeprom_read_byte((uint8_t*)EEPROM_MC_INPUT));
        break;

        default:
        return false;

    }

}

void OpenDeck::sysExSetDefaultConf()    {

    //write default configuration stored in PROGMEM to EEPROM
    for (int i=0; i<(int16_t)sizeof(defConf); i++)
    eeprom_update_byte((uint8_t*)i, pgm_read_byte(&(defConf[i])));

}

//create instance of library automatically
OpenDeck openDeck;