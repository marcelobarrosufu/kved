/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#pragma once

/**
@file
@defgroup KVED_FLASH KVED_FLASH
@brief kved flash API
@{
*/

/**
@brief Flash sectors that can be used for data persistence.
Positions and mappings will depend on the driver implementation.
*/
typedef enum kved_flash_sector_e
{
	KVED_FLASH_SECTOR_A = 0, /**< Setor A */
	KVED_FLASH_SECTOR_B,     /**< Setor B */
	KVED_FLASH_NUM_SECTORS,  /**< Number of sectors */
} kved_flash_sector_t;

/**
@brief Delete a flash sector
  @param[in] sec - sector to be deleted (see @ref kved_flash_sector_e)
  @return true: sector erased
  @return false: sector erasuring failed
*/
bool kved_flash_sector_erase(kved_flash_sector_t sec);

/**
@brief Write a word value into the flash sector
  @param[in] sec - sector (see @ref kved_flash_sector_e)
  @param[in] index - index position
  @param[in] data - value to written (word)
*/
void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data);

/**
@brief Reads a word value from the flash sector
  @param[in] sec - sector (see @ref kved_flash_sector_e)
  @param[in] index - index position
  @return read value (word)
*/
kved_word_t kved_flash_data_read(kved_flash_sector_t sec, uint16_t index);

/**
@brief Returns the sector size
  @return Sector size, in bytes
*/
uint32_t kved_flash_sector_size(void);

/**
 @brief Flash initialization. It will depend on the driver implementation.
*/
void kved_flash_init(void);

/**
@}
*/
