/*

OpenDECK library v1.3
File: SysEx.h
Last revision date: 2014-12-25
Author: Igor Petrovic

*/


#ifndef SYSEX_H_
#define SYSEX_H_

//sysex

//manufacturer ID bytes
#define SYS_EX_M_ID_0                           0x00
#define SYS_EX_M_ID_1                           0x53
#define SYS_EX_M_ID_2                           0x43

//lower and upper limits for...
//long press time
#define SYS_EX_BUTTON_LONG_PRESS_TIME_MIN       0x04
#define SYS_EX_BUTTON_LONG_PRESS_TIME_MAX       0x0F

//blink time
#define SYS_EX_LED_BLINK_TIME_MIN               0x01
#define SYS_EX_LED_BLINK_TIME_MAX               0x0F

//LED switch time on start-up
#define SYS_EX_LED_START_UP_SWITCH_TIME_MIN     0x01
#define SYS_EX_LED_START_UP_SWITCH_TIME_MAX     0x78

////////////////////////////////////////////////////

//button types
#define SYS_EX_BUTTON_TYPE_MOMENTARY            0x00
#define SYS_EX_BUTTON_TYPE_LATCHING             0x01

//LED blink/constant state determination
#define SYS_EX_LED_VELOCITY_C_OFF               0x00
#define SYS_EX_LED_VELOCITY_B_OFF               0x3F

//message length
#define SYS_EX_ML_REQ_HANDSHAKE                 0x05
#define SYS_EX_ML_REQ_STANDARD                  0x09
#define SYS_EX_ML_RES_BASIC                     0x08

//ACK/error start codes
#define SYS_EX_ACK                              0x41
#define SYS_EX_ERROR                            0x46

#define SYS_EX_ENABLE                           0x01
#define SYS_EX_DISABLE                          0x00

typedef enum {

    //message wish
    SYS_EX_WISH_START,
    SYS_EX_WISH_GET = SYS_EX_WISH_START,
    SYS_EX_WISH_SET,
    SYS_EX_WISH_RESTORE,
    SYS_EX_WISH_END

} sysExWish;

typedef enum {

    //wanted data amount
    SYS_EX_AMOUNT_START,
    SYS_EX_AMOUNT_SINGLE = SYS_EX_AMOUNT_START,
    SYS_EX_AMOUNT_ALL,
    SYS_EX_AMOUNT_END

} sysExAmount;

typedef enum {

    //message types
    SYS_EX_MT_START,
    SYS_EX_MT_HW_CONFIG = SYS_EX_MT_START,
    SYS_EX_MT_FEATURES,
    SYS_EX_MT_MIDI_CHANNEL,
    SYS_EX_MT_BUTTON,
    SYS_EX_MT_POT,
    SYS_EX_MT_LED,
    SYS_EX_MT_ENCODER,
    SYS_EX_MT_ALL,
    SYS_EX_MT_END

} sysExMessageType;

typedef enum {

    SYS_EX_HW_CONFIG_START,
    SYS_EX_HW_CONFIG_BOARD = SYS_EX_HW_CONFIG_START,
    SYS_EX_HW_CONFIG_BUTTONS,
    SYS_EX_HW_CONFIG_LEDS,
    SYS_EX_HW_CONFIG_POTS,
    SYS_EX_HW_CONFIG_END

} sysExHardwareConfig;

typedef enum {

    SYS_EX_BOARD_TYPE_START,
    SYS_EX_BOARD_TYPE_OPEN_DECK_1,
    SYS_EX_BOARD_TYPE_TANNIN,
    SYS_EX_BOARD_TYPE_END

} sysExBoardType;

typedef enum {

    //features
    SYS_EX_MST_FEATURES_START,
    SYS_EX_MST_FEATURES_MIDI = SYS_EX_MST_FEATURES_START,
    SYS_EX_MST_FEATURES_BUTTONS,
    SYS_EX_MST_FEATURES_LEDS,
    SYS_EX_MST_FEATURES_POTS,
    SYS_EX_MST_FEATURES_END

} sysExFeatures;

typedef enum {

    SYS_EX_FEATURES_MIDI_START,
    SYS_EX_FEATURES_MIDI_STANDARD_NOTE_OFF = SYS_EX_FEATURES_MIDI_START,
    SYS_EX_FEATURES_MIDI_END

} sysExMIDIfeatures;

typedef enum {

    SYS_EX_FEATURES_BUTTONS_START,
    SYS_EX_FEATURES_BUTTONS_LONG_PRESS = SYS_EX_FEATURES_BUTTONS_START,
    SYS_EX_FEATURES_BUTTONS_END

} sysExButtonFeatures;

typedef enum {

    SYS_EX_FEATURES_LEDS_START,
    SYS_EX_FEATURES_LEDS_START_UP_ROUTINE = SYS_EX_FEATURES_LEDS_START,
    SYS_EX_FEATURES_LEDS_BLINK,
    SYS_EX_FEATURES_LEDS_END

} sysExLEDfeatures;

