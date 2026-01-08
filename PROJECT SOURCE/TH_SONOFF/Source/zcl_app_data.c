#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"
#include "zcl_app.h"

#define APP_DEVICE_VERSION 2
#define APP_FLAGS 0
#define APP_HWVERSION 1
#define APP_ZCLVERSION 1

#define DEFAULT_SET_HIGH_TEMP 30
#define DEFAULT_SET_LOW_TEMP 20
#define DEFAULT_SET_HIGH_HUM 70
#define DEFAULT_SET_LOW_HUM 50
#define DEFAULT_ENABLE_TEMP FALSE
#define DEFAULT_ENABLE_HUM FALSE
#define DEFAULT_INVERT_TEMP FALSE
#define DEFAULT_INVERT_HUM FALSE
#define APP_REPORT_DELAY 5

const uint16 zclApp_clusterRevision_all = 0x0001;

int16 zclApp_Temperature_Sensor_MeasuredValue = 0;
uint16 zclApp_HumiditySensor_MeasuredValue = 0;
uint8 zclBattery_Voltage = 0xff;
uint8 zclBattery_PercentageRemainig = 0xff;
uint16 zclBattery_RawAdc = 0xff;

const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;

const uint8 zclApp_ManufacturerName[] = {17, 'E', 'f', 'e', 'k', 't', 'a', 'L', 'a', 'b', '_', 'f', 'o', 'r', '_', 'y', 'o', 'u'};
const uint8 zclApp_DateCode[] = { 15, '2', '0', '2', '6', '0', '1', '0', '7', ' ', '6', '4', '3', ' ', '7', '7'};
const uint8 zclApp_SWBuildID[] = {5, '1', '.', '0', '.', '3'};

const uint8 zclApp_ModelId[] = {14, 'S', 'N', 'Z', 'B', '-', '0', '2', '_', 'E', 'F', 'E', 'K', 'T', 'A'};

const uint8 zclApp_PowerSource = POWER_SOURCE_BATTERY;

application_config_t zclApp_Config = {
    .HighTemp = DEFAULT_SET_HIGH_TEMP,
    .LowTemp = DEFAULT_SET_LOW_TEMP,
    .HighHum = DEFAULT_SET_HIGH_HUM,
    .LowHum = DEFAULT_SET_LOW_HUM,
    .EnableTemp = DEFAULT_ENABLE_TEMP,
    .EnableHum = DEFAULT_ENABLE_HUM,
    .InvertLogicTemp = DEFAULT_INVERT_TEMP,
    .InvertLogicHum = DEFAULT_INVERT_HUM,
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
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_SWBuildID}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, RR, (void *)&zclBattery_Voltage}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclBattery_PercentageRemainig}},
    {POWER_CFG, {ATTRID_ReportDelay, ZCL_UINT16, RW, (void *)&zclApp_Config.ReportDelay}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_Temperature_Sensor_MeasuredValue}},
    {TEMP, {ATTRID_SET_HIGHTEMP, ZCL_INT16, RW, (void *)&zclApp_Config.HighTemp}},
    {TEMP, {ATTRID_SET_LOWTEMP, ZCL_INT16, RW, (void *)&zclApp_Config.LowTemp}},
    {TEMP, {ATTRID_ENABLE_TEMP, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.EnableTemp}},
    {TEMP, {ATTRID_INVERT_LOGIC_TEMP, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.InvertLogicTemp}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_HumiditySensor_MeasuredValue}},
    {HUMIDITY, {ATTRID_SET_HIGHHUM, ZCL_UINT16, RW, (void *)&zclApp_Config.HighHum}},
    {HUMIDITY, {ATTRID_SET_LOWHUM, ZCL_UINT16, RW, (void *)&zclApp_Config.LowHum}},
    {HUMIDITY, {ATTRID_ENABLE_HUM, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.EnableHum}},
    {HUMIDITY, {ATTRID_INVERT_LOGIC_HUM, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.InvertLogicHum}}
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
