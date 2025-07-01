# KVED 

kved (key/value embedded database) is a simple key/value database implementation for microcontrollers.

## kved features

* It is a copy-on-write (COW) implementation: the value is only written when it changes.
* Wear leveling: new values or changed values are always written into different positions, cycling over the flash sector. 
* Two sectors (with same size) are used to allow sector clean up when the current sector has space but it is full due to erased entries.
* Power loss tolerant: incomplete writings due to power outages does not corrupts the database. However, the newer value can be lost.
* Integrity checks at startup, erasing incomplete writings (old values are not lost) and checking which sector is in use.
* Flash with word size of 32 or 64 bits are supported.
* Iteration over the database supported.

## Limitations

* You need to use two flash sectors, with same size. 
* Your flash needs to support word granularity writings.
* After writing into a flash position (a word) should be possible to write again, lowering bit that were high. This is the mechanism to invalided a register.
* Multiple sectors support not implemented.
* As with many wear leveling systems, it is not desirable to use the system near maximum storage capacity. An effective use of 50% or less of the entries is recommended. 

## How kved works

kved is based on the principle that each writing (of a word) is atomic and also on the idea that a sector can be erased individually.

Each entry in the database is encoded by two words. The first encodes the access key, record type and recorde size and the second the value itself. The size of the input label depends on whether the word is 32-bit or 64-bit. Type and size are encoded in the first byte, in 4 bits each.

<pre>
32 bits flash:

 +------+--+
 | 3 2 1| 0|   <= bytes
 +------+--+
 | LABEL|TS|
 +------+--+

64 bits flash:

 +--------------+--+
 | 7 6 5 4 3 2 1| 0|   <= bytes
 +--------------+--+
 |         LABEL|TS|
 +--------------+--+
</pre>

Thus, the complete entry has the following format:

<pre>
 +---------+---------+
 | 1st WORD| 2nd WORD|
 +------+--+---------+
 | LABEL|TS|  VALUE  |
 +------+--+---------+
</pre>

Entries that are invalidated have the key value set as zero. This is used when a new key value is written. In this case, the new value is written in the first free position of the sector and the old one has its keyword zeroed.

The database has a header with two words, at the beggining of the sector. 
The first word is a kved signature, identified by 0xDEADBEEF, and the second is a counter. 

This counter is used to identify which sector is the most recent (with the highest counter value). Every time the sector is copied this counter is incremented.

Thus, in memory, the organization of the data will be as follows:

<pre>
<--  WORD  --><--  WORD -->  <= 8 bytes when using flash with word of 64 bits 
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
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  |
+---------+--+------------+
|FFF ... FFFF|FFF ... FFFF|  <= EMPTY ENTRY
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|
+------------+------------+
</pre>

The second sector is used when the first sector is full or with many invalid entries. In this case, the second sector is erased and it is populated with all valid entries only, increasing the header counter and freeing space for new entries. When the second sector is full, the first sector will be erased and used, following the same strategy.

At startup, some integrity checks are made. The first one is related to which sector should be used, being done by the ```kved_sector_consistency_check()``` function. Once the sector in use is decided, the data is also checked using the ```kved_data_consistency_check()``` function. These checks allow database consistency to be maintained even in the event of a power failure during writing or copying.

The amount of available entries is dependent on the sector size and the flash word size and can be given by the following expression:

<pre>
num_entries = sector_size/(word_size*2) - 1
</pre>

For a sector of 2048 bytes and a 32 bits flash, the number of entries is given by 255 entries (2048/(4*2) - 1).

## Missing features

* Data key integrity check after writing (verify valid data label, size and labels)

# Porting kved

kved basically depends on the following files:

* ```kved.c``` / ```kved.h```: kved implementation
* ```kved_cpu.c``` / ```kved_cpu.h```: cpu portability API if you need thead/interrupt safe operation
* ```kved_flash.c``` / ```kved_flash.h```: flash API

All these files should be portable and platform independent. However, you need to write the portability layer. So, create these files and code them according to your microcontroller:

###  ```port_cpu.c```

Create your implementation for thread/interrupt safe operation (optional):

  * ```void kved_cpu_critical_section_enter(void)```
  * ```void kved_cpu_critical_section_leave(void)```

###  ```port_flash.c```

You need to reserve two sectors of your microcontroller for kved usage and create your functions for erase sector, read and write words and intialize the flash. As the sector size depends on the microcontroller used, an additional function for reporting it is also required.

  * ```bool kved_flash_sector_erase(kved_flash_sector_t sec)```
  * ```void kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data)```
  * ```kved_word_t kved_flash_data_read(kved_flash_sector_t sec, uint16_t index)```
  * ```uint32_t kved_flash_sector_size(void)```
  * ```void kved_flash_init(void)```

###  ```port_flash.h```

Define flash word size. 

## Linker

Do not forget to reserve your flash sectors on your linker file otherwise your compiler can use them. For GNU linker (ld) see examples in STM32L433RC, STM32F411CE, STM32WB55RG and STM32F103C8 ports.

## Ports

A the momment, 5 ports are supported:

* Simul (simulation port, runs on PC, useful for debugging). Just type scons at repository root and run ```./kved```. GCC will be used as compiler.
* STM32L433RC using low level STM32 drivers.
* STM32F411CE (blackpill) using high level STM32 drivers.
* STM32WB55RG using low level STM32 drivers plus optional HSEM, using high level STM32 drivers.
* STM32F103C8 (bluepill) using low level STM32 drivers.

# Documentation

Please, see the [HTML Documentation](https://marcelobarrosalmeida.github.io/kved/).

# License

[MIT License](./LICENSE.md) - Marcelo Barros

# Original Author Contact

For enhancements and new ports, please clone the repository and submit a pull request.

For bugs, please fill a new issue.

Any other requests, please contact me by [email](marcelobarrosalmeida@gmail.com).

# Contributors

* [Fabrício Lucas](https://github.com/fbrlucas)
* [James Hunt](https://github.com/huntj88)
