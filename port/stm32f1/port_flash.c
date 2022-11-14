/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Fabricio Lucas de Almeida <fabriciolucasfbr@gmail.com>
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kved.h"
#include "kved_flash.h"
#include "kved_cpu.h"
#include "main.h"

#define FLASH_KEY1 0x45670123U  /*!< Flash key1 */
#define FLASH_KEY2 0xCDEF89ABU  /*!< Flash key2 */

#define FLASH_SECTOR_SIZE 1024

const uint32_t sector_size[KVED_FLASH_NUM_SECTORS] = { FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE };
const uint32_t sector_address[KVED_FLASH_NUM_SECTORS] = { 0x0800F800, 0x0800FC00 };
const uint8_t sector_page[KVED_FLASH_NUM_SECTORS] = { 62, 63 };

static void kved_flash_unlock(void)
{
	if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0U)
	{
		WRITE_REG(FLASH->KEYR, FLASH_KEY1);
		WRITE_REG(FLASH->KEYR, FLASH_KEY2);
	}
}

static void kved_flash_lock(void)
{
	SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}

static void kved_flash_operation_wait(void)
{
	while(READ_BIT(FLASH->SR, FLASH_SR_BSY))
	{}
}

bool kved_flash_sector_erase(kved_flash_sector_t sec)
{
	kved_flash_unlock();

    SET_BIT(FLASH->CR, FLASH_CR_PER);
    WRITE_REG(FLASH->AR, sector_address[sec]);
    SET_BIT(FLASH->CR, FLASH_CR_STRT);
	kved_flash_operation_wait();
	CLEAR_BIT(FLASH->CR, FLASH_CR_PER);

	kved_flash_lock();

	return true;
}

void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data)
{
	uint32_t addr = sector_address[sec] + index*sizeof(kved_word_t);

	kved_flash_unlock();

	/* The Flash memory can be programmed 16 bits (half words) at a time.*/
    for (uint8_t idx_half_world = 0U; idx_half_world < PORT_KVED_FLASH_WORD_SIZE / 2; idx_half_world++)
    {
    	SET_BIT(FLASH->CR, FLASH_CR_PG);
    	*(__IO uint16_t*)(addr + (2U * idx_half_world)) = (uint16_t)(data >> (16U * idx_half_world));
    	kved_flash_operation_wait();
    	CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
    }

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
