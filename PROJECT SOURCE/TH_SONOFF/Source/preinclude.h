#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE
#define ZCL_ON_OFF
#define TP2_LEGACY_ZC
#define NWK_AUTO_POLL
#define MULTICAST_ENABLED FALSE
#define ZCL_READ
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_REPORTING_DEVICE
#define ZSTACK_DEVICE_BUILD (DEVICE_BUILD_ENDDEVICE)
#define DISABLE_GREENPOWER_BASIC_PROXY
#define BDB_FINDING_BINDING_CAPABILITY_ENABLED 1
#define BDB_REPORTING TRUE
#define BDB_MAX_CLUSTERENDPOINTS_REPORTING 10
#define MT_ZDO_MGMT

#define ISR_KEYINTERRUPT
#define HAL_BUZZER FALSE
#define HAL_LCD FALSE
#define HAL_I2C TRUE

//i2c
#define OCM_CLK_PORT 1
#define OCM_DATA_PORT 1
#define OCM_CLK_PIN 6
#define OCM_DATA_PIN 7
#define HAL_I2C_RETRY_CNT 2

#define POWER_SAVING

// #define DO_DEBUG_UART

#ifdef DO_DEBUG_UART
#define HAL_UART TRUE
#define HAL_UART_DMA 1
#define INT_HEAP_LEN (2685 - 0x4B - 0xBB)
#endif


#define FACTORY_RESET_BY_LONG_PRESS_PORT 0x04 //port2
#define APP_COMMISSIONING_BY_LONG_PRESS TRUE
#define APP_COMMISSIONING_BY_LONG_PRESS_PORT 0x04 //port2
#define HAL_KEY_P2_INPUT_PINS BV(0)

#include "hal_board_cfg.h"
#include "stdint.h"
#include "stddef.h"