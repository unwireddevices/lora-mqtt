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
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", bytes[(reverse_order) ? num_bytes - 1 - i : i]);
        strcat(str, buf);
    }
}

bool is_big_endian(void)
{
    int n = 1;
    // big endian if true
    return (*(char *)&n == 0);
}

void uint64_swap_bytes(uint64_t *num)
{
    *num =  ((*num & (uint64_t)0x00000000000000ff) << 56) | \
            ((*num & (uint64_t)0x000000000000ff00) << 40) | \
            ((*num & (uint64_t)0x0000000000ff0000) << 24) | \
            ((*num & (uint64_t)0x00000000ff000000) <<  8) | \
            ((*num & (uint64_t)0x000000ff00000000) >>  8) | \
            ((*num & (uint64_t)0x0000ff0000000000) >> 24) | \
            ((*num & (uint64_t)0x00ff000000000000) >> 40) | \
            ((*num & (uint64_t)0xff00000000000000) >> 56);
}

void uint32_swap_bytes(uint32_t *num) {
    *num =  ((*num & 0x000000ff) << 24) | \
            ((*num & 0x0000ff00) <<  8) | \
            ((*num & 0x00ff0000) >>  8) | \
            ((*num & 0xff000000) >> 24);
}

void uint16_swap_bytes(uint16_t *num) {
    *num =  ((*num & 0x00ff) << 8) | \
            ((*num & 0xff00) >> 8);
}

void uint64_to_le(uint64_t *num)
{
    if (!is_big_endian()) {
        uint64_swap_bytes(num);
    }
}

void uint32_to_le(uint32_t *num)
{
    if (!is_big_endian()) {
        uint32_swap_bytes(num);
    }
}

void uint16_to_le(uint16_t *num)
{
    if (!is_big_endian()) {
        uint16_swap_bytes(num);
    }
}

void logprint(char *str)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%02d:%02d:%02d]%s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, str);
    syslog(LOG_INFO, "%s", str);
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

uint16_t crc16_arc(uint8_t *data, uint16_t len)
{
   uint16_t crc = 0x0000;
   uint16_t j;
   for (j = len; j > 0; j--)
   {
      crc ^= *data++;
      uint8_t i;
      for (i = 0; i < 8; i++)
      {
         if (crc & 1)
            crc = (crc >> 1) ^ 0xA001; // 0xA001 is the reflection of 0x8005
         else
            crc >>= 1;
      }
   }
   return (crc);
}

void convert_from_be_sam(void *ptr, size_t size) {    
    switch (size) {
        case 1: {
            int8_t v = *(int8_t*)ptr;
            *(uint8_t*)ptr = (~(v >> 7) & v) | (((v & 0x80) - v) & (v >> 7));
            break;
        }
        case 2: {
            uint16_to_le((uint16_t *)ptr);
            
            int16_t v = *(int16_t*)ptr;
            *(uint16_t*)ptr = (~(v >> 15) & v) | (((v & 0x8000) - v) & (v >> 15));
            break;
        }
        case 4: {
            uint32_to_le((uint32_t *)ptr);
            
            int32_t v = *(int32_t*)ptr;
            *(uint32_t*)ptr = (~(v >> 31) & v) | (((v & 0x80000000) - v) & (v >> 31));
            break;
        }
        case 8: {
            uint64_to_le((uint64_t *)ptr);
            
            int64_t v = *(int64_t*)ptr;
            *(uint64_t*)ptr = (~(v >> 63) & v) | (((v & (1ULL << 63)) - v) & (v >> 63));
            break;
            break;
        }
        default:
            return;
    }
}

void convert_to_be_sam(void *ptr, size_t size) {
    switch (size) {
        case 1: {
            int8_t v = *(int8_t*)ptr;
            *(uint8_t*)ptr = ((v + (v >> 7)) ^ (v >> 7)) | (v & (1 << 7));
            break;
        }
        case 2: {
            int16_t v = *(int16_t*)ptr;
            *(uint16_t*)ptr = ((v + (v >> 15)) ^ (v >> 15)) | (v & (1 << 15));
            uint16_to_le((uint16_t *)ptr);
            break;
        }
        case 4: {
            int32_t v = *(int32_t*)ptr;
            *(uint32_t*)ptr = ((v + (v >> 31)) ^ (v >> 31)) | (v & (1 << 31));
            uint32_to_le((uint32_t *)ptr);
            break;
        }
        case 8: {
            int64_t v = *(int64_t*)ptr;
            *(uint64_t*)ptr = ((v + (v >> 63)) ^ (v >> 63)) | (v & (1ULL << 63));
            uint64_to_le((uint64_t *)ptr);
            break;
        }
        default:
            return;
    }
}