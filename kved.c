/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

/**
@file
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "kved.h"
#include "kved_cpu.h"
#include "kved_flash.h"

// must match @ref kved_data_t
/** @private */
const uint8_t kved_data_type_size[] = 
{ 
	1,
	1,
	2,
	2,
	2,
	2,
	4,
	KVED_MAX_STRING_SIZE,
#if KVED_FLASH_WORD_SIZE >= 8
	8,
	8,
	8,
#endif
};

/** @private */
typedef struct kved_sector_stat_s
{
	uint16_t num_free_entries;    /**< @private */
	uint16_t num_deleted_entries; /**< @private */
	uint16_t num_used_entries;    /**< @private */
	uint16_t num_total_entries;   /**< @private */
} kved_sector_stat_t;

/** @private */
typedef struct kved_ctrl_s
{
	uint16_t first_index;       /**< @private */
	uint16_t first_free_index;  /**< @private */
	uint16_t last_index;        /**< @private */
	kved_sector_stat_t stats;   /**< @private */
	kved_flash_sector_t sector; /**< @private */
} kved_ctrl_t;

static kved_ctrl_t ctrl = { 0 };
static volatile bool started = false;

static void nv_sector_stats_erase(kved_sector_stat_t *stats)
{
	stats->num_deleted_entries = 0;
	stats->num_free_entries = 0;
	stats->num_total_entries = 0;
	stats->num_used_entries = 0;
}

#ifdef KVED_DEBUG
const uint8_t *kved_data_type_label[] = 
{ 
	(uint8_t *)"U8",
	(uint8_t *)"I8",
	(uint8_t *)"U16",
	(uint8_t *)"I16",
	(uint8_t *)"U32",
	(uint8_t *)"I32",
	(uint8_t *)"FLT",
	(uint8_t *)"STR",
#if KVED_FLASH_WORD_SIZE >= 8
	(uint8_t *)"U64",
	(uint8_t *)"I64",
	(uint8_t *)"DBL",
#endif
};

static void kved_print(kved_word_t val)
{
	uint8_t *p = (uint8_t *) &val;

	p += KVED_FLASH_WORD_SIZE - 1;
	for(uint8_t n = 0 ; n < sizeof(kved_word_t) ; n++, p--)
		printf("%02X",*p);
}

static void kved_print_ascii(kved_word_t val, size_t size, bool reverse)
{
	uint8_t *p = (uint8_t *) &val;

	if(reverse)
		p += size - 1;

	for(uint8_t n = 0 ; n < size ; n++)
	{
		if(isprint(*p))
		{
			printf("%c",*p);
		}
		else
		{
			printf(".");
		}

		if(reverse)
			p--;
		else
			p++;
	}
}

static void kved_internal_dump(kved_ctrl_t *ctrl)
{
	bool first_free_printed = false;
	kved_word_t hdr = kved_flash_data_read(ctrl->sector,0);
	kved_word_t cnt = kved_flash_data_read(ctrl->sector,1);

#if KVED_FLASH_WORD_SIZE == 8
	printf("HDR (SEC %c)     SIGNATURE        COUNTER\r\n",(ctrl->sector == 0 ? 'A' : 'B'));
#else
	printf("HDR (SEC %c)     SIGNAT.  COUNTER\r\n",(ctrl->sector == 0 ? 'A' : 'B'));
#endif	
	
	printf("ITEM IDX TYP SZ ");
	kved_print(hdr);
	printf(" ");
	kved_print(cnt);
	printf("\r\n");

	for(uint16_t index = ctrl->first_index ; index <= ctrl->last_index && !first_free_printed; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = kved_flash_data_read(ctrl->sector,index);
		kved_word_t val = kved_flash_data_read(ctrl->sector,index + 1);

		if(key == KVED_DELETED_ENTRY)
		{
			printf("DEL  ");
		}
		else if(key == KVED_FREE_ENTRY)
		{
			if(val == KVED_FREE_ENTRY)
			{
				printf("FREE ");
				first_free_printed = true;
			}
			else
			{
				printf("ERR1 ");
			}
		}
		else
		{
			printf("USED ");
		}

		uint8_t size = KVED_HDR_MASK_SIZE(key);
		uint8_t type = KVED_HDR_MASK_TYPE(key);

		if((val == KVED_FREE_ENTRY) || (key == KVED_DELETED_ENTRY))
		{
			printf("%03d        ",index);
		}
		else
		{
			printf("%03d %3s %02d ",index,(char *)kved_data_type_label[type],size);
		}
		kved_print(key);
		printf(" ");
		kved_print(val);
		printf(" ");
		kved_print_ascii(key,KVED_FLASH_WORD_SIZE,true);
		printf(" ");
		kved_print_ascii(val,KVED_FLASH_WORD_SIZE,type == KVED_DATA_TYPE_STRING ? false : true);
		printf("\r\n");
	}

	printf("TOTAL %d USED %d DELETED %d FREE %d\r\n\r\n",
			ctrl->stats.num_total_entries,
			ctrl->stats.num_used_entries,
			ctrl->stats.num_deleted_entries,
			ctrl->stats.num_free_entries);
}
#else
static void kved_internal_dump(kved_ctrl_t *ctrl)
{
}
#endif

