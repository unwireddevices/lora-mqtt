/*
 * Copyright (C) 2016 Unwired Devices [info@unwds.com]
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    
 * @ingroup     
 * @brief       
 * @{
 * @file
 * @brief       
 * @author      Evgeniy Ponomarev
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

#include "utils.h"

bool hex_to_bytes(char *hexstr, uint8_t *bytes, bool reverse_order) {
	return hex_to_bytesn(hexstr, strlen(hexstr), bytes, reverse_order);
}

bool hex_to_bytesn(char *hexstr, int len, uint8_t *bytes, bool reverse_order) {
	/* Length must be even */
	if (len % 2 != 0)
		return false;

	/* Move in string by two characters */
	char *ptr = &(*hexstr);
	int i = 0;
	if (reverse_order) {
		ptr += len - 2;

		for (; (len >> 1) - i; ptr -= 2) {
			unsigned int v = 0;
			sscanf(ptr, "%02x", &v);

			bytes[i++] = (uint8_t) v;
		}
	} else {
		for (; *ptr; ptr += 2) {
			unsigned int v = 0;
			sscanf(ptr, "%02x", &v);

			bytes[i++] = (uint8_t) v;
		}
	}

	return true;
}

void bytes_to_hex(uint8_t *bytes, size_t num_bytes, char *str, bool reverse_order) {
	size_t i;
	for (i = 0; i < num_bytes; i++) {
		char buf[2];
		sprintf(buf, "%02x", bytes[(reverse_order) ? num_bytes - 1 - i : i]);
		strcat(str, buf);
	}
}

bool is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1; 
}

void logprint(char *str)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	printf("[%02d:%02d:%02d]%s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, str);
	syslog(LOG_INFO, str);
}