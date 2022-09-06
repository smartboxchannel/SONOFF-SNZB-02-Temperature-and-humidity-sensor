#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDNwkMgr.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "Debug.h"
#include "OnBoard.h"

#include <stdio.h>
#include <stdlib.h>

#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"

#include "hal_i2c.h"

#include "battery.h"
#include "commissioning.h"
#include "factory_reset.h"
#include "utils.h"
#include "version.h"

#include "hdc1080.h"


#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;
bool initOut = false;
int16 sendInitReportCount = 0;
static uint8 currentSensorsReadingPhase = 0;
static uint8 currentConfigRepPhase = 0;
int16 temp_old = 0;
int16 temp_tr = 10;
int16 humi_old = 0;
int16 humi_tr = 25;
int16 startWork = 0;
bool pushBut = false;

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

static void zclApp_BasicResetCB(void);
static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_Report(void);
static void zclApp_ReadSensors(void);
static void zclApp_TempHumiSens(void);
static void zclSendTemp(int16 temp);
static void zclSendHum(uint16 hum);
static void zclApp_SendTemp(void);
static void zclApp_SendHum(void);
static void zclSendTemp(int16 temp);
static void zclSendHum(uint16 hum);
static void zclApp_SendRepTime(void);
static void zclApp_ConfigTemp(void);
static void zclApp_ConfigHum(void);
static void zclSendRT(void);
static void zclSendCTTr(void);
static void zclSendCHTr(void);

static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    zclApp_BasicResetCB,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void zclApp_Init(byte task_id) {
    requestNewTrustCenterLinkKey = FALSE;
    zclApp_RestoreAttributesFromNV();
    zclApp_TaskID = task_id;
    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);
    zclGeneral_RegisterCmdCallbacks(zclApp_FirstEP.EndPoint, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsFirstEPCount, zclApp_AttrsFirstEP);
    zcl_registerReadWriteCB(zclApp_FirstEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);
    zcl_registerForMsg(zclApp_TaskID);
    RegisterForKeys(zclApp_TaskID);
    
    IO_PUD_PORT(OCM_CLK_PORT, IO_PUP);
    IO_PUD_PORT(OCM_DATA_PORT, IO_PUP)
    HalI2CInit();
    hdc1080_init(TResolution_14,HResolution_14);
    
    //LREP("Started build %s \r\n", zclApp_DateCodeNT);
    zclApp_ReadSensors();
    
    osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 10000);
    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_BEVT, 7200000);
}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id;
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        if (initOut == false){
          sendInitReportCount++;
          if(sendInitReportCount  >=  5){
            osal_stop_timerEx(zclApp_TaskID, APP_REPORT_EVT);
            osal_clear_event(zclApp_TaskID, APP_REPORT_EVT);
            osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, zclApp_Config.ReportDelay*60000); 
            initOut = true;
          }else{
            osal_stop_timerEx(zclApp_TaskID, APP_REPORT_EVT);
            osal_clear_event(zclApp_TaskID, APP_REPORT_EVT);
            osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 10000);
          }
          pushBut = true;
          zclApp_Report();
        }else{
          osal_stop_timerEx(zclApp_TaskID, APP_REPORT_EVT);
          osal_clear_event(zclApp_TaskID, APP_REPORT_EVT);
          osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, zclApp_Config.ReportDelay*60000); 
         zclApp_Report(); 
        }
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }
    
    if (events & APP_REPORT_BEVT) {
        //LREPMaster("APP_REPORT_BEVT\r\n");
        zclBattery_Report();
        zclApp_SendRepTime();
        return (events ^ APP_REPORT_BEVT);
    }
    if (events & APP_REPORT_CEVT) {
        //LREPMaster("APP_REPORT_CEVT\r\n");
        zclApp_SendRepTime();
        return (events ^APP_REPORT_CEVT);
    }
    
    if (events & APP_SAVE_ATTRS_EVT) {
        //LREPMaster("APP_SAVE_ATTRS_EVT\r\n");
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }
    return 0;
}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    LREP("zclApp_HandleKeys portAndAction=0x%X keyCode=0x%X\r\n", portAndAction, keyCode);
    
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    
    if (portAndAction & HAL_KEY_RELEASE) {
        LREPMaster("Key press\r\n");
        pushBut = true;
        zclApp_Report();
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_CEVT, 200);
    }
    
}


