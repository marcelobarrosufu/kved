/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Fabricio Lucas de Almeida <fabriciolucasfbr@gmail.com>
*/

#include <stdint.h>
#include "kved_cpu.h"
#include "main.h"

void kved_cpu_critical_section_enter(void)
{
    __disable_irq();
}

void kved_cpu_critical_section_leave(void)
{
    __enable_irq();    
}
