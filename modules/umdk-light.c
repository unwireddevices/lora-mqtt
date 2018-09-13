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
 * @file	umdk-light.c
 * @brief   umdk-light message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_LIGHT_DATA = 0,
	UMDK_LIGHT_CMD_COMMAND = 1,
	UMDK_LIGHT_CMD_POLL = 2,
    UMDK_LIGHT_CMD_FAIL = 0xFF,
} umdk_light_cmd_t;

void umdk_light_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
    else if (strstr(param, "set_i2c ") == param) { 
         param += strlen("set_i2c ");	// Skip command

         uint8_t i2c = atoi(param);

         snprintf(out, bufsize, "02%02x", i2c);
    }
}

bool umdk_light_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    
    if (moddata[0] == UMDK_LIGHT_DATA) {
        uint16_t lum;
        memcpy((void *)&lum, &moddata[1], 2);
        convert_from_be_sam((void *)&lum, sizeof(lum));
        
        snprintf(buf, sizeof(buf), "%d", lum);
        add_value_pair(mqtt_msg, "luminocity", buf);
    }

    if (moddata[0] == UMDK_LIGHT_CMD_COMMAND) {
        add_value_pair(mqtt_msg, "msg", "ok");
    }
    
    if (moddata[0] == UMDK_LIGHT_CMD_FAIL) {
        add_value_pair(mqtt_msg, "msg", "error");
    }

	return true;
}