static void kved_sector_stats_read(kved_ctrl_t *ctrl)
{
	// [0,NV_HDR_SIZE] ARE NOT VALID AS ENTRY INDEXES, THEY ARE RESERVED FOR HEADER
	ctrl->first_index = KVED_HDR_SIZE_IN_WORDS;
	ctrl->last_index = (kved_flash_sector_size()/KVED_FLASH_WORD_SIZE) - KVED_HDR_SIZE_IN_WORDS;
	ctrl->first_free_index = 0;

	nv_sector_stats_erase(&ctrl->stats);

	for(uint16_t index = ctrl->first_index ; index <= ctrl->last_index ; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = kved_flash_data_read(ctrl->sector,index);

		if(KVED_ENTRY_IS_EQUAL(key,KVED_DELETED_ENTRY))
		{
			ctrl->stats.num_deleted_entries++;
		}
		else if(KVED_ENTRY_IS_EQUAL(key,KVED_FREE_ENTRY))
		{
			ctrl->stats.num_free_entries++;

			if(ctrl->first_free_index == 0)
				ctrl->first_free_index = index;
		}
		else
		{
			ctrl->stats.num_used_entries++;
		}

		ctrl->stats.num_total_entries++;
	}
}

void kved_key_decode(kved_data_t *data, kved_word_t key)
{
	data->type = KVED_HDR_MASK_TYPE(key);

	uint8_t *pkey = (uint8_t *) &key;
	pkey += KVED_MAX_KEY_SIZE;

	for(size_t p = 0 ; p < KVED_MAX_KEY_SIZE ; p++)
		data->key[p] = *pkey--;
}

kved_word_t kved_key_encode(kved_data_t *data)
{
	uint8_t *str_key = data->key;
	uint8_t key[KVED_MAX_KEY_SIZE];

	strncpy((char *)key,(char *)data->key,KVED_MAX_KEY_SIZE);

	uint8_t size = data->type == KVED_DATA_TYPE_STRING ? strnlen((char *)str_key,KVED_MAX_KEY_SIZE) : kved_data_type_size[data->type];
	uint8_t hdr = (data->type << 4) | size;

	kved_word_t encoded_key = KVED_NULL_ENTRY;
	uint8_t *pkey = (uint8_t *) &encoded_key;
	pkey += KVED_MAX_KEY_SIZE;

	for(size_t p = 0 ; p < KVED_MAX_KEY_SIZE ; p++)
		*pkey-- = key[p];
	
	*pkey = hdr;

	return encoded_key;
}

static bool kved_is_valid_key(kved_word_t key)
{
	key = KVED_HDR_MASK_KEY(key);

	return (KVED_ENTRY_IS_EQUAL(key,KVED_HDR_MASK_KEY(KVED_SIGNATURE_ENTRY))) ||
		   (KVED_ENTRY_IS_EQUAL(key,KVED_HDR_MASK_KEY(KVED_DELETED_ENTRY))) ||
		   (KVED_ENTRY_IS_EQUAL(key,KVED_HDR_MASK_KEY(KVED_FREE_ENTRY))) ? false : true;
}

static uint16_t kved_key_index_find(kved_word_t key)
{
	uint16_t key_index = KVED_INDEX_NOT_FOUND;

	key = KVED_HDR_MASK_KEY(key);

	for(uint16_t index = ctrl.first_index ; index <= ctrl.last_index ; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key_entry = kved_flash_data_read(ctrl.sector,index);
		key_entry = KVED_HDR_MASK_KEY(key_entry);

		if(KVED_ENTRY_IS_EQUAL(key,key_entry))
		{
			key_index = index;
			break;
		}
	}

	return key_index;
}

