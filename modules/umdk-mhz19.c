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
 * @file	umdk-mhz19.c
 * @brief   umdk-mhz19 message parser
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_MHZ19_ASK = 0,
    UMDK_MHZ19_SET_PERIOD = 1,
} umdk_mhz19_prefix_t;

typedef enum {
	UMDK_MHZ19_REPLY_OK = 0,
	UMDK_MHZ19_REPLY_RECEIVED = 1,

	UMDK_MHZ19_REPLY_ERR_OVF = 253,	/* RX buffer overflowed */
	UMDK_MHZ19_REPLY_ERR_FMT = 254,
	UMDK_MHZ19_ERR = 255,
} umdk_mhz19_reply_t;

void umdk_mhz19_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period "); // skip command

        uint8_t period = strtol(param, &param, 10);

        printf("[mqtt-mhz19] Set period to %d minutes\n", (int)period);

        snprintf(out, bufsize, "%02x%02x", UMDK_MHZ19_SET_PERIOD, period);
    }
    else if (strstr(param, "get ") == param) {
        param += strlen("get "); // skip command

        snprintf(out, bufsize, "%02x", UMDK_MHZ19_ASK);
    }
}

bool umdk_mhz19_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];
    char buf[20];
    
    if (moddatalen == 1) {
        switch (reply_type) {
            case UMDK_MHZ19_REPLY_OK: {
                snprintf(buf, 5, "%d", UMDK_MHZ19_REPLY_OK);
                add_value_pair(mqtt_msg, "type", buf);
                add_value_pair(mqtt_msg, "msg", "ok");
                break;
            }
            case UMDK_MHZ19_REPLY_ERR_OVF: {
                snprintf(buf, 5, "%d", UMDK_MHZ19_REPLY_ERR_OVF);
                add_value_pair(mqtt_msg, "type", buf);
                add_value_pair(mqtt_msg, "msg", "overflow error");
                break;
            }
            case UMDK_MHZ19_REPLY_ERR_FMT: {
                snprintf(buf, 5, "%d", UMDK_MHZ19_REPLY_ERR_FMT);
                add_value_pair(mqtt_msg, "type", buf);
                add_value_pair(mqtt_msg, "msg", "format error");
                break;
            }
            case UMDK_MHZ19_ERR: {
                snprintf(buf, 5, "%d", UMDK_MHZ19_ERR);
                add_value_pair(mqtt_msg, "type", buf);
                add_value_pair(mqtt_msg, "msg", "error");
                break;
            }
        }
    } else {
        int16_t co2 = moddata[0] | (moddata[1] << 8);
        /* uint16_to_le((uint16_t *)&co2); */

        int16_t temp = moddata[2] | (moddata[3] << 8);
        /* uint16_to_le((uint16_t *)&temp); */
        
        uint8_t is_data_valid = moddata[4];
        
        snprintf(buf, 5, "%d", co2);
        add_value_pair(mqtt_msg, "co2", buf);
        
        int_to_float_str(buf, temp, 1);
        add_value_pair(mqtt_msg, "temp", buf);
        
        snprintf(buf, 4, "%d", is_data_valid);
        add_value_pair(mqtt_msg, "valid", buf);
    }
    return true;
}
