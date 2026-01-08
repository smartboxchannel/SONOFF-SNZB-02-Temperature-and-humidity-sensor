#include "commissioning.h"
#include "OSAL_PwrMgr.h"
#include "ZDApp.h"
#include "bdb_interface.h"
#include "hal_key.h"
#include "zcl_app.h"
#include "OnBoard.h"

static void zclCommissioning_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void zclCommissioning_ResetBackoffRetry(void);
static void zclCommissioning_BindNotification(bdbBindNotificationData_t *data);
extern bool requestNewTrustCenterLinkKey;


byte rejoinsLeft = APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES;
uint32 rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY;

uint8 zclCommissioning_TaskId = 0;

#define APP_TX_POWER TX_PWR_PLUS_4


void zclCommissioning_Init(uint8 task_id) {
    zclCommissioning_TaskId = task_id;

    bdb_RegisterCommissioningStatusCB(zclCommissioning_ProcessCommissioningStatus);
    bdb_RegisterBindNotificationCB(zclCommissioning_BindNotification);

    ZMacSetTransmitPower(APP_TX_POWER);

    // this is important to allow connects throught routers
    // to make this work, coordinator should be compiled with this flag #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;
#if APP_COMMISSIONING_BY_LONG_PRESS == FALSE
      bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
#else 
     bdb_StartCommissioning(BDB_COMMISSIONING_MODE_IDDLE);
#endif
}

static void zclCommissioning_ResetBackoffRetry(void) {
    rejoinsLeft = APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES;
    rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY;
}

static void zclCommissioning_OnConnect(void) {
    zclCommissioning_ResetBackoffRetry();
    osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT, 10000);
}

static void zclCommissioning_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_INITIALIZATION:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NO_NETWORK:
            break;
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclCommissioning_OnConnect();
            break;
        default:
            break;
        }
        break;
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            zclCommissioning_OnConnect();
            HAL_TURN_OFF_LED1();
            break;

        default:

            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclCommissioning_ResetBackoffRetry();
            break;

        default:
            if (rejoinsLeft > 0) {
                rejoinDelay = (uint32)((float)rejoinDelay * APP_COMMISSIONING_END_DEVICE_REJOIN_BACKOFF);
                rejoinsLeft -= 1;
            } else {
                rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_MAX_DELAY;
                zclCommissioning_ResetBackoffRetry();
            }
            osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_END_DEVICE_REJOIN_EVT, rejoinDelay);
            break;
        }
        HAL_TURN_OFF_LED1();
        break;
    default:
        break;
    }
}

static void zclCommissioning_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    if (pInMsg->attrCmd) {
        osal_mem_free(pInMsg->attrCmd);
    }
}

void zclCommissioning_Sleep(uint8 allow) {
#if defined(POWER_SAVING)
    if (allow) {

        NLME_SetPollRate(0);

    } else {
        NLME_SetPollRate(POLL_RATE);
    }
#endif
}

uint16 zclCommissioning_event_loop(uint8 task_id, uint16 events) {
    if (events & SYS_EVENT_MSG) {
        devStates_t zclApp_NwkState;
        afIncomingMSGPacket_t *MSGpkt;
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclCommissioning_TaskId))) {

            switch (MSGpkt->hdr.event) {
            case ZDO_STATE_CHANGE:
                
                zclApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                
                if (zclApp_NwkState == DEV_END_DEVICE) {
                   
                }
                break;

            case ZCL_INCOMING_MSG:
                zclCommissioning_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
                break;

            default:
                break;
            }
            osal_msg_deallocate((uint8 *)MSGpkt);
        }

        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_COMMISSIONING_END_DEVICE_REJOIN_EVT) {
#if ZG_BUILD_ENDDEVICE_TYPE
        bdb_ZedAttemptRecoverNwk();
        osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT, 10000);
#endif
        return (events ^ APP_COMMISSIONING_END_DEVICE_REJOIN_EVT);
    }

    if (events & APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT) {
        zclCommissioning_Sleep(true);
        return (events ^ APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT);
    }
    
#if APP_COMMISSIONING_BY_LONG_PRESS == TRUE    
    if (events & APP_COMMISSIONING_BY_LONG_PRESS_EVT) {
        HAL_TURN_ON_LED1();
        osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_OFF_EVT, 10000);
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
        sendInitReportCount = 0;
        return (events ^ APP_COMMISSIONING_BY_LONG_PRESS_EVT);
    }
    if (events & APP_COMMISSIONING_OFF_EVT) {
        HAL_TURN_OFF_LED1();
        return (events ^ APP_COMMISSIONING_OFF_EVT);
    }
#endif
    return 0;
}

static void zclCommissioning_BindNotification(bdbBindNotificationData_t *data) {
    uint16 maxEntries = 0, usedEntries = 0;
    bindCapacity(&maxEntries, &usedEntries);
}

void zclCommissioning_HandleKeys(uint8 portAndAction, uint8 keyCode) {
    if (portAndAction & HAL_KEY_PRESS) {
      
#if ZG_BUILD_ENDDEVICE_TYPE
        if (devState == DEV_NWK_ORPHAN) {
            bdb_ZedAttemptRecoverNwk();
        }
#endif
    }
#if APP_COMMISSIONING_BY_LONG_PRESS == TRUE 
    if (portAndAction & HAL_KEY_RELEASE) {
      osal_stop_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_BY_LONG_PRESS_EVT);
    } else {
       
      bool statTimer = true;
#if APP_COMMISSIONING_BY_LONG_PRESS_PORT
      statTimer = APP_COMMISSIONING_BY_LONG_PRESS_PORT & portAndAction;
#endif
      if (statTimer && bdbAttributes.bdbNodeIsOnANetwork == 0) {
        
         uint32 timeout = APP_COMMISSIONING_HOLD_TIME_FAST;
         osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_BY_LONG_PRESS_EVT, timeout);
      }
    }
#endif    
}