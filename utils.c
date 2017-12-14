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
    if (is_big_endian()) {
        uint64_swap_bytes(num);
    }
}

void uint32_to_le(uint32_t *num)
{
    if (is_big_endian()) {
        uint32_swap_bytes(num);
    }
}

void uint16_to_le(uint16_t *num)
{
    if (is_big_endian()) {
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

/* GPS binary format decoder */
/* format is used by umdk-gps, umdk-idcard and other GPS-enabled devices */
void parse_gps_data(gps_data_t *gps, uint8_t *data, bool decode_nmea) {
    memset(gps, 0, sizeof(gps_data_t));

    gps->ready = (data[0] & 1);

    if (gps->ready) {
        int lat, lat_d, lon, lon_d;
        lat = data[1] + (data[2] << 8);
        lat_d = data[3];
        lon = data[4] + (data[5] << 8);
        lon_d = data[6];

        gps->latitude = (float)lat + (float)lat_d/100.0;
        gps->longitude = (float)lon + (float)lon_d/100.0;

        /* Apply sign bits from reply */
        if ((data[0] >> 5) & 1) {
            gps->latitude = -gps->latitude;
        }

        if ((data[0] >> 6) & 1) {
            gps->longitude = -gps->longitude;
        }

        gps->valid = (data[0] >> 7) & 1;
    }

    return;
}
