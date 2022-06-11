/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "kved.h"
#include "kved_cpu.h"
#include "kved_flash.h"

void kved_value_test(void)
{
	kved_value_t v;

	kved_data_t d = { .key = "A1" };

	// U8
	d.type = KVED_DATA_TYPE_UINT8;
	v.u8 = UINT8_MAX;
	v.u8 = d.value.u8;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.u8 == d.value.u8);

	// I8
	d.type = KVED_DATA_TYPE_INT8;
	d.value.i8 = INT8_MIN;
	v.i8 = d.value.i8;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.i8 == d.value.i8);

	// U16
	d.type = KVED_DATA_TYPE_UINT16;
	v.u16 = UINT16_MAX;
	v.u16 = d.value.u16;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.u16 == d.value.u16);

	// I16
	d.type = KVED_DATA_TYPE_INT16;
	d.value.i16 = INT16_MIN;
	v.i16 = d.value.i16;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.i16 == d.value.i16);

	// U32
	d.type = KVED_DATA_TYPE_UINT32;
	v.u32 = UINT32_MAX;
	v.u32 = d.value.u32;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.u32 == d.value.u32);

	// I32
	d.type = KVED_DATA_TYPE_INT32;
	d.value.i32 = INT32_MIN;
	v.i32 = d.value.i32;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.i32 == d.value.i32);

	// FLOAT
	d.type = KVED_DATA_TYPE_FLOAT;
	d.value.flt = 3.141592;
	v.flt = d.value.flt;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.flt == d.value.flt);

	// STRING
	d.type = KVED_DATA_TYPE_STRING;
	v.str[0] = d.value.str[0] = 'A'; 
	v.str[1] = d.value.str[1] = 'B'; 
	v.str[2] = d.value.str[2] = 'C';
	v.str[3] = d.value.str[3] = '\0';
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.str[0] == d.value.str[0]);
	assert(v.str[1] == d.value.str[1]);
	assert(v.str[2] == d.value.str[2]);
	assert(v.str[3] == d.value.str[3]);

	// Forcing roll over (sector a)
	d.type = KVED_DATA_TYPE_UINT32;
	for(size_t n = 0 ; n < (kved_flash_sector_size()/(2*sizeof(kved_word_t))) ; n++)
	{
		d.value.u32 = (n << 24) | (n << 16) | (n << 8) |n;
		v.u32 = d.value.u32;
		kved_data_write(&d);
		kved_data_read(&d);
		assert(v.u32 == d.value.u32);
	}

	// Forcing roll over (sector b)
	d.type = KVED_DATA_TYPE_UINT32;
	for(size_t n = 0 ; n < (kved_flash_sector_size()/(2*sizeof(kved_word_t))) ; n++)
	{
		d.value.u32 = (n << 24) | (n << 16) | (n << 8) |n;
		v.u32 = d.value.u32;
		kved_data_write(&d);
		kved_data_read(&d);
		assert(v.u32 == d.value.u32);
	}

#if KVED_FLASH_WORD_SIZE == 8
	// U64
	d.type = KVED_DATA_TYPE_UINT64;
	d.value.u64 = UINT64_MAX;
	v.u64 = d.value.u64;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.u64 == d.value.u64);

	// I64
	d.type = KVED_DATA_TYPE_INT64;
	d.value.i64 = INT64_MIN;
	v.i64 = d.value.i64;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.i64 == d.value.i64);

	// DOUBLE
	d.type = KVED_DATA_TYPE_DOUBLE;
	d.value.dbl = 3.141592;
	v.dbl = d.value.dbl;
	kved_data_write(&d);
	kved_data_read(&d);
	assert(v.dbl == d.value.dbl);

	kved_data_t long_key = {
		.type = KVED_DATA_TYPE_STRING,
		.key = "calib3",
		.value.str ="KVECTOP"
	};

	v.str[0] = d.value.str[0]; 
	v.str[1] = d.value.str[1]; 
	v.str[2] = d.value.str[2];
	v.str[3] = d.value.str[3];
	v.str[4] = d.value.str[4]; 
	v.str[5] = d.value.str[5]; 
	v.str[6] = d.value.str[6];
	v.str[7] = d.value.str[7];

	kved_data_write(&long_key);
	kved_data_read(&long_key);
	assert(v.str[0] == d.value.str[0]);
	assert(v.str[1] == d.value.str[1]);
	assert(v.str[2] == d.value.str[2]);
	assert(v.str[3] == d.value.str[3]);
	assert(v.str[4] == d.value.str[4]);
	assert(v.str[5] == d.value.str[5]);
	assert(v.str[6] == d.value.str[6]);
	assert(v.str[7] == d.value.str[7]);

#endif
}

void kved_header_test(void)
{
	kved_init();
    kved_dump();

    // test sector selection by newer counter
	kved_flash_sector_erase(KVED_FLASH_SECTOR_A);
    kved_flash_sector_erase(KVED_FLASH_SECTOR_B);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,1,10);
	kved_flash_data_write(KVED_FLASH_SECTOR_B,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_B,1,11);

	kved_init();
    kved_dump();

    // test error condition when max counter is reached
	kved_flash_sector_erase(KVED_FLASH_SECTOR_A);
	kved_flash_sector_erase(KVED_FLASH_SECTOR_B);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,1,KVED_FLASH_UINT_MAX);
    kved_flash_data_write(KVED_FLASH_SECTOR_B,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_B,1,10);

	kved_init();
    kved_dump();

    // test sector roll over
	kved_flash_sector_erase(KVED_FLASH_SECTOR_A);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,1,KVED_FLASH_UINT_MAX-1);
	kved_flash_sector_erase(KVED_FLASH_SECTOR_B);
	kved_flash_data_write(KVED_FLASH_SECTOR_B,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_B,1,0);

	kved_init();
    kved_dump();
}

void kved_key_test(void)
{      
	kved_flash_sector_erase(KVED_FLASH_SECTOR_A);
	kved_flash_sector_erase(KVED_FLASH_SECTOR_B);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,0,KVED_SIGNATURE_ENTRY);
	kved_flash_data_write(KVED_FLASH_SECTOR_A,1,0);

	kved_init();

	kved_data_t d1 = {
			.type = KVED_DATA_TYPE_UINT32,
			.key = "c1",
			.value.u32 = 0x12345678
	};

	kved_data_t d2 = {
			.type = KVED_DATA_TYPE_UINT8,
			.key = "c2",
			.value.u8 = 0xAA
	};

	kved_data_t d3 = {
			.type = KVED_DATA_TYPE_STRING,
			.key = "c3",
			.value.str ="MAR"
	};

	kved_data_write(&d1);
	kved_data_write(&d2);
	kved_data_write(&d3);

	// duplicated keys
	kved_flash_data_write(KVED_FLASH_SECTOR_A,8,kved_key_encode(&d2));
	kved_flash_data_write(KVED_FLASH_SECTOR_A,9,0x12345);
	
    kved_dump();
	kved_init();

	// invalid entry (key not written)
	kved_flash_data_write(KVED_FLASH_SECTOR_A,11,0x12345);

	kved_dump();
	kved_init();

	d2.type = KVED_DATA_TYPE_INT16;
	d2.value.i16 = -1;
	kved_data_write(&d2);

	kved_init();
}
