/*************************************************************************************************
  HDC1080 lightweight sensor driver for CC2530,                                                 **
  Written by Andrew Lamchenko aka Berk,                                                         **
  LLC EfektaLab, Moscow, Russia,                                                                **
  September, 2022.                                                                              **
**************************************************************************************************/

#ifndef __HDC1080_H
#define __HDC1080_H

#define         HDC_1080_ADD                            0x40
#define         HDC_1080_ADD_READ                       (HDC_1080_ADD << 1) | 0x01
#define         HDC_1080_ADD_WRITE                      (HDC_1080_ADD << 1) | 0x00
#define         Configuration_register_add              0x02
#define         Temperature_register_add                0x00
#define         Humidity_register_add                   0x01


typedef enum
{
  TResolution_14 = 0,
  TResolution_11 = 1
}Temp_Reso;

typedef enum
{
  HResolution_14 = 0,
  HResolution_11 = 1,
  HResolution_8 =2
}Humi_Reso;

void hdc1080_init(Temp_Reso Temperature_Resolution_x_bit,Humi_Reso Humidity_Resolution_x_bit);
void hdc1080_start_measurement(void);
void hdc1080_read_measurement(float* temperature, float* humidity);
#endif