static void zclApp_BasicResetCB(void) {
    zclApp_ResetAttributesToDefaultValues();
    zclApp_SaveAttributesToNV();
}

static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper) {

    osal_start_timerEx(zclApp_TaskID, APP_SAVE_ATTRS_EVT, 2000);
    return ZSuccess;
}

static void zclApp_SaveAttributesToNV(void) {
    uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
}

static void zclApp_RestoreAttributesFromNV(void) {
  
    uint8 status = osal_nv_item_init(NW_APP_CONFIG, sizeof(application_config_t), NULL);
    if (status == NV_ITEM_UNINIT) {
        uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
    if (status == ZSUCCESS) {
        osal_nv_read(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
}



static void zclApp_ReadSensors(void) {
    LREP("currentSensorsReadingPhase %d\r\n", currentSensorsReadingPhase);

    switch (currentSensorsReadingPhase++) {  
      
    case 0:
        hdc1080_start_measurement();

        break;  
        
    case 1:
        zclApp_TempHumiSens();

        break;  
      
    case 2:
      zclApp_SendTemp();
        
      break;
      
    case 3:   
        zclApp_SendHum();
        
        break;
        
    case 4:       
        if(zclApp_Config.EnableTemp == 1){
          zclApp_ConfigTemp();
        }
         if(zclApp_Config.EnableHum == 1){
        zclApp_ConfigHum(); 
        }
        break;
        
    case 5:
      if(!pushBut){
      if(startWork <= 10){   
        startWork++;
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_CEVT, 250);
      }
      }else{
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_CEVT, 250);
      }
        break;

    default:
        currentSensorsReadingPhase = 0;
         if(pushBut == true){
        pushBut = false;
        }
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
        break;
    }
    if (currentSensorsReadingPhase != 0) {
      
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 30);
    }
}


static void zclApp_TempHumiSens(void) {
  
  float t = 0.0;
  float h = 0.0;
  hdc1080_read_measurement(&t, &h);
  zclApp_Temperature_Sensor_MeasuredValue = (int16)(t*100);
  zclApp_HumiditySensor_MeasuredValue = (uint16)(h*100);
}


static void _delay_us(uint16 microSecs) {
    while (microSecs--) {
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
    }
}

void user_delay_ms(uint32_t period) { _delay_us(1000 * period); }

static void zclApp_Report(void) { 
  osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 100); 
}


static void zclApp_SendTemp(void){
   if(!pushBut){
    if (abs(zclApp_Temperature_Sensor_MeasuredValue - temp_old) >= temp_tr) {
        temp_old = zclApp_Temperature_Sensor_MeasuredValue;
        //LREP("ReadIntTempSens t=%d\r\n", zclApp_Temperature_Sensor_MeasuredValue);
        //bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
        zclSendTemp(zclApp_Temperature_Sensor_MeasuredValue);
    }
   }
}


static void zclApp_SendHum(void){
   if(!pushBut){
    if (abs(zclApp_HumiditySensor_MeasuredValue - humi_old) >= humi_tr*10) {
        humi_old = zclApp_HumiditySensor_MeasuredValue;
        //LREP("ReadIntTempSens t=%d\r\n", zclApp_HumiditySensor_MeasuredValue);
        //bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
        zclSendHum(zclApp_HumiditySensor_MeasuredValue);
    }
   }
}


static void zclSendTemp(int16 temp) {
  
  const uint8 NUM_ATTRIBUTES = 1;
  zclReportCmd_t *pReportCmd;
  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    pReportCmd->attrList[0].attrID = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_INT16;
    pReportCmd->attrList[0].attrData = (void *)(&temp);
    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TEMP, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
  }
  osal_mem_free(pReportCmd);
}

