/*

OpenDECK library v0.99
File: SysEx.cpp
Last revision date: 2014-09-15
Author: Igor Petrovic

*/

#include "OpenDeck.h"
#include <avr/eeprom.h>
#include "Ownduino.h"

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
    if (sysExCheckID(sysExArray[SYS_EX_MS_M_ID_0], sysExArray[SYS_EX_MS_M_ID_1], sysExArray[SYS_EX_MS_M_ID_2]))  {

        //only check rest of the message if it's not just a ID check and controller has received handshake
        if ((arrSize >= SYS_EX_ML_REQ_DATA) && (sysExEnabled))    {

            //check wish validity
            if (sysExCheckWish(sysExArray[SYS_EX_MS_WISH]))    {

                //check if wanted data is for single or all parameters
                if (sysExCheckSingleAll(sysExArray[SYS_EX_MS_AMOUNT])) {

                    //check if message type is correct
                    if (sysExCheckMessageType(sysExArray[SYS_EX_MS_MT])) {

                        //determine minimum message length based on asked parameters
                        if (arrSize < sysExGenerateMinMessageLenght(sysExArray[SYS_EX_MS_WISH],
                        sysExArray[SYS_EX_MS_AMOUNT],
                        sysExArray[SYS_EX_MS_MT]))    {

                            sysExGenerateError(SYS_EX_ERROR_MESSAGE_LENGTH);
                            return false;

                        }

                        //check if subtype is correct
                        if (sysExCheckMessageSubType(sysExArray[SYS_EX_MS_MT], sysExArray[SYS_EX_MS_MST]))   {

                            //check if wanted parameter is valid only if single parameter is specified
                            if (!sysExArray[SYS_EX_MS_AMOUNT]) {

                                if (sysExCheckParameterID(sysExArray[SYS_EX_MS_MT], sysExArray[SYS_EX_MS_MST], sysExArray[SYS_EX_MS_PARAMETER_ID]))  {

                                    //if message wish is set, check new parameter
                                    if (sysExArray[SYS_EX_MS_WISH] == SYS_EX_WISH_SET) {

                                        if (!sysExCheckNewParameterID(  sysExArray[SYS_EX_MS_MT],
                                                                        sysExArray[SYS_EX_MS_MST],
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

                            }   else    {   //all parameters

                                    //check each new parameter for set command
                                    if (sysExArray[SYS_EX_MS_WISH] == SYS_EX_WISH_SET) {

                                        uint8_t arrayIndex = SYS_EX_MS_NEW_PARAMETER_ID_ALL;

                                        for (int i=0; i<(arrSize - arrayIndex); i++)

                                        if (!sysExCheckNewParameterID(  sysExArray[SYS_EX_MS_MT],
                                                                        sysExArray[SYS_EX_MS_MST],
                                                                        i,
                                                                        sysExArray[arrayIndex+i]))   {

                                            sysExGenerateError(SYS_EX_ERROR_NEW_PARAMETER);
                                            return false;

                                         }

                                    }

                                }

                        }  else {

                                sysExGenerateError(SYS_EX_ERROR_MST);
                                return false;

                            }

                        }   else {

                                sysExGenerateError(SYS_EX_ERROR_MT);
                                return false;

                            }

                    }   else {

                            sysExGenerateError(SYS_EX_ERROR_AMOUNT);
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

    (firstByte  == SYS_EX_M_ID_0)   &&
    (secondByte == SYS_EX_M_ID_1)   &&
    (thirdByte  == SYS_EX_M_ID_2)

    );

}

bool OpenDeck::sysExCheckWish(uint8_t wish)   {

    if ((wish == SYS_EX_WISH_GET) || (wish == SYS_EX_WISH_SET) || (wish == SYS_EX_WISH_RESTORE)) return true;
    return false;

}

bool OpenDeck::sysExCheckSingleAll(uint8_t parameter)    {

    return ((parameter == SYS_EX_AMOUNT_ALL) || (parameter == SYS_EX_AMOUNT_SINGLE));

}

bool OpenDeck::sysExCheckMessageType(uint8_t messageID) {

    if  (

    (messageID == SYS_EX_MT_MIDI_CHANNEL)    ||
    (messageID == SYS_EX_MT_HW_PARAMETER)    ||
    (messageID == SYS_EX_MT_FREE_PINS)       ||
    (messageID == SYS_EX_MT_SW_FEATURE)      ||
    (messageID == SYS_EX_MT_HW_FEATURE)      ||
    (messageID == SYS_EX_MT_BUTTON)          ||
    (messageID == SYS_EX_MT_POT)             ||
    (messageID == SYS_EX_MT_ENC)             ||
    (messageID == SYS_EX_MT_LED)
    

    )   return true;

    return false;

}

bool OpenDeck::sysExCheckMessageSubType(uint8_t messageType, uint8_t messageSubType)    {

    switch (messageType)    {

        case SYS_EX_MT_MIDI_CHANNEL:
        return (messageSubType == 0);
        break;

        case SYS_EX_MT_HW_PARAMETER:
        return (messageSubType == 0);
        break;

        case SYS_EX_MT_FREE_PINS:
        return (messageSubType == 0);
        break;

        case SYS_EX_MT_SW_FEATURE:
        return (messageSubType == 0);
        break;

        case SYS_EX_MT_HW_FEATURE:
        return (messageSubType == 0);
        break;

        case SYS_EX_MT_BUTTON:
        return ((messageSubType == SYS_EX_MST_BUTTON_TYPE) || (messageSubType == SYS_EX_MST_BUTTON_NOTE));
        break;

        case SYS_EX_MT_POT:
        return  (

        (messageSubType == SYS_EX_MST_POT_ENABLED)  ||
        (messageSubType == SYS_EX_MST_POT_INVERTED) ||
        (messageSubType == SYS_EX_MST_POT_CC_NUMBER)

        );

        break;

        case SYS_EX_MT_ENC:
        //to-do
        return false;
        break;

        case SYS_EX_MT_LED:
        return  (

        (messageSubType == SYS_EX_MST_LED_ACT_NOTE)         ||
        (messageSubType == SYS_EX_MST_LED_START_UP_NUMBER)  ||
        (messageSubType == SYS_EX_MST_LED_STATE)

        );

        break;

        default:
        return false;

    }

}

bool OpenDeck::sysExCheckParameterID(uint8_t messageType, uint8_t messageSubType, uint8_t parameter)   {

    switch (messageType)    {

        case SYS_EX_MT_MIDI_CHANNEL:
        return  (

        (parameter == SYS_EX_MC_BUTTON_NOTE)                ||
        (parameter == SYS_EX_MC_LONG_PRESS_BUTTON_NOTE)     ||
        (parameter == SYS_EX_MC_POT_CC)                     ||
        (parameter == SYS_EX_MC_ENC_CC)                     ||
        (parameter == SYS_EX_MC_INPUT)

        );

        break;

        case SYS_EX_MT_HW_PARAMETER:
        return  (

        (parameter == SYS_EX_HW_P_BOARD_TYPE)               ||
        (parameter == SYS_EX_HW_P_LONG_PRESS_TIME)          ||
        (parameter == SYS_EX_HW_P_BLINK_TIME)               ||
        (parameter == SYS_EX_HW_P_START_UP_SWITCH_TIME)     ||
        (parameter == SYS_EX_HW_P_START_UP_ROUTINE)||
        (parameter == SYS_EX_HW_P_TOTAL_LED_NUMBER)
        
        );

        break;

        case SYS_EX_MT_FREE_PINS:
        return  (

        (parameter == SYS_EX_FREE_PIN_A)    ||
        (parameter == SYS_EX_FREE_PIN_B)    ||
        (parameter == SYS_EX_FREE_PIN_C)    ||
        (parameter == SYS_EX_FREE_PIN_D)

        );

        break;

        case SYS_EX_MT_SW_FEATURE:
        return  (

        (parameter == SYS_EX_SW_F_RUNNING_STATUS)           ||
        (parameter == SYS_EX_SW_F_STANDARD_NOTE_OFF)        ||
        (parameter == SYS_EX_SW_F_ENC_NOTES)                ||
        (parameter == SYS_EX_SW_F_POT_NOTES)                ||
        (parameter == SYS_EX_SW_F_LONG_PRESS)               ||
        (parameter == SYS_EX_SW_F_LED_BLINK)                ||
        (parameter == SYS_EX_SW_F_START_UP_ROUTINE)
        );

        break;

        case SYS_EX_MT_HW_FEATURE:
        return  (

        (parameter == SYS_EX_HW_F_BUTTONS)                  ||
        (parameter == SYS_EX_HW_F_POTS)                     ||
        (parameter == SYS_EX_HW_F_ENC)                      ||
        (parameter == SYS_EX_HW_F_LEDS)
        );

        break;

        case SYS_EX_MT_BUTTON:
        return   (parameter < MAX_NUMBER_OF_BUTTONS);
        break;

        case SYS_EX_MT_POT:
        return   (parameter < MAX_NUMBER_OF_POTS);
        break;

        case SYS_EX_MT_ENC:
        return   (parameter < (MAX_NUMBER_OF_BUTTONS/2));
        break;

        case SYS_EX_MT_LED:
        return (parameter < MAX_NUMBER_OF_LEDS);
        break;

        default:
        return false;
        break;

    }

    return false;

}

bool OpenDeck::sysExCheckNewParameterID(uint8_t messageType, uint8_t messageSubType, uint8_t parameter, uint8_t newParameter) {

    switch (messageType)    {

        case SYS_EX_MT_MIDI_CHANNEL:
        return ((newParameter >= 1) && (newParameter <= 16));   //there are only 16 MIDI channels
        break;

        case SYS_EX_MT_HW_PARAMETER:

        switch (parameter)    {

            case SYS_EX_HW_P_BOARD_TYPE:
            return  (

                (newParameter == 0)                             ||
                (newParameter == SYS_EX_BOARD_TYPE_TANNIN)      ||
                (newParameter == SYS_EX_BOARD_TYPE_OPEN_DECK_1)

            );

            break;

            case SYS_EX_HW_P_LONG_PRESS_TIME:
            return ((newParameter >= 4) && (newParameter <= 15));
            break;

            case SYS_EX_HW_P_BLINK_TIME:
            return ((newParameter >= 1) && (newParameter <= 15));
            break;

            case SYS_EX_HW_P_START_UP_SWITCH_TIME:
            return ((newParameter >= 1) && (newParameter <= 150));
            break;

            case SYS_EX_HW_P_START_UP_ROUTINE:
            return (newParameter < NUMBER_OF_START_UP_ROUTINES);
            break;

            case SYS_EX_HW_P_TOTAL_LED_NUMBER:
            return (newParameter < MAX_NUMBER_OF_LEDS);
            break;

            default:
            return false;
            break;

        }

        break;

        case SYS_EX_MT_FREE_PINS:
        return  (

        (newParameter == SYS_EX_FREE_PIN_STATE_DISABLED) ||
        (newParameter == SYS_EX_FREE_PIN_STATE_B_ROW)    ||
        (newParameter == SYS_EX_FREE_PIN_STATE_L_ROW)

        );

        break;

        case SYS_EX_MT_SW_FEATURE:
        case SYS_EX_MT_HW_FEATURE:
        return ((newParameter == SYS_EX_ENABLE) || (newParameter == SYS_EX_DISABLE));
        break;

        case SYS_EX_MT_BUTTON:

        switch (messageSubType) {

            case SYS_EX_MST_BUTTON_TYPE:
            return (

                (newParameter == SYS_EX_BUTTON_TYPE_MOMENTARY) ||
                (newParameter == SYS_EX_BUTTON_TYPE_LATCHING)
                
            );

            break;

            case SYS_EX_MST_BUTTON_NOTE:
            return (newParameter < 128);
            break;
            
            default:
            return false;
            break;

        }

        break;

        case SYS_EX_MT_POT:
        case SYS_EX_MT_ENC:
        return (newParameter < 128);
        break;

        case SYS_EX_MT_LED:

        switch (messageSubType)  {

            case SYS_EX_MST_LED_ACT_NOTE:
            return (newParameter < 128);
            break;

            case SYS_EX_MST_LED_START_UP_NUMBER:
            return (newParameter < MAX_NUMBER_OF_LEDS);

            case SYS_EX_MST_LED_STATE:
            return  (

            (newParameter == SYS_EX_LED_STATE_C_OFF)  ||
            (newParameter == SYS_EX_LED_STATE_C_ON)   ||
            (newParameter == SYS_EX_LED_STATE_B_OFF)  ||
            (newParameter == SYS_EX_LED_STATE_B_ON)

            );

            break;

            default:
            return false;
            break;

        }

        break;

        default:
        return false;
        break;

    }

    return false;
}

void OpenDeck::sysExGenerateError(uint8_t errorNumber)  {

    uint8_t sysExResponse[5];

    sysExResponse[0] = SYS_EX_M_ID_0;
    sysExResponse[1] = SYS_EX_M_ID_1;
    sysExResponse[2] = SYS_EX_M_ID_2;
    sysExResponse[3] = SYS_EX_ERROR;
    sysExResponse[4] = errorNumber;

    sendSysExDataCallback(sysExResponse, 5);

}

void OpenDeck::sysExGenerateAck()   {

    uint8_t sysExAckResponse[4];

    sysExAckResponse[0] = SYS_EX_M_ID_0;
    sysExAckResponse[1] = SYS_EX_M_ID_1;
    sysExAckResponse[2] = SYS_EX_M_ID_2;
    sysExAckResponse[3] = SYS_EX_ACK;
    
    sysExEnabled = true;

    sendSysExDataCallback(sysExAckResponse, 4);

}

uint8_t OpenDeck::sysExGenerateMinMessageLenght(bool wish, bool singleAll, uint8_t messageType)    {

    //single parameter
    if (!singleAll)  {

        if (!wish)      return SYS_EX_ML_REQ_DATA + 1;  //get   //add 1 to length for parameter
        else            return SYS_EX_ML_REQ_DATA + 2;  //set   //add 2 to length for parameter and new value

        }   else    {

        if (!wish)    return SYS_EX_ML_REQ_DATA;        //get
        else    {                                       //set

            switch (messageType)    {

                case SYS_EX_MT_MIDI_CHANNEL:
                return SYS_EX_ML_REQ_DATA + NUMBER_OF_MIDI_CHANNELS;
                break;

                case SYS_EX_MT_HW_PARAMETER:
                return SYS_EX_ML_REQ_DATA + NUMBER_OF_HW_P;
                break;

                case SYS_EX_MT_FREE_PINS:
                return SYS_EX_ML_REQ_DATA + NUMBER_OF_FREE_PINS;
                break;

                case SYS_EX_MT_SW_FEATURE:
                return SYS_EX_ML_REQ_DATA + NUMBER_OF_SW_F;
                break;

                case SYS_EX_MT_HW_FEATURE:
                return SYS_EX_ML_REQ_DATA + NUMBER_OF_HW_F;
                break;
                
                case SYS_EX_MT_BUTTON:
                return SYS_EX_ML_REQ_DATA + MAX_NUMBER_OF_BUTTONS;
                break;
                
                case SYS_EX_MT_POT:
                return SYS_EX_ML_REQ_DATA + MAX_NUMBER_OF_POTS;
                break;
                
                case SYS_EX_MT_LED:
                return SYS_EX_ML_REQ_DATA + MAX_NUMBER_OF_LEDS;
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

    if ((!freePinConfEn) && (sysExArray[SYS_EX_MS_MT] == SYS_EX_MT_FREE_PINS))  {

        sysExGenerateError(SYS_EX_ERROR_NOT_SUPPORTED);
        return;

    }

    uint8_t componentNr     = 1,
            maxComponentNr  = 0,
            _parameter      = 0;

    //create basic response
    uint8_t sysExResponse[128+SYS_EX_ML_REQ_DATA];

    sysExResponse[0] = SYS_EX_M_ID_0;
    sysExResponse[1] = SYS_EX_M_ID_1;
    sysExResponse[2] = SYS_EX_M_ID_2;

    sysExResponse[3] = SYS_EX_ACK;

    sysExResponse[4] = sysExArray[SYS_EX_MS_WISH];
    sysExResponse[5] = sysExArray[SYS_EX_MS_AMOUNT];

    sysExResponse[6] = sysExArray[SYS_EX_MS_MT];
    sysExResponse[7] = sysExArray[SYS_EX_MS_MST];

    switch (sysExArray[SYS_EX_MS_MT])   {

        case SYS_EX_MT_MIDI_CHANNEL:
        maxComponentNr = NUMBER_OF_MIDI_CHANNELS;
        break;

        case SYS_EX_MT_HW_PARAMETER:
        maxComponentNr = NUMBER_OF_HW_P;
        break;

        case SYS_EX_MT_SW_FEATURE:
        maxComponentNr = NUMBER_OF_SW_F;
        break;

        case SYS_EX_MT_HW_FEATURE:
        maxComponentNr = NUMBER_OF_HW_F;
        break;

        case SYS_EX_MT_BUTTON:
        maxComponentNr = MAX_NUMBER_OF_BUTTONS;
        break;

        case SYS_EX_MT_POT:
        maxComponentNr = MAX_NUMBER_OF_POTS;
        break;

        case SYS_EX_MT_ENC:
        maxComponentNr = MAX_NUMBER_OF_BUTTONS/2;
        break;

        case SYS_EX_MT_LED:
        maxComponentNr = MAX_NUMBER_OF_LEDS;
        break;

        case SYS_EX_MT_FREE_PINS:
        maxComponentNr = NUMBER_OF_FREE_PINS;
        break;

        default:
        break;

    }

    if (sysExArray[SYS_EX_MS_AMOUNT]) componentNr = maxComponentNr;

    //create response based on wanted message type
    if (sysExArray[SYS_EX_MS_WISH] == SYS_EX_WISH_GET)    {

        if (sysExArray[SYS_EX_MS_AMOUNT])   _parameter = 0;
        else                                _parameter = sysExArray[SYS_EX_MS_PARAMETER_ID];

        for (int i=0; i<componentNr; i++) {

            sysExResponse[i+SYS_EX_ML_RES_BASIC] = sysExGet(sysExArray[SYS_EX_MS_MT], sysExArray[SYS_EX_MS_MST], _parameter);
            _parameter++;

        }

        sendSysExDataCallback(sysExResponse, SYS_EX_ML_RES_BASIC+componentNr);
        return;

        }   else    if (sysExArray[SYS_EX_MS_WISH] == SYS_EX_WISH_SET)   {

        uint8_t arrayIndex;

        if (sysExArray[SYS_EX_MS_AMOUNT]) {

            _parameter = 0;
            arrayIndex = SYS_EX_MS_NEW_PARAMETER_ID_ALL;

            }   else    {

            _parameter = sysExArray[SYS_EX_MS_PARAMETER_ID];
            arrayIndex = SYS_EX_MS_NEW_PARAMETER_ID_SINGLE;

        }

        for (int i=0; i<componentNr; i++)   {

            if (!sysExSet(sysExArray[SYS_EX_MS_MT], sysExArray[SYS_EX_MS_MST], _parameter, sysExArray[arrayIndex+i]))  {

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

uint8_t OpenDeck::sysExGetHardwareParameter(uint8_t parameter)  {

    switch (parameter)  {

        case SYS_EX_HW_P_BOARD_TYPE:
        return _board;
        break;

        case SYS_EX_HW_P_LONG_PRESS_TIME:
        //long press time
        return _longPressTime/100;
        break;

        case SYS_EX_HW_P_BLINK_TIME:
        //blink time
        return _blinkTime/100;
        break;

        case SYS_EX_HW_P_TOTAL_LED_NUMBER:
        return totalNumberOfLEDs;
        break;

        case SYS_EX_HW_P_START_UP_SWITCH_TIME:
        //start-up led switch time
        startUpRoutine();
        return _startUpLEDswitchTime/10;
        break;

        case SYS_EX_HW_P_START_UP_ROUTINE:
        startUpRoutine();
        return startUpRoutinePattern;
        break;

        default:
        break;

    }   return 0;

}

bool OpenDeck::sysExGetFeature(uint8_t featureType, uint8_t feature)    {

    switch (featureType)    {

        case SYS_EX_MT_SW_FEATURE:
        return bitRead(softwareFeatures, feature);
        break;

        case SYS_EX_MT_HW_FEATURE:
        return bitRead(hardwareFeatures, feature);
        break;

        default:
        return false;
        break;

    }

    return false;

}

uint8_t OpenDeck::sysExGet(uint8_t messageType, uint8_t messageSubType, uint8_t parameter)  {

    switch (messageType)    {

        case SYS_EX_MT_MIDI_CHANNEL:
        return sysExGetMIDIchannel(parameter);
        break;

        case SYS_EX_MT_HW_PARAMETER:
        return sysExGetHardwareParameter(parameter);
        break;

        case SYS_EX_MT_FREE_PINS:
        return freePinState[parameter];
        break;

        case SYS_EX_MT_SW_FEATURE:
        case SYS_EX_MT_HW_FEATURE:
        return sysExGetFeature(messageType, parameter);
        break;

        case SYS_EX_MT_BUTTON:
        if (messageSubType == SYS_EX_MST_BUTTON_TYPE)                   return getButtonType(parameter);
        else                                                            return buttonNote[parameter];
        break;

        case SYS_EX_MT_POT:
        if (messageSubType == SYS_EX_MST_POT_ENABLED)                   return getPotEnabled(parameter);
        else if (messageSubType == SYS_EX_MST_POT_INVERTED)             return getPotInvertState(parameter);
        else                                                            return ccNumber[parameter];
        break;

        case SYS_EX_MT_LED:
        if (messageSubType == SYS_EX_MST_LED_ACT_NOTE)                  return ledNote[parameter];
        else if (messageSubType == SYS_EX_MST_LED_START_UP_NUMBER)      return eeprom_read_byte((uint8_t*)EEPROM_LED_START_UP_NUMBER_START+parameter);
        else if (messageSubType == SYS_EX_MST_LED_STATE)                return ledState[parameter];
        else return 0;
        break;

        default:
        return 0;
        break;

    }

}

bool OpenDeck::sysExSet(uint8_t messageType, uint8_t messageSubType, uint8_t parameter, uint8_t newParameter)    {

    switch (messageType)    {

        case SYS_EX_MT_MIDI_CHANNEL:
        return sysExSetMIDIchannel(parameter, newParameter);
        break;

        case SYS_EX_MT_HW_PARAMETER:
        return sysExSetHardwareParameter(parameter, newParameter);
        break;
        
        case SYS_EX_MT_FREE_PINS:
        return configureFreePin(parameter, newParameter);
        break;

        case SYS_EX_MT_SW_FEATURE:
        case SYS_EX_MT_HW_FEATURE:
        return sysExSetFeature(messageType, parameter, newParameter);
        break;

        case SYS_EX_MT_BUTTON:
        if (messageSubType == SYS_EX_MST_BUTTON_TYPE)                   return sysExSetButtonType(parameter, newParameter);
        else                                                            return sysExSetButtonNote(parameter, newParameter);
        break;

        case SYS_EX_MT_POT:
        if (messageSubType == SYS_EX_MST_POT_ENABLED)                   return sysExSetPotEnabled(parameter, newParameter);
        else if (messageSubType == SYS_EX_MST_POT_INVERTED)             return sysExSetPotInvertState(parameter, newParameter);
        else                                                            return sysExSetCCnumber(parameter, newParameter);

        case SYS_EX_MT_LED:
        if (messageSubType == SYS_EX_MST_LED_ACT_NOTE)                  return sysExSetLEDnote(parameter, newParameter);
        else if (messageSubType == SYS_EX_MST_LED_START_UP_NUMBER)      return sysExSetLEDstartNumber(parameter, newParameter);
        else if (messageSubType == SYS_EX_MST_LED_STATE)    {

            switch (newParameter)   {

                case SYS_EX_LED_STATE_C_OFF:
                handleLED(false, false, parameter);
                return true;
                break;

                case SYS_EX_LED_STATE_C_ON:
                handleLED(true, false, parameter);
                return true;
                break;

                case SYS_EX_LED_STATE_B_OFF:
                handleLED(false, true, parameter);
                return true;
                break;

                case SYS_EX_LED_STATE_B_ON:
                handleLED(true, true, parameter);
                return true;
                break;

                default:
                return false;
                break;

            }

        }   else return false;

        break;

        default:
        return false;
        break;

    }
    
}


//setters
bool OpenDeck::sysExSetFeature(uint8_t featureType, uint8_t feature, bool state)    {

    switch (featureType)    {

        case SYS_EX_MT_SW_FEATURE:
        //software feature
        bitWrite(softwareFeatures, feature, state);
        eeprom_update_byte((uint8_t*)EEPROM_SOFTWARE_FEATURES_START, softwareFeatures);
        return (eeprom_read_byte((uint8_t*)EEPROM_SOFTWARE_FEATURES_START) == softwareFeatures);
        break;

        case SYS_EX_MT_HW_FEATURE:
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

        case SYS_EX_HW_P_BOARD_TYPE:
        _board = value;
        eeprom_update_byte((uint8_t*)EEPROM_HW_P_BOARD_TYPE, value);
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_BOARD_TYPE) == value);
        break;
        
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
        startUpRoutine();
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_START_UP_SWITCH_TIME) == value);
        break;

        case SYS_EX_HW_P_START_UP_ROUTINE:
        //set start-up routine pattern
        startUpRoutinePattern = value;
        eeprom_update_byte((uint8_t*)EEPROM_HW_P_START_UP_ROUTINE, value);
        startUpRoutine();
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_START_UP_ROUTINE) == value);
        break;
        
        case SYS_EX_HW_P_TOTAL_LED_NUMBER:
        //set total number of LEDs (needed for start-up routine)
        totalNumberOfLEDs = value;
        eeprom_update_byte((uint8_t*)EEPROM_HW_P_TOTAL_LED_NUMBER, value);
        return (eeprom_read_byte((uint8_t*)EEPROM_HW_P_TOTAL_LED_NUMBER) == value);
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

    uint16_t eepromAddress = EEPROM_POT_ENABLED_START;

    for (int i=0; i<MAX_NUMBER_OF_POTS/8; i++)  {

        potEnabled[i] = 0;
        eeprom_update_byte((uint8_t*)eepromAddress+i, potEnabled[i]);
        if (!(eeprom_read_byte((uint8_t*)eepromAddress+i) == potEnabled[i])) return false;

    }   return true;

}

bool OpenDeck::sysExSetLEDnote(uint8_t ledNumber, uint8_t _ledNote) {

    uint16_t eepromAddress = EEPROM_LED_ACT_NOTE_START+ledNumber;

    ledNote[ledNumber] = _ledNote;
    eeprom_update_byte((uint8_t*)eepromAddress, _ledNote);
    return _ledNote == eeprom_read_byte((uint8_t*)eepromAddress);

}

bool OpenDeck::sysExSetLEDstartNumber(uint8_t ledNumber, uint8_t startNumber) {

    uint16_t eepromAddress = EEPROM_LED_START_UP_NUMBER_START+ledNumber;

    eeprom_update_byte((uint8_t*)eepromAddress, startNumber);
    return startNumber == eeprom_read_byte((uint8_t*)eepromAddress);

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

bool OpenDeck::sysExSetFreePin(uint8_t pin, uint8_t pinState)   {

    freePinState[pin] = pinState;
    eeprom_update_byte((uint8_t*)EEPROM_FREE_PIN_START+pin, pinState);
    return (eeprom_read_byte((uint8_t*)EEPROM_FREE_PIN_START+pin) == pinState);

}

void OpenDeck::sysExSetDefaultConf()    {

    //write default configuration stored in PROGMEM to EEPROM
    for (int i=0; i<(int16_t)sizeof(defConf); i++)
    eeprom_update_byte((uint8_t*)i, pgm_read_byte(&(defConf[i])));

}

//data restoration
bool OpenDeck::sysExRestore(uint8_t messageType, uint8_t messageSubType, uint8_t parameter) {

    switch (messageType)    {

        case SYS_EX_MT_MIDI_CHANNEL:
        break;

        case SYS_EX_MT_HW_PARAMETER:
        break;

        case SYS_EX_MT_SW_FEATURE:
        break;

        case SYS_EX_MT_HW_FEATURE:
        break;
        
        case SYS_EX_MT_FREE_PINS:
        break;

        case SYS_EX_MT_BUTTON:
        break;

        case SYS_EX_MT_POT:
        break;

        case SYS_EX_MT_ENC:
        break;

        case SYS_EX_MT_LED:
        break;

        case SYS_EX_MT_ALL:
        break;

        default:
        break;

    }   return false;

}