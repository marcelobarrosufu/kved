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

#include "kved.h"
#include "kved_cpu.h"

__weak void kved_cpu_critical_section_enter(void)
{
}

__weak void kved_cpu_critical_section_leave(void)
{   
}
