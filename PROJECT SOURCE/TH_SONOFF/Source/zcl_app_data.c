#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"
#include "zcl_app.h"
#include "battery.h"
#include "version.h"

#define APP_DEVICE_VERSION 2
#define APP_FLAGS 0
#define APP_HWVERSION 1
#define APP_ZCLVERSION 1

#define DEFAULT_SET_HIGH_TEMP 0
#define DEFAULT_SET_LOW_TEMP 0
#define DEFAULT_SET_HIGH_HUM 0
#define DEFAULT_SET_LOW_HUM 0
#define DEFAULT_ENABLE_TEMP FALSE
#define DEFAULT_ENABLE_HUM FALSE
#define APP_REPORT_DELAY 5

const uint16 zclApp_clusterRevision_all = 0x0001;

int16 zclApp_Temperature_Sensor_MeasuredValue = 0;
uint16 zclApp_HumiditySensor_MeasuredValue = 0;

const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;

const uint8 zclApp_ManufacturerName[] = {13, 'e', 'f', 'e', 'k', 't', 'a', 'l', 'a', 'b', '.', 'c', '0', 'm'};

const uint8 zclApp_ModelId[] = {14, 'Z', 'B', '-', 'T', 'H', '0', '1', '_', 'E', 'F', 'E', 'K', 'T', 'A'};

const uint8 zclApp_PowerSource = POWER_SOURCE_BATTERY;

application_config_t zclApp_Config = {
    .HighTemp = DEFAULT_SET_HIGH_TEMP,
    .LowTemp = DEFAULT_SET_LOW_TEMP,
    .HighHum = DEFAULT_SET_HIGH_HUM,
    .LowHum = DEFAULT_SET_LOW_HUM,
    .EnableTemp = DEFAULT_ENABLE_TEMP,
    .EnableHum = DEFAULT_ENABLE_HUM,
    .ReportDelay = APP_REPORT_DELAY
};

CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, RR, (void *)&zclBattery_Voltage}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclBattery_PercentageRemainig}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC, ZCL_UINT16, RR, (void *)&zclBattery_RawAdc}},
    {POWER_CFG, {ATTRID_ReportDelay, ZCL_UINT16, RW, (void *)&zclApp_Config.ReportDelay}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, R, (void *)&zclApp_Temperature_Sensor_MeasuredValue}},
    {TEMP, {ATTRID_SET_HIGHTEMP, ZCL_INT16, RW, (void *)&zclApp_Config.HighTemp}},
    {TEMP, {ATTRID_SET_LOWTEMP, ZCL_INT16, RW, (void *)&zclApp_Config.LowTemp}},
    {TEMP, {ATTRID_ENABLE_TEMP, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.EnableTemp}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclApp_HumiditySensor_MeasuredValue}},
    {HUMIDITY, {ATTRID_SET_HIGHHUM, ZCL_UINT16, RW, (void *)&zclApp_Config.HighHum}},
    {HUMIDITY, {ATTRID_SET_LOWHUM, ZCL_UINT16, RW, (void *)&zclApp_Config.LowHum}},
    {HUMIDITY, {ATTRID_ENABLE_HUM, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.EnableHum}}
};

uint8 CONST zclApp_AttrsFirstEPCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));

const cId_t zclApp_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC, POWER_CFG, TEMP, HUMIDITY};

#define APP_MAX_INCLUSTERS (sizeof(zclApp_InClusterList) / sizeof(zclApp_InClusterList[0]))

const cId_t zclApp_OutClusterListFirstEP[] = {ONOFF, TEMP, HUMIDITY};

#define APP_MAX_OUTCLUSTERS_FIRST_EP (sizeof(zclApp_OutClusterListFirstEP) / sizeof(zclApp_OutClusterListFirstEP[0]))

SimpleDescriptionFormat_t zclApp_FirstEP = {
    1,
    ZCL_HA_PROFILE_ID,
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,
    APP_DEVICE_VERSION,
    APP_FLAGS,
    APP_MAX_INCLUSTERS,
    (cId_t *)zclApp_InClusterList,
    APP_MAX_OUTCLUSTERS_FIRST_EP,
    (cId_t *)zclApp_OutClusterListFirstEP
};

void zclApp_ResetAttributesToDefaultValues(void) {
    zclApp_Config.HighTemp = DEFAULT_SET_HIGH_TEMP;
    zclApp_Config.LowTemp = DEFAULT_SET_LOW_TEMP;
    zclApp_Config.HighHum = DEFAULT_SET_HIGH_HUM;
    zclApp_Config.LowHum = DEFAULT_SET_LOW_HUM;
    zclApp_Config.EnableTemp = DEFAULT_ENABLE_TEMP;
    zclApp_Config.EnableHum = DEFAULT_ENABLE_HUM;
    zclApp_Config.ReportDelay = APP_REPORT_DELAY;
}