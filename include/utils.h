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
 * @author      cr0s
 */
#ifndef LORA_STAR_UNI_UTILS_H_
#define LORA_STAR_UNI_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

bool hex_to_bytes(char *hexstr, uint8_t *bytes, bool reverse_order);

bool hex_to_bytesn(char *hexstr, int len, uint8_t *bytes, bool reverse_order);

void bytes_to_hex(uint8_t *bytes, size_t num_bytes, char *str, bool reverse_order);

bool is_big_endian(void);

void uint16_to_le(uint16_t *num);
void uint32_to_le(uint32_t *num);
void uint64_to_le(uint64_t *num);

void uint16_swap_bytes(uint16_t *num);
void uint32_swap_bytes(uint32_t *num);
void uint64_swap_bytes(uint64_t *num);

void logprint(char *str);

void int_to_float_str(char *buf, int decimal, uint8_t precision);

bool is_number(char* str);

typedef struct {
	float latitude;
	float longitude;
    /*
	char date[15];
	char time[15];
    */
    bool valid;
    bool ready;
} gps_data_t;

void parse_gps_data(gps_data_t *gps, uint8_t *data, bool decode_nmea);

uint16_t crc16_arc(uint8_t *data, uint16_t len);

#endif
