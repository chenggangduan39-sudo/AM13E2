/*****************************************************************************
 * KEROS Interface driver
 *
 *
 * Copyright(C) CHIPSBRAIN GLOBAL CO., Ltd.
 * All rights reserved.
 *
 * File Name    : keros_interface.c
 * Author       : ARES HA
 *
 * Version      : V0.3
 * Date         : 2015.09.08
 * Description  : Keros Interface Source
 ****************************************************************************/

/* USER INCLUDE BEGIN */

//#include "stm32l0xx_hal.h"


/* USER INCLUDE END  */
#include "keros_lib.h"
#include "keros_i2c_interface.h"
#include "keros_interface.h"


/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/
/* USER CODE BEGIN  */

uint8_t keros_read_data( uint16_t sub_addr, int read_len, uint8_t * r_data )
{
		uint8_t bSubAddress[2];

		bSubAddress[0] = sub_addr>>8;
		bSubAddress[1] = sub_addr & 0xff;

		return  _i2c_read(KEROS_DEVID_ADDR,  bSubAddress, r_data, read_len);
}

uint8_t keros_write_data( uint16_t sub_addr, uint8_t * w_data, int write_len )
{
		uint8_t bSubAddress[2];

		bSubAddress[0] = sub_addr>>8;
		bSubAddress[1] = sub_addr & 0xff;

		return _i2c_write(KEROS_DEVID_ADDR, bSubAddress, w_data, write_len);
}

// 1ms
void keros_delay ( uint32_t wait_time )
{
	delay_ms(wait_time);
}

uint8_t keros_power_on( void )
{
		uint8_t Data = 0xF0;

		return _i2c_write(KEROS_DEVID_ADDR, NULL,&Data,1);

}
/* USER CODE END  */
