/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

/**
@file
@defgroup KVED_CPU KVED_CPU
@brief kved CPU API
@{
*/

#pragma once

/**
 @brief Critical section entry point
 */
void kved_cpu_critical_section_enter(void);

/**
 @brief Critical section exit
 */
void kved_cpu_critical_section_leave(void);

/**
@}
*/
