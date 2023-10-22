/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2023 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kved.h"
#include "kved_flash.h"
#include "kved_cpu.h"
#include "main.h"


#define FLASH_SECTOR_SIZE 8192

const uint32_t sector_size[KVED_FLASH_NUM_SECTORS] = { FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE };
const uint32_t sector_address[KVED_FLASH_NUM_SECTORS] = { 0x0807c000, 0x0807e000 };
const uint8_t sector_page[KVED_FLASH_NUM_SECTORS] = { 30, 31 };
const uint8_t flash_bank = 2;

static void kved_flash_unlock(void)
{
	HAL_FLASH_Unlock();
}

static void kved_flash_lock(void)
{
	HAL_FLASH_Lock();
}

static void kved_flash_cache_disable(void)
{
	HAL_ICACHE_Disable();
}

static void kved_flash_cache_restore(void)
{
	HAL_ICACHE_Enable();
}

bool kved_flash_sector_erase(kved_flash_sector_t sec_idx)
{
    uint32_t page_index = sector_page[sec_idx];
    uint32_t pgerr;
    FLASH_EraseInitTypeDef cfg;

    kved_flash_unlock();
    kved_flash_cache_disable();

    cfg.TypeErase   = FLASH_TYPEERASE_PAGES;
    cfg.Banks       = flash_bank;
    cfg.Page        = page_index;
    cfg.NbPages     = 1;

    HAL_FLASHEx_Erase(&cfg,&pgerr);

    kved_flash_cache_restore();
    kved_flash_lock();

	return true;
}

void kved_flash_data_write(kved_flash_sector_t sec_idx, uint16_t index, kved_word_t data)
{
    uint32_t addr = sector_address[sec_idx] + index*sizeof(kved_word_t);

    kved_flash_unlock();
    kved_flash_cache_disable();

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,addr,(uint32_t)&data);

    kved_flash_cache_restore();
    kved_flash_lock();
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
