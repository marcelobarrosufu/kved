/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

/**
@file
@defgroup KVED KVED
@brief KVED (key/value embedded databse): simple key/value persistence for embedded applications.
@{
*/

#pragma once

/**
@brief Flash word size.
Set your flash word size (4 or 8 bytes).
This can be done when compiling using -DKVED_FLASH_WORD_SIZE=8, for instance.
*/
#ifndef KVED_FLASH_WORD_SIZE
// #define KVED_FLASH_WORD_SIZE (8)
#define KVED_FLASH_WORD_SIZE (4)
#endif

//#define KVED_DEBUG

#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#ifndef __weak
#define __weak  __attribute__((weak))
#endif
#elif defined ( __GNUC__ ) && !defined (__CC_ARM)
#ifndef __weak
#define __weak   __attribute__((weak))
#endif
#endif

/**
@}
*/
