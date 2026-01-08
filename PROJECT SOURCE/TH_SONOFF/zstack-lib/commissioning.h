#ifndef commissioning_h
#define commissioning_h

#define APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT  0x0100
#define APP_COMMISSIONING_END_DEVICE_REJOIN_EVT       0x0200
#define APP_COMMISSIONING_BY_LONG_PRESS_EVT           0x0400
#define APP_COMMISSIONING_OFF_EVT                     0x0800

#define APP_COMMISSIONING_END_DEVICE_REJOIN_MAX_DELAY ((uint32)900000) // 60 minutes 60 * 60 * 1000
#define APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY 10 * 1000 // 20 seconds
#define APP_COMMISSIONING_END_DEVICE_REJOIN_BACKOFF ((float) 1.2)
#define APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES 24

#ifndef APP_COMMISSIONING_BY_LONG_PRESS
    #define APP_COMMISSIONING_BY_LONG_PRESS FALSE
#endif

#ifndef APP_COMMISSIONING_BY_LONG_PRESS_PORT
    #define APP_COMMISSIONING_BY_LONG_PRESS_PORT 0x00
#endif

#ifndef APP_COMMISSIONING_HOLD_TIME_FAST
    #define APP_COMMISSIONING_HOLD_TIME_FAST 3000
#endif


extern void zclCommissioning_Init(uint8 task_id);
extern uint16 zclCommissioning_event_loop(uint8 task_id, uint16 events);
extern void zclCommissioning_Sleep( uint8 allow );
extern void zclCommissioning_HandleKeys(uint8 portAndAction, uint8 keyCode);
extern bool zclApp_onNet;

#endif