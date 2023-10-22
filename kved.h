/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#pragma once

/**
@file
@defgroup KVED KVED
@brief KVED (key/value embedded databse): simple key/value persistence for embedded applications.
@{
*/

#include "kved_config.h"

/* kved structure

<-flashword->|<-flashword->  <= 8 bytes when using flash with word of 64 bits , for instance
+------------+------------+
| SIGNATURE  |  COUNTER   | <= HEADER ID AND NEWER COPY IDENTIFICATION
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  | <= VALID KEY (KEY ENTRY, TYPE, SIZE AND VALUE)
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  |
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  |
+---------+--+------------+
|000 ... 0000| KEY VALUE  |  <= ERASED KEY
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|  <= EMPTY ENTRY
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|
+------------+------------+

*/

#if KVED_FLASH_WORD_SIZE == 16
	typedef struct { uint64_t low; uint64_t high; } kved_word_t; /**< flash word data type */
#elif KVED_FLASH_WORD_SIZE == 8
	typedef uint64_t kved_word_t; /**< flash word data type */
#elif KVED_FLASH_WORD_SIZE == 4
	typedef uint32_t kved_word_t; /**< flash word data type */
#else
	#error Invalid flash word data type
#endif

#define KVED_HDR_SIZE_IN_WORDS    2 /**< kved header size */
#define KVED_ENTRY_SIZE_IN_WORDS  2 /**< kved entry size */

#if KVED_FLASH_WORD_SIZE == 16
	#define KVED_SIGNATURE_ENTRY     (kved_word_t) { .low = 0xDEADBEEFDEADBEEFULL, .high = 0xDEADBEEFDEADBEEFULL }
	#define KVED_DELETED_ENTRY       (kved_word_t) { .low = 0x0000000000000000ULL, .high = 0x0000000000000000ULL }
	#define KVED_FREE_ENTRY          (kved_word_t) { .low = 0xFFFFFFFFFFFFFFFFULL, .high = 0xFFFFFFFFFFFFFFFFULL }
	#define KVED_HDR_ENTRY_MSK       (kved_word_t) { .low = 0xFFFFFFFFFFFFFF00ULL, .high = 0xFFFFFFFFFFFFFFFFULL }
#elif KVED_FLASH_WORD_SIZE == 8
	#define KVED_SIGNATURE_ENTRY  0xDEADBEEFDEADBEEFULL
	#define KVED_DELETED_ENTRY    0x0000000000000000ULL
	#define KVED_FREE_ENTRY       0xFFFFFFFFFFFFFFFFULL
	#define KVED_HDR_ENTRY_MSK    0xFFFFFFFFFFFFFF00ULL
#elif KVED_FLASH_WORD_SIZE == 4
	#define KVED_SIGNATURE_ENTRY  0xDEADBEEFUL /**< kved signature */
	#define KVED_DELETED_ENTRY    0x00000000UL /**< deleted entry identification */
	#define KVED_FREE_ENTRY       0xFFFFFFFFUL /**< free entry identification */
	#define KVED_HDR_ENTRY_MSK    0xFFFFFF00UL /**< label entry mask */
#endif

#if KVED_FLASH_WORD_SIZE == 16
	#define KVED_HDR_MASK_KEY(k)      ((kved_word_t) { .low = (k).low & KVED_HDR_ENTRY_MSK.low, .high = (k).high & KVED_HDR_ENTRY_MSK.high }) /**< label entry mask */
	#define KVED_HDR_MASK_TYPE(k)     (((k).low & 0xF0) >> 4) /**< type entry mask */
	#define KVED_HDR_MASK_SIZE(k)     (((k).low & 0x0F)) /**< size entry mask */
	#define KVED_FLASH_UINT_MAX       ((kved_word_t){ .low = 0xFFFFFFFFFFFFFFFFULL, .high = 0xFFFFFFFFFFFFFFFFULL }) /**< last valid unsigned int value for current flash word */
	#define KVED_ENTRY_IS_EQUAL(a,b)  (((a).low == (b).low) && ((a).high == (b).high)) /**< check when entry is equal */
    #define KVED_ENTRY_FROM_VALUE(v)  ((kved_word_t){ .low = (v), .high = (v) })
#else
	#define KVED_HDR_MASK_KEY(k)      ( (k) & KVED_HDR_ENTRY_MSK) /**< label entry mask */
	#define KVED_HDR_MASK_TYPE(k)     (((k) & 0xF0) >> 4) /**< type entry mask */
	#define KVED_HDR_MASK_SIZE(k)     (((k) & 0x0F)) /**< size entry mask */
	#define KVED_FLASH_UINT_MAX       (~((kved_word_t)0)) /**< last valid unsigned int value for current flash word */
	#define KVED_ENTRY_IS_EQUAL(a,b)  ((a) == (b)) /**< check when entry is equal */
	#define KVED_ENTRY_FROM_VALUE(v)  ((kved_word_t)0)
#endif


#define KVED_NULL_ENTRY KVED_DELETED_ENTRY

/** Maximum supported string length, per record, without termination */
#define KVED_MAX_STRING_SIZE (KVED_FLASH_WORD_SIZE)
/** Key size for data access, with terminator */
#define KVED_MAX_KEY_SIZE    (KVED_FLASH_WORD_SIZE-1) 
/** Index return value when a key is not found in the database */
#define KVED_INDEX_NOT_FOUND 0 

