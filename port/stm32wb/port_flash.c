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

#define FLASH_SECTOR_SIZE 4096

const uint32_t sector_size[KVED_FLASH_NUM_SECTORS] = { FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE };
const uint32_t sector_address[KVED_FLASH_NUM_SECTORS] = { 0x0807E000, 0x0807F000 };
const uint8_t sector_page[KVED_FLASH_NUM_SECTORS] = { 126, 127 };
/* need define -> #define KVED_FLASH_WORD_SIZE (8) */

static volatile bool reactivate_icache = false;
static volatile bool reactivate_dcache = false;

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

static void kved_flash_cache_disable(void)
{
	if (READ_BIT(FLASH->ACR, FLASH_ACR_ICEN) == FLASH_ACR_ICEN)
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

/* See AN5289 Rev 7 - pag 36 for more informations */
bool kved_flash_sector_erase(kved_flash_sector_t sec_idx)
{
	uint32_t page_index = sector_page[sec_idx];

#ifdef HAL_HSEM_MODULE_ENABLED
	while(HAL_HSEM_Take(2, 0) != HAL_OK){}
#endif
	kved_flash_unlock();
	kved_flash_cache_disable();
	while(READ_BIT(FLASH->SR, FLASH_SR_PESD)){}
	kved_cpu_critical_section_enter();

#ifdef HAL_HSEM_MODULE_ENABLED
	while(HAL_HSEM_IsSemTaken(6)){}
	while(HAL_HSEM_Take(7, 0) != HAL_OK){}
#endif

	MODIFY_REG(FLASH->CR, FLASH_CR_PNB, ((page_index & 0xFFU) << FLASH_CR_PNB_Pos));
	SET_BIT(FLASH->CR, FLASH_CR_PER);
	SET_BIT(FLASH->CR, FLASH_CR_STRT);
	kved_flash_operation_wait();
	CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));

#ifdef HAL_HSEM_MODULE_ENABLED
	HAL_HSEM_Release(7, 0);
#endif

	kved_cpu_critical_section_leave();
	kved_flash_cache_restore();
	kved_flash_lock();

#ifdef HAL_HSEM_MODULE_ENABLED
	HAL_HSEM_Release(2, 0);
#endif

	return true;
}

/* See AN5289 Rev 7 - pag 36 for more informations */
void kved_flash_data_write(kved_flash_sector_t sec_idx, uint16_t index, kved_word_t data)
{
	uint32_t addr = sector_address[sec_idx] + index*sizeof(kved_word_t);

#ifdef HAL_HSEM_MODULE_ENABLED
	while(HAL_HSEM_Take(2, 0) != HAL_OK){}
#endif
	kved_flash_unlock();
	kved_flash_cache_disable();

	while(READ_BIT(FLASH->SR, FLASH_SR_PESD)){}

	kved_cpu_critical_section_enter();

#ifdef HAL_HSEM_MODULE_ENABLED
	while(HAL_HSEM_IsSemTaken(6)){}
	while(HAL_HSEM_Take(7, 0) != HAL_OK){}
#endif

	SET_BIT(FLASH->CR, FLASH_CR_PG);
	*(__IO uint32_t*)addr = (uint32_t)data;
	__ISB();
	*(__IO uint32_t*)(addr+4U) = (uint32_t)(data >> 32);
	CLEAR_BIT(FLASH->CR, FLASH_CR_PG);

#ifdef HAL_HSEM_MODULE_ENABLED
	HAL_HSEM_Release(7, 0);
#endif
	kved_cpu_critical_section_leave();
	kved_flash_operation_wait();
	kved_flash_cache_restore();
	kved_flash_lock();

#ifdef HAL_HSEM_MODULE_ENABLED
	HAL_HSEM_Release(2, 0);
#endif
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