static kved_word_t kved_value_encode(kved_data_t *data)
{
#if KVED_FLASH_WORD_SIZE >= 8
	return data->value.u64;
#else
	return data->value.u32;
#endif	
}

static void kved_value_decode(kved_data_t *data, kved_word_t value)
{
#if KVED_FLASH_WORD_SIZE >= 8
	data->value.u64 = value;
#else
	data->value.u32 = value;
#endif
}

static void kved_sector_switch(kved_ctrl_t *ctrl, kved_word_t cnt, kved_word_t upd_key, kved_word_t upd_value)
{
	uint16_t next_index = KVED_HDR_SIZE_IN_WORDS;
	uint16_t total_items = 0;
	uint16_t used_items = 0;
	kved_flash_sector_t next_sector = ctrl->sector == KVED_FLASH_SECTOR_A ? KVED_FLASH_SECTOR_B : KVED_FLASH_SECTOR_A;

	kved_flash_sector_erase(next_sector);

	upd_key = KVED_HDR_MASK_KEY(upd_key);

	for(uint16_t index = ctrl->first_index ; index <= ctrl->last_index ; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = kved_flash_data_read(ctrl->sector,index);

		if(kved_is_valid_key(key))
		{
			kved_word_t val = kved_flash_data_read(ctrl->sector,index + 1);

			kved_flash_data_write(next_sector,next_index++,key);

			if(KVED_ENTRY_IS_EQUAL(KVED_HDR_MASK_KEY(key),upd_key))
				kved_flash_data_write(next_sector,next_index++,upd_value);
			else
				kved_flash_data_write(next_sector,next_index++,val);

			used_items++;
		}

		total_items++;
	}

	kved_flash_sector_t last_sector = ctrl->sector;
	ctrl->sector = next_sector;
	ctrl->first_index = KVED_HDR_SIZE_IN_WORDS;
	ctrl->last_index = (kved_flash_sector_size()/KVED_FLASH_WORD_SIZE) - KVED_HDR_SIZE_IN_WORDS;
	ctrl->first_free_index = next_index;
	ctrl->stats.num_deleted_entries = 0;
	ctrl->stats.num_total_entries = total_items;
	ctrl->stats.num_used_entries = used_items;
	ctrl->stats.num_free_entries = total_items - used_items;

	// last value is not valid since it is equal to an erased flash entry
	if(KVED_ENTRY_IS_EQUAL(KVED_ENTRY_FROM_VALUE(cnt + 1),KVED_FLASH_UINT_MAX)) // last value, avoiding some #if #def related to flash size
		cnt = 0;
	else
		cnt++;

	kved_flash_data_write(next_sector,1,cnt);
	kved_flash_data_write(next_sector,0,KVED_SIGNATURE_ENTRY);

	kved_flash_data_write(last_sector,0,KVED_NULL_ENTRY); // only invalidate header, it is faster
}

