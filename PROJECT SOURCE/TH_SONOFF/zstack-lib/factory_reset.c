#include "factory_reset.h"
#include "AF.h"
#include "OnBoard.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "ZComDef.h"
#include "hal_key.h"
#include "zcl_app.h"

static void zclFactoryResetter_ResetToFN(void);
static void zclFactoryResetter_ProcessBootCounter(void);
static void zclFactoryResetter_ResetBootCounter(void);
static void zclApp_LedOnF(void);
static void zclApp_LedOffF(void);

static uint8 zclFactoryResetter_TaskID;

uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events) {
    if (events & FACTORY_RESET_EVT) {
        zclFactoryResetter_ResetToFN();
        return (events ^ FACTORY_RESET_EVT);
    }

    if (events & FACTORY_BOOTCOUNTER_RESET_EVT) {
        zclFactoryResetter_ResetBootCounter();
        return (events ^ FACTORY_BOOTCOUNTER_RESET_EVT);
    }
    
    if (events & FACTORY_LED_EVT) {
        zclApp_LedOnF();
        return (events ^ FACTORY_LED_EVT);
    }
    
    if (events & FACTORY_LEDOFF_EVT) {
        zclApp_LedOffF();
        return (events ^ FACTORY_LEDOFF_EVT);
    }
    return 0;
}
void zclFactoryResetter_ResetBootCounter(void) {
    uint16 bootCnt = 0;
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

void zclFactoryResetter_Init(uint8 task_id) {
    zclFactoryResetter_TaskID = task_id;
    /**
     * We can't register more than one task, call in main app taks
     * zclFactoryResetter_HandleKeys(portAndAction, keyCode);
     * */
    // RegisterForKeys(task_id);
#if FACTORY_RESET_BY_BOOT_COUNTER
    zclFactoryResetter_ProcessBootCounter();
#endif
}

void zclFactoryResetter_ResetToFN(void) {
    HAL_TURN_ON_LED1();
    osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
    osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);
    bdb_resetLocalAction();
}

void zclFactoryResetter_HandleKeys(uint8 portAndAction, uint8 keyCode) {
#if FACTORY_RESET_BY_LONG_PRESS
    if (portAndAction & HAL_KEY_RELEASE) {
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);
        HAL_TURN_OFF_LED1();
    } else {
        bool statTimer = true;
#if FACTORY_RESET_BY_LONG_PRESS_PORT
        statTimer = FACTORY_RESET_BY_LONG_PRESS_PORT & portAndAction;
#endif
        if (statTimer) {
            uint32 timeout = bdbAttributes.bdbNodeIsOnANetwork ? FACTORY_RESET_HOLD_TIME_LONG : FACTORY_RESET_HOLD_TIME_FAST;
            osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, timeout);
            osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT, 2000);
        }
    }
#endif
}

void zclFactoryResetter_ProcessBootCounter(void) {
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT, FACTORY_RESET_BOOTCOUNTER_RESET_TIME);

    uint16 bootCnt = 0;
    if (osal_nv_item_init(ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt) == ZSUCCESS) {
        osal_nv_read(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
    }
    bootCnt += 1;
    if (bootCnt >= FACTORY_RESET_BOOTCOUNTER_MAX_VALUE) {
        bootCnt = 0;
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT);
        osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, 5000);
    }
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

static void zclApp_LedOnF(void){
   HAL_TURN_ON_LED1();
  osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT, 50);
}

static void zclApp_LedOffF(void){
   HAL_TURN_OFF_LED1();
   osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT, 1000);
}