static void zclSendHum(uint16 hum) {
  
  const uint8 NUM_ATTRIBUTES = 1;
  zclReportCmd_t *pReportCmd;
  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    pReportCmd->attrList[0].attrID = ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT16;
    pReportCmd->attrList[0].attrData = (void *)(&hum);
    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, HUMIDITY, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
  }
  osal_mem_free(pReportCmd);
}


static void zclApp_SendRepTime(void){

  switch (currentConfigRepPhase++) {  
      
    case 0:
      zclSendRT();
      break;      
      
    case 1:
      zclSendCTTr();
      break;
      
    case 2:   
      zclSendCHTr();
        break;
        
    default:
        currentConfigRepPhase = 0;
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
        break;
    }
    if (currentConfigRepPhase != 0) {
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_CEVT, 50);
    }
}


static void zclApp_ConfigTemp(void){
    if(zclApp_Temperature_Sensor_MeasuredValue >= zclApp_Config.HighTemp*100){
      zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
    }else if(zclApp_Temperature_Sensor_MeasuredValue <= zclApp_Config.LowTemp*100){
      zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
    }
}


static void zclApp_ConfigHum(void){
if(zclApp_Config.EnableHum == 1){
      if(zclApp_HumiditySensor_MeasuredValue >= zclApp_Config.HighHum*100){
      zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
    }else if(zclApp_HumiditySensor_MeasuredValue <= zclApp_Config.HighHum*100){
      zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
    }
   }
}


static void zclSendRT(void) {
  
  const uint8 NUM_ATTRIBUTES = 1;
  zclReportCmd_t *pReportCmd;
  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    pReportCmd->attrList[0].attrID = ATTRID_ReportDelay;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT16;
    pReportCmd->attrList[0].attrData = (void *)(&zclApp_Config.ReportDelay);
    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, POWER_CFG, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
  }
  osal_mem_free(pReportCmd);
}


static void zclSendCTTr(void) {
  
  const uint8 NUM_ATTRIBUTES = 4;
  zclReportCmd_t *pReportCmd;
  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    pReportCmd->attrList[0].attrID = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_INT16;
    pReportCmd->attrList[0].attrData = (void *)(&zclApp_Temperature_Sensor_MeasuredValue);
    
    pReportCmd->attrList[1].attrID = ATTRID_ENABLE_TEMP;
    pReportCmd->attrList[1].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[1].attrData = (void *)(&zclApp_Config.EnableTemp);
    
    pReportCmd->attrList[2].attrID = ATTRID_SET_HIGHTEMP;
    pReportCmd->attrList[2].dataType = ZCL_INT16;
    pReportCmd->attrList[2].attrData = (void *)(&zclApp_Config.HighTemp);
    
    pReportCmd->attrList[3].attrID = ATTRID_SET_LOWTEMP;
    pReportCmd->attrList[3].dataType = ZCL_INT16;
    pReportCmd->attrList[3].attrData = (void *)(&zclApp_Config.LowTemp);

    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TEMP, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
  }
  osal_mem_free(pReportCmd);
}


static void zclSendCHTr(void) {
  
  const uint8 NUM_ATTRIBUTES = 4;
  zclReportCmd_t *pReportCmd;
  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    pReportCmd->attrList[0].attrID = ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT16;
    pReportCmd->attrList[0].attrData = (void *)(&zclApp_HumiditySensor_MeasuredValue);
    
    pReportCmd->attrList[1].attrID = ATTRID_ENABLE_HUM;
    pReportCmd->attrList[1].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[1].attrData = (void *)(&zclApp_Config.EnableHum);
    
    pReportCmd->attrList[2].attrID = ATTRID_SET_HIGHHUM;
    pReportCmd->attrList[2].dataType = ZCL_UINT16;
    pReportCmd->attrList[2].attrData = (void *)(&zclApp_Config.HighHum);
    
    pReportCmd->attrList[3].attrID = ATTRID_SET_LOWHUM;
    pReportCmd->attrList[3].dataType = ZCL_UINT16;
    pReportCmd->attrList[3].attrData = (void *)(&zclApp_Config.LowHum);

    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, HUMIDITY, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
  }
  osal_mem_free(pReportCmd);
  zclSendHum(zclApp_HumiditySensor_MeasuredValue);
}