typedef enum {

    SYS_EX_FEATURES_POTS_START,
    SYS_EX_FEATURES_POTS_NOTES = SYS_EX_FEATURES_POTS_START,
    SYS_EX_FEATURES_POTS_END

} sysExPotFeatures;

typedef enum {

    SYS_EX_MC_START,
    SYS_EX_MC_BUTTON_NOTE = SYS_EX_MC_START,
    SYS_EX_MC_LONG_PRESS_BUTTON_NOTE,
    SYS_EX_MC_BUTTON_PP,
    SYS_EX_MC_POT_CC,
    SYS_EX_MC_POT_PP,
    SYS_EX_MC_POT_NOTE,
    SYS_EX_MC_INPUT,
    SYS_EX_MC_END

} sysExMIDIchannels;

typedef enum {

    SYS_EX_MST_BUTTON_START,
    SYS_EX_MST_BUTTON_HW_P = SYS_EX_MST_BUTTON_START,
    SYS_EX_MST_BUTTON_TYPE,
    SYS_EX_MST_BUTTON_PP_ENABLED,
    SYS_EX_MST_BUTTON_NOTE,
    SYS_EX_MST_BUTTON_END

} sysExMessageSubTypeButton;

typedef enum {

    SYS_EX_BUTTON_HW_P_START,
    SYS_EX_BUTTON_HW_P_LONG_PRESS_TIME = SYS_EX_BUTTON_HW_P_START,
    SYS_EX_BUTTON_HW_P_END

} sysExButtonHwParameter;

typedef enum {

    SYS_EX_MST_POT_START,
    SYS_EX_MST_POT_HW_P = SYS_EX_MST_POT_START,
    SYS_EX_MST_POT_ENABLED,
    SYS_EX_MST_POT_PP_ENABLED,
    SYS_EX_MST_POT_INVERTED,
    SYS_EX_MST_POT_CC_PP_NUMBER,
    SYS_EX_MST_POT_LOWER_LIMIT,
    SYS_EX_MST_POT_UPPER_LIMIT,
    SYS_EX_MST_POT_END

} sysExMessageSubTypePot;

typedef enum {

    SYS_EX_MST_LED_START,
    SYS_EX_MST_LED_HW_P = SYS_EX_MST_LED_START,
    SYS_EX_MST_LED_ACT_NOTE,
    SYS_EX_MST_LED_START_UP_NUMBER,
    SYS_EX_MST_LED_STATE,
    SYS_EX_LED_END

} sysExMessageSubTypeLED;

typedef enum {

    SYS_EX_LED_HW_P_START,
    SYS_EX_LED_HW_P_TOTAL_NUMBER = SYS_EX_LED_HW_P_START,
    SYS_EX_LED_HW_P_BLINK_TIME,
    SYS_EX_LED_HW_P_START_UP_SWITCH_TIME,
    SYS_EX_LED_HW_P_START_UP_ROUTINE,
    SYS_EX_LED_HW_P_END

} sysEXLEDHwParameter;

typedef enum {

    SYS_EX_LED_STATE_START,
    SYS_EX_LED_STATE_C_OFF = SYS_EX_LED_STATE_START,
    SYS_EX_LED_STATE_C_ON,
    SYS_EX_LED_STATE_B_OFF,
    SYS_EX_LED_STATE_B_ON,
    SYS_EX_LED_STATE_END

} sysExLEDstates;

typedef enum {

    SYS_EX_ERROR_HANDSHAKE,
    SYS_EX_ERROR_WISH,
    SYS_EX_ERROR_AMOUNT,
    SYS_EX_ERROR_MT,
    SYS_EX_ERROR_MST,
    SYS_EX_ERROR_PARAMETER,
    SYS_EX_ERROR_NEW_PARAMETER,
    SYS_EX_ERROR_MESSAGE_LENGTH,
    SYS_EX_ERROR_NOT_SUPPORTED,
    SYS_EX_ERROR_EEPROM,

} sysExErrors;

typedef enum {

    SYS_EX_MS_M_ID_0 = 1,
    SYS_EX_MS_M_ID_1 = 2,
    SYS_EX_MS_M_ID_2 = 3,
    SYS_EX_MS_WISH = 4,
    SYS_EX_MS_AMOUNT = 5,
    SYS_EX_MS_MT = 6,
    SYS_EX_MS_MST = 7,
    SYS_EX_MS_PARAMETER_ID = 8,
    SYS_EX_MS_NEW_PARAMETER_ID_SINGLE = 9,
    SYS_EX_MS_NEW_PARAMETER_ID_ALL = 8,

} sysExMessageByteOrder;

#endif /* SYSEX_H_ */