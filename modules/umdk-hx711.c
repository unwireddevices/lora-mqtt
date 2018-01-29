/* Copyright (c) 2018 Unwired Devices LLC [info@unwds.com]
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
 * @file	umdk-hx711.c
 * @brief   umdk-hx711 message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
	UMDK_HX711_CMD_POLL = 0,
    UMDK_HX711_CMD_PERIOD,
    UMDK_HX711_CMD_ZERO,
    UMDK_HX711_CMD_SCALE,
} umdk_hx711_cmd_t;

typedef enum {
	UMDK_HX711_DATA_DATA = 0,
    UMDK_HX711_DATA_OK,
    UMDK_HX711_DATA_ERROR,
} umdk_hx711_data_t;

void umdk_hx711_command(char *param, char *out, int bufsize) {
    if (strstr(param, "period ") == param) {
        param += strlen("period ");    // Skip command
        uint8_t period = atoi(param);
        snprintf(out, bufsize, "%02x%02x", UMDK_HX711_CMD_PERIOD, period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "%02x", UMDK_HX711_CMD_POLL);
    }
    else if (strstr(param, "zero ") == param) {
    	 snprintf(out, bufsize, "%02x", UMDK_HX711_CMD_ZERO);
    }
    else if (strstr(param, "scale ") == param) {
        param += strlen("scale ");    // Skip command
        uint32_t weight = atoi(param);
        snprintf(out, bufsize, "%02x%08x", UMDK_HX711_CMD_SCALE, weight);
    }
}

bool umdk_hx711_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    
    switch (moddata[0]) {
        case UMDK_HX711_DATA_OK:
            add_value_pair(mqtt_msg, "msg", "ok");
            break;
        case UMDK_HX711_DATA_ERROR:
            add_value_pair(mqtt_msg, "msg", "error");
            break;
        case UMDK_HX711_DATA_DATA: {
            uint32_t weight = moddata[1] | (moddata[2] << 8) | (moddata[3] << 16) | (moddata[4] << 24);
            snprintf(buf, sizeof(buf), "%" PRIu32, weight);
            
            add_value_pair(mqtt_msg, "weight", buf);
            break;
        }
        default:
            return false;
    }

    return true;
}
