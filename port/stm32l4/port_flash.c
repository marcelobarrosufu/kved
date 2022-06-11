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

#define FLASH_SECTOR_SIZE 2048

const uint32_t sector_size[KVED_FLASH_NUM_SECTORS] = { FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE };
const uint32_t sector_address[KVED_FLASH_NUM_SECTORS] = { 0x0803F000, 0x0803F800 };
const uint8_t sector_page[KVED_FLASH_NUM_SECTORS] = { 126, 127 };

static volatile bool started = false;
static volatile bool reactivate_icache = false;
static volatile bool reactivate_dcache = false;

static void kved_flash_unlock(void)
{
	if(READ_BIT(FLASH->CR, FLASH_CR_LOCK))
	{
		WRITE_REG(FLASH->KEYR, FLASH_KEY1);
		WRITE_REG(FLASH->KEYR, FLASH_KEY2);
	}
}

static void kved_flash_lock(void)
{
	SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}

static void kved_flash_cache_disable(void)
{
	reactivate_icache = false;
	reactivate_dcache = false;

	if(READ_BIT(FLASH->ACR, FLASH_ACR_ICEN))
	{
		LL_FLASH_DisableInstCache();
		reactivate_icache = true;
	}

	if(READ_BIT(FLASH->ACR, FLASH_ACR_DCEN))
	{
		LL_FLASH_DisableDataCache();
		reactivate_dcache = true;
	}
}

static void kved_flash_cache_restore(void)
{
	if(reactivate_icache)
	{
		LL_FLASH_EnableInstCacheReset();
		LL_FLASH_EnableInstCache();
	}

	if(reactivate_dcache)
	{
		LL_FLASH_EnableDataCacheReset();
		LL_FLASH_EnableDataCache();
	}
}

static void kved_flash_operation_wait(void)
{
	while(READ_BIT(FLASH->SR, FLASH_SR_BSY))
	{}
}

static void kved_flash_check_errors(void)
{
	if(READ_BIT(FLASH->SR, FLASH_SR_PROGERR))
			SET_BIT(FLASH->SR, FLASH_SR_PROGERR);

	if(READ_BIT(FLASH->SR, FLASH_SR_WRPERR))
			SET_BIT(FLASH->SR, FLASH_SR_WRPERR);

	if(READ_BIT(FLASH->SR, FLASH_SR_PGAERR))
			SET_BIT(FLASH->SR, FLASH_SR_PGAERR);

	if(READ_BIT(FLASH->SR, FLASH_SR_SIZERR))
			SET_BIT(FLASH->SR, FLASH_SR_SIZERR);

	if(READ_BIT(FLASH->SR, FLASH_SR_PGSERR))
			SET_BIT(FLASH->SR, FLASH_SR_PGSERR);

	if(READ_BIT(FLASH->SR, FLASH_SR_MISERR))
			SET_BIT(FLASH->SR, FLASH_SR_MISERR);

	if(READ_BIT(FLASH->SR, FLASH_SR_FASTERR))
			SET_BIT(FLASH->SR, FLASH_SR_FASTERR);
}

bool kved_flash_sector_erase(kved_flash_sector_t sec)
{
	uint32_t page_index = sector_page[sec];

	kved_flash_unlock();
	kved_flash_cache_disable();

	kved_flash_operation_wait();
	kved_flash_check_errors();

	MODIFY_REG(FLASH->CR, FLASH_CR_PNB, ((page_index & 0xFFU) << FLASH_CR_PNB_Pos));
	SET_BIT(FLASH->CR, FLASH_CR_PER);
	SET_BIT(FLASH->CR, FLASH_CR_STRT);

	kved_flash_operation_wait();

	CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));

	kved_flash_cache_restore();
	kved_flash_lock();

	return true;
}

void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data)
{
	uint32_t addr = sector_address[sec] + index*sizeof(kved_word_t);

	kved_flash_unlock();
	kved_flash_cache_disable();

	// Check that no Flash main memory operation is ongoing by checking the BSY bit in the Flash status register (FLASH_SR).
	kved_flash_operation_wait();

	// Check and clear all error programming flags due to a previous programming. If not, PGSERR is set.
	kved_flash_check_errors();

	//  Set the PG bit in the Flash control register (FLASH_CR)
	SET_BIT(FLASH->CR, FLASH_CR_PG);

	// Write a first word in an address aligned with double word
	// Write the second word

	*(__IO uint32_t*)addr = (uint32_t)data;
	/* Barrier to ensure programming is performed in 2 steps, in right order
	    (independently of compiler optimization behavior) */
	__ISB();

	/* Program second word */
	*(__IO uint32_t*)(addr+4U) = (uint32_t)(data >> 32);

	 // Wait until the BSY bit is cleared in the FLASH_SR register
	 kved_flash_operation_wait();

	 // Check that EOP flag is set in the FLASH_SR register (meaning that the programming operation has succeed), and clear it by software.
	 // (somente se tem interrupcao)

	 // Clear the PG bit in the FLASH_CR register if there no more programming request anymore.
	 CLEAR_BIT(FLASH->CR, FLASH_CR_PG);

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
