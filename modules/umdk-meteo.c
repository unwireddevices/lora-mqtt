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
 * @file	umdk-meteo.c
 * @brief   umdk-meteo message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_METEO_DATA = 0,
	UMDK_METEO_COMMAND = 1,
	UMDK_METEO_POLL = 2,
    UMDK_METEO_FAIL = 0xFF,
} umdk_meteo_cmd_t;

void umdk_meteo_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
}

bool umdk_meteo_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    
    if (moddata[0] == UMDK_METEO_DATA) {
        int16_t temp;
        memcpy((void *)&temp, &moddata[1], 2);
        convert_from_be_sam((void *)&temp, sizeof(temp));
    
        int16_t hum;
        memcpy((void *)&hum, &moddata[3], 2);
        convert_from_be_sam((void *)&hum, sizeof(hum));

        uint16_t press;
        memcpy((void *)&press, &moddata[5], 2);
        convert_from_be_sam((void *)&press, sizeof(press));
    
        int_to_float_str(buf, temp, 1);
        add_value_pair(mqtt_msg, "temperature", buf);
        
        int_to_float_str(buf, hum, 1);
        add_value_pair(mqtt_msg, "humidity", buf);
        
        snprintf(buf, sizeof(buf), "%d", press);
        add_value_pair(mqtt_msg, "pressure", buf);
    }
    
    if (moddata[0] == UMDK_METEO_COMMAND) {
        add_value_pair(mqtt_msg, "msg", "ok");
    }

    if (moddata[0] == UMDK_METEO_FAIL) {
        add_value_pair(mqtt_msg, "msg", "error");
    }
    
    return true;
}