/**
@brief Supported data types.
*/
typedef enum kved_data_types_e
{
	KVED_DATA_TYPE_UINT8 = 0, /**< 8 bits, unsigned */
	KVED_DATA_TYPE_INT8,      /**< 8 bits, signed */
	KVED_DATA_TYPE_UINT16,    /**< 16 bits, unsigned */
	KVED_DATA_TYPE_INT16,     /**< 16 bits, signed */
	KVED_DATA_TYPE_UINT32,    /**< 32 bits, unsigned */
	KVED_DATA_TYPE_INT32,     /**< 32 bits, com sinal */
	KVED_DATA_TYPE_FLOAT,     /**< Single precision floating point (float) */
	KVED_DATA_TYPE_STRING,    /**< String up to @ref KVED_MAX_STRING_SIZE bytes, excluding terminator */
#if KVED_FLASH_WORD_SIZE >= 8
	KVED_DATA_TYPE_UINT64,    /**< 64 bits, signed */
	KVED_DATA_TYPE_INT64,     /**< 64 bits, unsigned */
	KVED_DATA_TYPE_DOUBLE,    /**< Double precision floating point (double) */
#endif	
} kved_data_types_t;

/**
@brief Union with supported data types
*/
typedef union kved_value_u
{
	uint8_t u8; /**< unsigned 8 bits value */
	int8_t i8; /**< signed 8 bits value */
	uint16_t u16; /**< unsigned 16 bits value */
	int16_t i16; /**< signed 16 bits value */
	uint32_t u32; /**< unsigned 32 bits value */
	int32_t i32; /**< signed 32 bits value */
	float flt; /**< single precision float */
	uint8_t str[KVED_MAX_STRING_SIZE]; /**< string */
#if KVED_FLASH_WORD_SIZE >= 8
	uint64_t u64; /**< unsigned 64 bits value */
	int64_t i64; /**< signed 64 bits value */
	double dbl; /**< double precision float */
#endif		
} kved_value_t; 

/**
@brief Structure for accessing the database containing information about key, type and value
*/
typedef struct kved_data_s
{
	kved_value_t value;             /**< User value */                  
	uint8_t key[KVED_MAX_KEY_SIZE]; /**< String used as access key */
	kved_data_types_t type;         /**< Data type used according to @ref kved_data_types_t */
} kved_data_t;

/**
@brief Writes a new value to the database.
@param[in] data - information about the data to be written
@return true: recording successful.
@return false: error during the recording process.

@code

kved_data_t kv1 = {
	.type = KVED_DATA_TYPE_UINT32,
	.key = "ca1",
	.value.u32 = 0x12345678
};

kved_data_t kv2 = {
	.type = KVED_DATA_TYPE_STRING,
	.key = "ID",
	.value.str ="N01"
};

kved_data_write(&kv1);
kved_data_write(&kv2);

@endcode
*/
bool kved_data_write(kved_data_t *data);

/**
@brief Retrieves a previously saved value from database.
@param[out] data - Structure where the retrieved value will be stored (type and content)
@return true: read successfully.
@return false: error during the reading process.

@code

kved_data_t kv1 = {
	.key = "ca1",
};

kved_data_t kv2 = {
	.key = "ID",
};

if(kved_data_read(&kv1))
	printf("Value: %d\n",kv1.value.u32);

if(kved_data_read(&kv2))
	printf("Value: %4s\n",kv2.value.str);

@endcode
*/
bool kved_data_read(kved_data_t *data);

/**
@brief Deletes a previously saved value in the database, if it exists.
@param[in] data - Structure where the retrieved value will be stored (type and content)
@return true: deletion successful.
@return false: error during the erasuring process.

@code

kved_data_t kv1 = {
	.key = "ca1",
};

if(kved_data_delete(&kv1))
	printf("Entry erased");

@endcode
*/
bool kved_data_delete(kved_data_t *data);

/**
@brief Retrieves a previously saved value from database but using an index.
This function is used in conjunction with @ref kved_first_used_index_get and @ref kved_next_used_index_get
to iterate over the database.
@param[in] index - index of the value to retrieve
@param[out] data - structure where the retrieved value will be stored (type and content)
@return true: read successfully.
@return false: error during reading process.

@code

kved_data_t kv1;

uint16_t index = kved_first_used_index_get();

printf("Database keys:\n");

while(index != KVED_INDEX_NOT_FOUND)
{
	kved_data_read_by_index(index,&data);
	printf("- Key: %s\n",kv1.key);
	index = kved_next_used_index_get(index);
}

@endcode
*/
bool kved_data_read_by_index(uint16_t index, kved_data_t *data);

/**
@brief Get the first valid index in the database.
@return returns @ref KVED_INDEX_NOT_FOUND (index not found, database empty) or an index value greater than zero
*/
uint16_t kved_first_used_index_get(void);

/**
@brief Given the last index, get the next valid index from the database.
@param[in] last_index - last valid index used
@return return @ref KVED_INDEX_NOT_FOUND when the end of the database is reached or a valid index value (greater than zero)
*/
uint16_t kved_next_used_index_get(uint16_t last_index);

/**
@brief Returns the number of database entries (used or not)
@return Number of entries
*/
uint16_t kved_total_entries_get(void);

/**
@brief Returns the number of used database entries
@return Number of entries
*/
uint16_t kved_used_entries_get(void);

/**
@brief Returns the number available entries in the database
@return Number of entries
*/
uint16_t kved_free_entries_get(void);

/**
@brief Print all values stored in the database
*/
void kved_dump(void);

/**
@brief Format the database, deleting all values
*/
void kved_format(void);

/**
@brief Given a key entry from database, decode it to data type and user key
@param[out] data - structure where data type and user key will be stored
@param[in] key - key entry
*/
void kved_key_decode(kved_data_t *data, kved_word_t key);

/**
@brief Given a data type and user key, encode it as a key entry to be written in the database
@param[in] data - user entry
@return Encoded key entry
*/
kved_word_t kved_key_encode(kved_data_t *data);

/**
@brief Initialize the database. Must be called before any use.
*/
void kved_init(void);

/**
@}
*/
