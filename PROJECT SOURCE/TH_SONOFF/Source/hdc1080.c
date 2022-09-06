/*************************************************************************************************
  HDC1080 lightweight sensor driver for CC2530,                                                 **
  Written by Andrew Lamchenko aka Berk,                                                         **
  LLC EfektaLab, Moscow, Russia,                                                                **
  September, 2022.                                                                              **
**************************************************************************************************/

#include "hdc1080.h"
#include <stdlib.h>
#include "hal_i2c.h"

void hdc1080_init(Temp_Reso Temperature_Resolution_x_bit,Humi_Reso Humidity_Resolution_x_bit)
{
	uint16_t config_reg_value=0x1000;
	uint8_t data_send[2];

	if(Temperature_Resolution_x_bit == TResolution_11)
	{
		config_reg_value |= (1<<10);
	}

	switch(Humidity_Resolution_x_bit)
	{
	case HResolution_11:
		config_reg_value|= (1<<8);
		break;
	case HResolution_8:
		config_reg_value|= (1<<9);
		break;
	}

	data_send[0]= (config_reg_value>>8);
	data_send[1]= (config_reg_value&0x00ff);

	HalI2CSend(HDC_1080_ADD_WRITE, data_send, 2); 
}


void hdc1080_start_measurement(void)
{
	uint8_t command = Temperature_register_add;
	HalI2CSend(HDC_1080_ADD_WRITE, &command, 1);
	// Need delay here 15ms for conversion compelete.
}


void hdc1080_read_measurement(float* temperature, float* humidity)
{
	uint8_t receive_data[4];
	uint16_t temp_x,humi_x;

	HalI2CReceive(HDC_1080_ADD_READ, receive_data, 4);

	temp_x =((receive_data[0]<<8)|receive_data[1]);
	humi_x =((receive_data[2]<<8)|receive_data[3]);

	*temperature=((temp_x/65536.0)*165.0)-40.0;
	*humidity=((humi_x/65536.0)*100.0);
}
