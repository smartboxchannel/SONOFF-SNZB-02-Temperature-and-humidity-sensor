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

#include "commissioning.h"
#include "factory_reset.h"
#include "utils.h"

#include "hdc1080.h"


#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

#define MULTI (float)0.428

#define VOLTAGE_MIN 2.1

#define VOLTAGE_MAX 3.2

#define ZCL_BATTERY_REPORT_REPORT_CONVERTER(millivolts) getBatteryRemainingPercentageZCLCR2032(millivolts)

#ifdef BDB_REPORTING
#if BDBREPORTING_MAX_ANALOG_ATTR_SIZE == 8
  //uint8 reportableChange1[] = {0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 0x6400 is 100 in int16
  uint8 reportableChange2[] = {0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 0x1900 is 25 in int16
  uint8 reportableChange3[] = {0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 0x3200 is 50 in int16
  uint8 reportableChange4[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 0x0100 is 1 in int16
#endif
#if BDBREPORTING_MAX_ANALOG_ATTR_SIZE == 4
  //uint8 reportableChange1[] = {0x64, 0x00, 0x00, 0x00}; 
  uint8 reportableChange2[] = {0x19, 0x00, 0x00, 0x00}; 
  uint8 reportableChange3[] = {0x32, 0x00, 0x00, 0x00}; 
  uint8 reportableChange4[] = {0x01, 0x00, 0x00, 0x00}; 
#endif 
#if BDBREPORTING_MAX_ANALOG_ATTR_SIZE == 2
  //uint8 reportableChange1[] = {0x64, 0x00}; 
  uint8 reportableChange2[] = {0x19, 0x00}; 
  uint8 reportableChange3[] = {0x32, 0x00}; 
  uint8 reportableChange4[] = {0x01, 0x00}; 
#endif 
#endif      

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;
bool initOut = false;
int16 sendInitReportCount = 0;
static uint8 currentSensorsReadingPhase = 0;
int16 temp_old = 0;
int16 temp_tr = 10;
int16 humi_old = 0;
int16 humi_tr = 25;
int16 startWork = 0;
bool pushBut = false;

uint8 SeqNum = 0;
afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

static void zclApp_BasicResetCB(void);
static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_Report(void);
static void zclApp_ReadSensors(void);
static void zclApp_TempHumiSens(void);
static void zclApp_SendTemp(void);
static void zclApp_SendHum(void);
static void zclApp_Termostat(void);
static void zclApp_Gigrostat(void);
static uint16 getBatteryVoltage(void);
static uint8 getBatteryVoltageZCL(uint16 millivolts);
static uint8 getBatteryRemainingPercentageZCLCR2032(uint16 volt16);
static void zclBattery_Report(void);
static void zclBattery_Read(void);
static void zclApp_LedOn(void);
static void zclApp_LedOff(void);

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
    
    bdb_RepAddAttrCfgRecordDefaultToList(1, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE, 20, 600, reportableChange2);
    bdb_RepAddAttrCfgRecordDefaultToList(1, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, 20, 1200, reportableChange3);   
    bdb_RepAddAttrCfgRecordDefaultToList(1, POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE, 1800, 3600, reportableChange4);
    bdb_RepAddAttrCfgRecordDefaultToList(1, POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, 1800, 3600, reportableChange4);
    
    IO_PUD_PORT(OCM_CLK_PORT, IO_PUP);
    IO_PUD_PORT(OCM_DATA_PORT, IO_PUP)
    HalI2CInit();
    hdc1080_init(TResolution_14,HResolution_14);
 
    zclApp_ReadSensors();
    
    osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 5000);
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
        //LREPMaster("APP_REPORT_EVT\r\n");
        if (initOut == false){
          sendInitReportCount++;
          if(sendInitReportCount  >=  10){
            osal_stop_timerEx(zclApp_TaskID, APP_REPORT_EVT);
            osal_clear_event(zclApp_TaskID, APP_REPORT_EVT);
            osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, (uint32)zclApp_Config.ReportDelay*1000); 
            initOut = true;
          }else{
            osal_stop_timerEx(zclApp_TaskID, APP_REPORT_EVT);
            osal_clear_event(zclApp_TaskID, APP_REPORT_EVT);
            osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 5000);
          }
          pushBut = true;
          zclApp_Report();
        }else{
          osal_stop_timerEx(zclApp_TaskID, APP_REPORT_EVT);
          osal_clear_event(zclApp_TaskID, APP_REPORT_EVT);
          osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, (uint32)zclApp_Config.ReportDelay*1000); 
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
        return (events ^ APP_REPORT_BEVT);
    }
    
    if (events & APP_SAVE_ATTRS_EVT) {
        //LREPMaster("APP_SAVE_ATTRS_EVT\r\n");
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }
    if (events & APP_LED_EVT) {
           zclApp_LedOff();
        return (events ^ APP_LED_EVT);
    }
    return 0;
}


static void zclApp_LedOn(void){
   HAL_TURN_ON_LED1();
  osal_start_timerEx(zclApp_TaskID, APP_LED_EVT, 300);
}

static void zclApp_LedOff(void){
   HAL_TURN_OFF_LED1();
}


static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    #if APP_COMMISSIONING_BY_LONG_PRESS == TRUE
    if (bdbAttributes.bdbNodeIsOnANetwork == 1) {
      zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    }
#else
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
#endif
  
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    
    if (portAndAction & HAL_KEY_RELEASE) {
        LREPMaster("Key press\r\n");
        pushBut = true;
        zclApp_Report();
    }
    
}


