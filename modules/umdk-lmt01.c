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
 * @file	umdk-lmt01.c
 * @brief   umdk-lmt01 message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_lmt01_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
    else if (strstr(param, "set_gpios ") == param) {
    /*	 param += strlen("set_gpios ");	// Skip command

         uint8_t gpio = 0;
         while ((gpio = strtol(param, param, 10))

         snprintf(out, bufsize, "02");*/
    }
}

bool umdk_lmt01_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

    if (moddatalen == 1) {
        if (moddata[0] == 0) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else {
            add_value_pair(mqtt_msg, "msg", "error");
        }
        return true;
    }

    int i;
    for (i = 0; i < 8; i += 2) {
        int16_t sensor = moddata[i] | (moddata[i+1] << 8);
        /* uint16_to_le((uint16_t *)&sensor); */
        
        char ch[3] = {};
        snprintf(ch, sizeof(ch), "s%d", (i / 2) + 1);

        if (sensor == 0x7FFF) {
            add_value_pair(mqtt_msg, ch, "null");
        }
        else {
            int_to_float_str(buf, sensor, 1);
            add_value_pair(mqtt_msg, ch, buf);
        }
    }
    return true;
}