static bool kved_internal_data_write(kved_data_t *data)
{
	bool sector_changed = false;

	if(!started)
		return false;

	kved_word_t key = kved_key_encode(data);

	if(!kved_is_valid_key(key))
		return false;

	uint16_t key_index = kved_key_index_find(key);
	bool old_entry = key_index != KVED_INDEX_NOT_FOUND;

	// check if the value has changed or not (for existing keys)
	if(old_entry)
	{
		kved_word_t stored_value = kved_flash_data_read(ctrl.sector,key_index + 1);

		if(KVED_ENTRY_IS_EQUAL(stored_value,kved_value_encode(data)))
			return true;
	}

	// no space, exchanging sector do not solve this situation, you need more flash space !
	if(ctrl.stats.num_total_entries == ctrl.stats.num_used_entries)
		return false;

	// ok, we have space but a clean up is required before. 
	// Let's do a sector switch and leave the garbage behind.
	// If we are writing using an existing entries it will
	// be their valued updated during the process. 
	if(ctrl.stats.num_free_entries == 0)
	{
		kved_word_t cnt = kved_flash_data_read(ctrl.sector,1);
		kved_sector_switch(&ctrl,cnt,key,kved_value_encode(data));
		sector_changed = true;
	}

	// AN existing entry is moved to the new sector using the new value already, not to do anymore.
	// However we need to take care when a new entry is added or when we are updating an entry 
	// in the same sector. 
	bool old_entry_updated_in_the_same_sector = (!sector_changed) && old_entry;

	if(!old_entry || old_entry_updated_in_the_same_sector)
	{
		// first data, after key
		kved_flash_data_write(ctrl.sector,ctrl.first_free_index + 1,kved_value_encode(data));
		kved_flash_data_write(ctrl.sector,ctrl.first_free_index,key);

		ctrl.stats.num_free_entries--;
		ctrl.stats.num_used_entries++;
		ctrl.first_free_index += KVED_ENTRY_SIZE_IN_WORDS;

		// Existing data written in the same sector: erase the old entry
		if(old_entry_updated_in_the_same_sector)
		{
			kved_flash_data_write(ctrl.sector,key_index,KVED_DELETED_ENTRY);

			ctrl.stats.num_deleted_entries++;
			ctrl.stats.num_used_entries--;
		}
	}

#ifdef KVED_DEBUG
	kved_dump();
#endif	

	return true;
}

bool kved_data_write(kved_data_t *data)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_data_write(data);
	kved_cpu_critical_section_leave();

	return result;
}

static uint16_t kved_internal_first_used_index_get(void)
{
	uint16_t first_index = KVED_INDEX_NOT_FOUND;

	for(uint16_t index = ctrl.first_index ; index <= ctrl.last_index ; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = kved_flash_data_read(ctrl.sector,index);

		if(kved_is_valid_key(key))
		{
			first_index = index;
			break;
		}
	}

	return first_index;
}

uint16_t kved_first_used_index_get(void)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_first_used_index_get();
	kved_cpu_critical_section_leave();

	return result;
}

static uint16_t kved_internal_next_used_index_get(uint16_t last_index)
{
	uint16_t next_index = KVED_INDEX_NOT_FOUND;

	for(uint16_t index = last_index + KVED_ENTRY_SIZE_IN_WORDS ; index <= ctrl.last_index ; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = kved_flash_data_read(ctrl.sector,index);

		if(kved_is_valid_key(key))
		{
			next_index = index;
			break;
		}
	}

	return next_index;
}

uint16_t kved_next_used_index_get(uint16_t last_index)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_next_used_index_get(last_index);
	kved_cpu_critical_section_leave();

	return result;
}

static bool kved_internal_data_read_by_index(uint16_t index, kved_data_t *data)
{
	if((index < ctrl.first_index) || (index > ctrl.last_index))
		return false;

	kved_word_t key = kved_flash_data_read(ctrl.sector,index);

	if(!kved_is_valid_key(key))
		return false;

	kved_word_t val = kved_flash_data_read(ctrl.sector,index + 1);
	kved_value_decode(data,val);
	kved_key_decode(data,key);

	return true;
}

bool kved_data_read_by_index(uint16_t index, kved_data_t *data)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_data_read_by_index(index,data);
	kved_cpu_critical_section_leave();

	return result;
}

static uint16_t kved_internal_free_entries_get(void)
{
	if(!started)
		return 0;

	uint16_t entries = ctrl.stats.num_total_entries - ctrl.stats.num_used_entries;

	return entries;
}

uint16_t kved_free_entries_get(void)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_free_entries_get();
	kved_cpu_critical_section_leave();

	return result;
}

static uint16_t kved_internal_total_entries_get(void)
{
	if(!started)
		return 0;

	return ctrl.stats.num_total_entries;
}

uint16_t kved_total_entries_get(void)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_total_entries_get();
	kved_cpu_critical_section_leave();

	return result;
}

static uint16_t kved_internal_used_entries_get(void)
{
	if(!started)
		return 0;

	return ctrl.stats.num_used_entries;
}

uint16_t kved_used_entries_get(void)
{
	uint16_t result;

	kved_cpu_critical_section_enter();
	result = kved_internal_used_entries_get();
	kved_cpu_critical_section_leave();

	return result;
}

