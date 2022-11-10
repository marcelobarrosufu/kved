/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Fabricio Lucas de Almeida <fabriciolucasfbr@gmail.com>
*/

#pragma once

/**
@brief Flash word size.
Max supported is 16 bytes, but used 4 bytes only.
*/
#define PORT_KVED_FLASH_WORD_SIZE (4)
