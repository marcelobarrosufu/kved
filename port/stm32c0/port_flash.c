/*
kved (key/value embedded database), a simple key/value database
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>

Auther: James Hunt <huntj88@gmail.com>
Tested on STM32C071RB mcu
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kved.h"
#include "kved_flash.h"
#include "main.h"

// Address of page zero of our flash
#define FLASH_PAGE_ZERO_ADDRESS  0x08000000   
#define FLASH_PAGE_SECTOR_SIZE 2048

// Page 31 and 32 are used for sectors A and B respectively, these values can be changed to suit your application
const uint8_t pages[KVED_FLASH_NUM_SECTORS] = { 31, 32 };

bool kved_flash_sector_erase(kved_flash_sector_t sec)
{
	uint32_t sector_error;
	FLASH_EraseInitTypeDef sector =
	{ 
		.TypeErase = FLASH_TYPEERASE_PAGES,
		.NbPages = 1,
	};

	sector.Page = pages[sec];

	HAL_FLASH_Unlock();
	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&sector,&sector_error);
	HAL_FLASH_Lock();

	return status == HAL_OK;
}

uint32_t getHexAddressPage(int dataPage){
	uint32_t bits       = FLASH_PAGE_SECTOR_SIZE * dataPage;
	uint32_t hexAddress = FLASH_PAGE_ZERO_ADDRESS + bits;
	return hexAddress;
}

void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data)
{
	uint32_t addr = getHexAddressPage(pages[sec]) + index*sizeof(kved_word_t);

	HAL_FLASH_Unlock();
	HAL_FLASH_Program(TYPEPROGRAM_DOUBLEWORD,addr,data);
	HAL_FLASH_Lock();
}

kved_word_t kved_flash_data_read(kved_flash_sector_t sec, uint16_t index)
{
	uint32_t addr = getHexAddressPage(pages[sec]) + index*sizeof(kved_word_t);
	return *((kved_word_t *)addr);
}

uint32_t kved_flash_sector_size(void)
{
	return FLASH_PAGE_SECTOR_SIZE;
}

void kved_flash_init(void)
{
}
