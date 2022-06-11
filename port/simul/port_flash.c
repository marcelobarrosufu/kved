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

#define FLASH_NUM_ENTRIES (16)
#define FLASH_SECTOR_SIZE (FLASH_NUM_ENTRIES*KVED_FLASH_WORD_SIZE)

kved_word_t data_bank0[FLASH_NUM_ENTRIES];
kved_word_t data_bank1[FLASH_NUM_ENTRIES];
kved_word_t *sector_address[KVED_FLASH_NUM_SECTORS] = { data_bank0, data_bank1 };


bool kved_flash_sector_erase(kved_flash_sector_t sec)
{
	memset(sector_address[sec],0xFF,FLASH_SECTOR_SIZE);

	return true;
}

void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data)
{
	sector_address[sec][index] = data;
}

kved_word_t kved_flash_data_read(kved_flash_sector_t sec, uint16_t index)
{
	return sector_address[sec][index];
}

uint32_t kved_flash_sector_size(void)
{
	// sector sizes must be equal
	return FLASH_SECTOR_SIZE;
}

void kved_flash_init(void)
{
	kved_flash_sector_erase(KVED_FLASH_SECTOR_A);
	kved_flash_sector_erase(KVED_FLASH_SECTOR_B);
}