static bool kved_internal_data_read(kved_data_t *data)
{
	if(!started)
		return false;

	kved_word_t key = kved_key_encode(data);

	if(!kved_is_valid_key(key))
		return false;

	uint16_t key_index = kved_key_index_find(key);

	if(key_index == KVED_INDEX_NOT_FOUND)
		return false;

	 // update the type as user may not know about them before calling
	data->type = KVED_HDR_MASK_TYPE(kved_flash_data_read(ctrl.sector,key_index));

	 kved_word_t value = kved_flash_data_read(ctrl.sector,key_index+1);
	 kved_value_decode(data,value);

	return true;
}

bool kved_data_read(kved_data_t *data)
{
	bool result;

	kved_cpu_critical_section_enter();
	result = kved_internal_data_read(data);
	kved_cpu_critical_section_leave();

	return result;
}

static bool kved_internal_data_delete(kved_data_t *data)
{
	if(!started)
		return false;

	kved_word_t key = kved_key_encode(data);

	if(!kved_is_valid_key(key))
		return false;

	uint16_t key_index = kved_key_index_find(key);

	if(key_index == KVED_INDEX_NOT_FOUND)
		return false;

	kved_flash_data_write(ctrl.sector,key_index,KVED_DELETED_ENTRY);

	ctrl.stats.num_deleted_entries++;
	ctrl.stats.num_used_entries--;

#ifdef KVED_DEBUG
	kved_dump();
#endif	

	return true;
}

bool kved_data_delete(kved_data_t *data)
{
	bool result;

	kved_cpu_critical_section_enter();
	result = kved_internal_data_delete(data);
	kved_cpu_critical_section_leave();

	return result;
}

static void kved_internal_format(void)
{
	// erase data and control
	kved_flash_sector_erase(KVED_FLASH_SECTOR_A);
	kved_flash_sector_erase(KVED_FLASH_SECTOR_B);
	memset(&ctrl,0,sizeof(ctrl));

	// setup sector as first sector and update stats.
	// Consistency is not required in such situation.
	ctrl.sector  = KVED_FLASH_SECTOR_A;
	kved_flash_data_write(ctrl.sector,1,KVED_NULL_ENTRY);// first cnt, after ID
	kved_flash_data_write(ctrl.sector,0,KVED_SIGNATURE_ENTRY);

	kved_sector_stats_read(&ctrl);
}

void kved_format(void)
{
	kved_cpu_critical_section_enter();
	kved_internal_format();
	kved_cpu_critical_section_leave();
}

static void kved_sector_consistency_check(void)
{
	bool invalidate_a = false;
	bool invalidate_b = false;

	kved_word_t id_sec_a = kved_flash_data_read(KVED_FLASH_SECTOR_A,0);
	kved_word_t id_sec_b = kved_flash_data_read(KVED_FLASH_SECTOR_B,0);

	// Two valid signatures: as the signature is the latest item to be written into the sector
	// when a formatting or copy operation is performed, probably a restart event happened just 
	// after data copying and, in this case, the section with the
	// newest cnt will win and the other can be erase as the copy was done.
	// (remember: last value (0xFF..FF) is not valid as it is the same value of a erased word
	if(KVED_ENTRY_IS_EQUAL(id_sec_a,KVED_SIGNATURE_ENTRY) && KVED_ENTRY_IS_EQUAL(id_sec_b,KVED_SIGNATURE_ENTRY))
	{
		kved_word_t cnt_sec_a = kved_flash_data_read(KVED_FLASH_SECTOR_A,1);
		kved_word_t cnt_sec_b = kved_flash_data_read(KVED_FLASH_SECTOR_B,1);

		// the (a) counter rolled over and the
		// sector (a->b) copy was done so erase older sector (a) and use the newer (b)
		if((cnt_sec_a == (KVED_FLASH_UINT_MAX-1)) && KVED_ENTRY_IS_EQUAL(cnt_sec_b,0))
		{
			invalidate_a = true;
		}
		// the (b) counter rolled over and the
		// sector (b->a) copy was done so erase older sector (b) and use the newer (a)
		else if((cnt_sec_b == (KVED_FLASH_UINT_MAX-1)) && (cnt_sec_a == 0))
		{
			invalidate_b = true;
		}
		// unexpected situation since counter can not be KVED_FLASH_UINT_MAX: we 
		// will invalidate the sector. Maybe some people would like to analize it 
		// and rescue some valid entries. Not implemented, anyway.
		else if((cnt_sec_a == KVED_FLASH_UINT_MAX) || (cnt_sec_b == KVED_FLASH_UINT_MAX))
		{
			invalidate_a = KVED_ENTRY_IS_EQUAL(cnt_sec_a,KVED_FLASH_UINT_MAX);
			invalidate_b = KVED_ENTRY_IS_EQUAL(cnt_sec_b,KVED_FLASH_UINT_MAX);
		}

		// ok, regular situation where one counter is the newest and 
		// all values are normal, just choose the newest one
		if((invalidate_a == false) && (invalidate_b == false))
		{
			if(cnt_sec_a > cnt_sec_b)
					invalidate_b = true;
				else
					invalidate_a = true;
		}

		// Invalidate selected sectors ...
		if(invalidate_a)
			kved_flash_data_write(KVED_FLASH_SECTOR_A,0,KVED_NULL_ENTRY);

		if(invalidate_b)
			kved_flash_data_write(KVED_FLASH_SECTOR_B,0,KVED_NULL_ENTRY);
	}
}