static void zclApp_BasicResetCB(void) {
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
        zclApp_Termostat();
        zclApp_Gigrostat();
        break;

    default:
        currentSensorsReadingPhase = 0;
         if(pushBut == true){
        zclBattery_Report();
        pushBut = false;
        }
        break;
    }
    if (currentSensorsReadingPhase != 0) {
      
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 20);
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
  
  if(devState == DEV_END_DEVICE){
  osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 250); 
  }
 
 if(devState == DEV_END_DEVICE && pushBut == true){
  zclApp_LedOn();
  }
}


static void zclApp_SendTemp(void){
   if(pushBut == true){
  zclReportCmd_t *pReportCmd;
  const uint8 NUM_ATTRIBUTES = 5;
  
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
    
    pReportCmd->attrList[4].attrID = ATTRID_INVERT_LOGIC_TEMP;
    pReportCmd->attrList[4].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[4].attrData = (void *)(&zclApp_Config.InvertLogicTemp);
  
    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TEMP, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
  }
    osal_mem_free(pReportCmd);
  }else{
    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    }
}


static void zclApp_SendHum(void){
  if(pushBut == true){
  zclReportCmd_t *pReportCmd;
  const uint8 NUM_ATTRIBUTES = 5;
  
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
    
    pReportCmd->attrList[4].attrID = ATTRID_INVERT_LOGIC_HUM;
    pReportCmd->attrList[4].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[4].attrData = (void *)(&zclApp_Config.InvertLogicHum);

    zcl_SendReportCmd(zclApp_FirstEP.EndPoint, &inderect_DstAddr, HUMIDITY, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
  }
    osal_mem_free(pReportCmd);
 }else{
    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE); 
  }
}



static void zclApp_Termostat(void){
    if(zclApp_Config.EnableTemp == 1){
  
    if(zclApp_Temperature_Sensor_MeasuredValue > zclApp_Config.HighTemp*100){
      if(zclApp_Config.InvertLogicTemp == 1){
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, true, bdb_getZCLFrameCounter());
        }
      }else{
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, true, bdb_getZCLFrameCounter());
        }
      }
    }else if(zclApp_Temperature_Sensor_MeasuredValue < zclApp_Config.LowTemp*100){
      if(zclApp_Config.InvertLogicTemp == 1){
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, true, bdb_getZCLFrameCounter());
        }
      }else{
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, true, bdb_getZCLFrameCounter());
        }
      }
    }
  }
}


static void zclApp_Gigrostat(void){
 if(zclApp_Config.EnableHum == 1){
  
    if(zclApp_HumiditySensor_MeasuredValue > zclApp_Config.HighHum*100){
      if(zclApp_Config.InvertLogicHum == 1){
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
        }
      }else{
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
        }
      }
    }else if(zclApp_HumiditySensor_MeasuredValue < zclApp_Config.LowHum*100){
      if(zclApp_Config.InvertLogicHum == 1){
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
        }
      }else{
        if(bdbAttributes.bdbNodeIsOnANetwork == 1){
        zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
        }
      }
    }
  }
}





static uint8 getBatteryVoltageZCL(uint16 millivolts) {
    uint8 volt8 = (uint8)(millivolts / 100);
    if ((millivolts - (volt8 * 100)) > 50) {
        return volt8 + 1;
    } else {
        return volt8;
    }
}

static uint16 getBatteryVoltage(void) {
    HalAdcSetReference(HAL_ADC_REF_125V);
    zclBattery_RawAdc = adcReadSampled(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_125V, 10);
    return (uint16)(zclBattery_RawAdc * MULTI);
}


static uint8 getBatteryRemainingPercentageZCLCR2032(uint16 volt16) {
    float battery_level;
    if (volt16 >= 3000) {
        battery_level = 100;
    } else if (volt16 > 2900) {
        battery_level = 100 - ((3000 - volt16) * 58) / 100;
    } else if (volt16 > 2740) {
        battery_level = 42 - ((2900 - volt16) * 24) / 160;
    } else if (volt16 > 2440) {
        battery_level = 18 - ((2740 - volt16) * 12) / 300;
    } else if (volt16 > 2100) {
        battery_level = 6 - ((2440 - volt16) * 6) / 340;
    } else {
        battery_level = 0;
    }
    return (uint8)(battery_level * 2);
}


static void zclBattery_Read(void) {
    uint16 millivolts = getBatteryVoltage();
    zclBattery_Voltage = getBatteryVoltageZCL(millivolts);
    zclBattery_PercentageRemainig = ZCL_BATTERY_REPORT_REPORT_CONVERTER(millivolts);
    //LREP("Battery voltageZCL=%d prc=%d voltage=%d\r\n", zclBattery_Voltage, zclBattery_PercentageRemainig, millivolts);
}


static void zclBattery_Report(void) {
  zclBattery_Read();
if(pushBut == false){
   bdb_RepChangedAttrValue(2, POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING);
}else{
    zclReportCmd_t *pReportCmd;
    const uint8 NUM_ATTRIBUTES = 3;
    
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
        pReportCmd->numAttr = NUM_ATTRIBUTES;

         pReportCmd->attrList[0].attrID = ATTRID_POWER_CFG_BATTERY_VOLTAGE;
        pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[0].attrData = (void *)(&zclBattery_Voltage);

        pReportCmd->attrList[1].attrID = ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING;
        pReportCmd->attrList[1].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[1].attrData = (void *)(&zclBattery_PercentageRemainig);
        
        pReportCmd->attrList[2].attrID = ATTRID_ReportDelay;
        pReportCmd->attrList[2].dataType = ZCL_UINT16;
        pReportCmd->attrList[2].attrData = (void *)(&zclApp_Config.ReportDelay);
        
        zcl_SendReportCmd(1, &inderect_DstAddr, POWER_CFG, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
    }
    osal_mem_free(pReportCmd);
}
}