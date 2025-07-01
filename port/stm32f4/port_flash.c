/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kved.h"
#include "kved_flash.h"
#include "main.h"

#define FLASH_KEY1 0x45670123U /*!< Flash key1 */
#define FLASH_KEY2 0xCDEF89ABU /*!< Flash key2 */

#define FLASH_SECTOR_SIZE 16368

const uint32_t sector_size[KVED_FLASH_NUM_SECTORS] = { FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE };
const uint32_t sector_address[KVED_FLASH_NUM_SECTORS] = { 0x08004000, 0x08008000 };
const uint8_t sector_page[KVED_FLASH_NUM_SECTORS] = { 1, 2 };

bool kved_flash_sector_erase(kved_flash_sector_t sec_idx)
{
	uint32_t sector_error;
	FLASH_EraseInitTypeDef sector = 
	{ 
		.TypeErase = FLASH_TYPEERASE_SECTORS,
		.NbSectors = 1,
		.VoltageRange = FLASH_VOLTAGE_RANGE_3,
		.Banks = FLASH_BANK_1,
	};

	sector.Sector = sector_page[sec_idx];

	HAL_FLASH_Unlock();
	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&sector,&sector_error);
	HAL_FLASH_Lock();	

	return status == HAL_OK;
}

void kved_flash_data_write(kved_flash_sector_t sec_idx, uint16_t index, kved_word_t data)
{
	uint32_t addr = sector_address[sec_idx] + index*sizeof(kved_word_t);

	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,addr,(uint32_t)&data);
	HAL_FLASH_Lock();
}

kved_word_t kved_flash_data_read(kved_flash_sector_t sec, uint16_t index)
{
	uint32_t addr = sector_address[sec] + index*sizeof(kved_word_t);

	return *((kved_word_t *)addr);
}

uint32_t kved_flash_sector_size(void)
{
	return FLASH_SECTOR_SIZE;
}

void kved_flash_init(void)
{
}
