#ifndef ZCL_APP_H
#define ZCL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"
#include "zcl.h"

#define APP_REPORT_EVT                  0x0001
#define APP_READ_SENSORS_EVT            0x0002
#define APP_SAVE_ATTRS_EVT              0x0004
#define APP_REPORT_BEVT                 0x0008
#define APP_REPORT_CEVT                 0x0020

#define NW_APP_CONFIG 0x0402   
   
#define R ACCESS_CONTROL_READ
#define RW (R | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)
#define RR (R | ACCESS_REPORTABLE)
#define RRW (R | ACCESS_REPORTABLE | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)

#define BASIC       ZCL_CLUSTER_ID_GEN_BASIC
#define POWER_CFG   ZCL_CLUSTER_ID_GEN_POWER_CFG
#define ONOFF       ZCL_CLUSTER_ID_GEN_ON_OFF
#define TEMP        ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
#define HUMIDITY    ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY
 
#define ATTRID_ReportDelay       0x0201      
#define ATTRID_SET_HIGHTEMP      0x0202
#define ATTRID_SET_LOWTEMP       0x0203
#define ATTRID_SET_HIGHHUM       0x0204
#define ATTRID_SET_LOWHUM        0x0205
#define ATTRID_ENABLE_TEMP       0x0206
#define ATTRID_ENABLE_HUM        0x0207

#define ZCL_UINT8 ZCL_DATATYPE_UINT8
#define ZCL_UINT16 ZCL_DATATYPE_UINT16
#define ZCL_INT16 ZCL_DATATYPE_INT16
#define ZCL_INT8  ZCL_DATATYPE_INT8
#define ZCL_INT32 ZCL_DATATYPE_INT32
#define ZCL_UINT32 ZCL_DATATYPE_UINT32
#define ZCL_SINGLE ZCL_DATATYPE_SINGLE_PREC

typedef struct {
    int16 HighTemp;
    int16 LowTemp;
    uint16 HighHum;
    uint16 LowHum;
    uint16 EnableTemp;
    uint16 EnableHum;
    uint16 ReportDelay;
} application_config_t;

extern SimpleDescriptionFormat_t zclApp_FirstEP;

extern uint8 zclApp_BatteryVoltage;
extern uint8 zclApp_BatteryPercentageRemainig;
extern uint16 zclApp_BatteryVoltageRawAdc;
extern int16 zclApp_Temperature_Sensor_MeasuredValue;
extern uint16 zclApp_HumiditySensor_MeasuredValue;

extern CONST zclAttrRec_t zclApp_AttrsFirstEP[];
extern CONST uint8 zclApp_AttrsFirstEPCount;
extern application_config_t zclApp_Config;

extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_PowerSource;

extern void zclApp_Init(byte task_id);

extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);
extern void zclApp_ResetAttributesToDefaultValues(void);
void user_delay_ms(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
