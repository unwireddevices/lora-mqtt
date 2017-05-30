/* Copyright (c) 2017 Unwired Devices LLC [info@unwds.com]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
#include <time.h>
#include <sys/time.h>

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
		snprintf(buf, sizeof(buf), "%02x", bytes[(reverse_order) ? num_bytes - 1 - i : i]);
		strcat(str, buf);
	}
}

bool is_big_endian(void)
{
    int n = 1;
    // big endian if true
    return (*(char *)&n == 0);
}

void uint32_to_le(uint32_t *num)
{
    if (is_big_endian()) {
        *num = ((*num >> 24) & 0xff) | ((*num << 8) & 0xff0000) | ((*num >> 8) & 0xff00) | ((*num << 24) & 0xff000000);
    }
}

void uint16_to_le(uint16_t *num)
{
    if (is_big_endian()) {
        *num = ((*num >> 8) & 0xff) | ((*num << 8) & 0xff00);
    }
}

void logprint(char *str)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	printf("[%02d:%02d:%02d]%s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, str);
	syslog(LOG_INFO, str);
}

void int_to_float_str(char *buf, int decimal, uint8_t precision) {  
    int i = 0;
    int divider = 1;
    char format[10] = { };
    char digits[3];
    
    if (decimal < 0) {
        strcat(format, "-");
    }
    strcat(format, "%d.%0");
    
    for (i = 0; i<precision; i++) {
        divider *= 10;
    }

    snprintf(digits, 3, "%dd", i);
    strcat(format, digits);
    
    snprintf(buf, 50, format, abs(decimal/divider), abs(decimal%divider));
}

bool is_number(char* str) {
    char *endptr = NULL;
    strtol(str, &endptr, 0);
    
    if ( &str[strlen(str)] == endptr  ) {
        return true;
    } else {
        return false;
    }
}