static void kved_data_consistency_check(void)
{
	for(uint16_t index = ctrl.first_index ; index <= ctrl.last_index ; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = kved_flash_data_read(ctrl.sector,index);
		kved_word_t val = kved_flash_data_read(ctrl.sector,index + 1);

		// As we write value first and key after and we may be powered off during this operation
		// it is necessary to fix cases where only the value was written. In such situation
		// a key with only FFs may exist and this entry will be deleted.
		// If the power down occurs when key is been written it is not possible to
		// detect if the operation was finished or not. But this case need to be solved
		// by application, removing any unknown or unexpected key (application knows its owns keys, kved not). 
		if((key == KVED_FLASH_UINT_MAX) && (val != KVED_FLASH_UINT_MAX))
		{
			kved_flash_data_write(ctrl.sector,index,KVED_NULL_ENTRY);
			ctrl.stats.num_deleted_entries++;
			ctrl.stats.num_free_entries--;
			continue;
		}

		// Second possible situation: new data is written in the same section and the
		// old value is not erased due some power down.
		// So, it is required to check for duplicated keys AHEAD (always)
		// and erase the old entry.
		if(kved_is_valid_key(key))
		{
			for(uint16_t dup_key_index = index + KVED_ENTRY_SIZE_IN_WORDS ; dup_key_index <= ctrl.last_index ; dup_key_index += KVED_ENTRY_SIZE_IN_WORDS)
			{
				kved_word_t dup_key = kved_flash_data_read(ctrl.sector,dup_key_index);
				if(kved_is_valid_key(dup_key))
				{
					if(KVED_HDR_MASK_KEY(dup_key) == KVED_HDR_MASK_KEY(key))
					{
						kved_flash_data_write(ctrl.sector,index,KVED_NULL_ENTRY);
						ctrl.stats.num_deleted_entries++;
						ctrl.stats.num_used_entries--;
						break;
					}
				}
			}
		}
	}
}

void kved_dump(void)
{
	kved_cpu_critical_section_enter();
	kved_internal_dump(&ctrl);
	kved_cpu_critical_section_leave();
}

void kved_init(void)
{
	kved_flash_init();

	kved_sector_consistency_check();

	kved_word_t id_sec_a = kved_flash_data_read(KVED_FLASH_SECTOR_A,0);
	kved_word_t id_sec_b = kved_flash_data_read(KVED_FLASH_SECTOR_B,0);

	if(KVED_ENTRY_IS_EQUAL(id_sec_a,KVED_SIGNATURE_ENTRY))
	{
		ctrl.sector = KVED_FLASH_SECTOR_A;
	}
	else if(KVED_ENTRY_IS_EQUAL(id_sec_b,KVED_SIGNATURE_ENTRY))
	{
		ctrl.sector = KVED_FLASH_SECTOR_B;
	}
	else
	{
		ctrl.sector  = KVED_FLASH_SECTOR_A;
		kved_flash_sector_erase(ctrl.sector);
		kved_flash_data_write(ctrl.sector,1,0);// first cnt, after ID
		kved_flash_data_write(ctrl.sector,0,KVED_SIGNATURE_ENTRY);
	}

	kved_sector_stats_read(&ctrl);

	kved_data_consistency_check();

	started = true;

#ifdef KVED_DEBUG
	kved_dump();
#endif	
}
