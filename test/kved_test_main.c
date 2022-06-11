/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "kved.h"
#include "kved_test.h"

int main(void)
{
	kved_init();
	printf("------------ value test ------------\r\n");
	kved_value_test();
	printf("------------ header test ------------\r\n");
	kved_header_test();
	printf("------------ key test ------------\r\n");
	kved_key_test();

	return 0;
}
