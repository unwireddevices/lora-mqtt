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
 * @file	umdk-adc.c
 * @brief   umdk-adc message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_adc_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
    else if (strstr(param, "set_gpio ") == param) {
        param += strlen("set_gpio "); // Skip command

        uint8_t gpio = atoi(param);

        snprintf(out, bufsize, "02%02x", gpio);
    }
    else if (strstr(param, "set_lines ") == param) {
        param += strlen("set_lines ");    // Skip command

        uint8_t lines_en = 0;
        uint8_t line = 0;
        while ( (line = (uint8_t)strtol(param, &param, 10)) ) {
            if (line > 0 && line <= 7) {
                lines_en |= 1 << (line - 1);
            }
        }

        snprintf(out, bufsize, "03%02x", lines_en);
    }
}

bool umdk_adc_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
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
    for (i = 0; i < 16; i += 2) {
        uint16_t sensor = moddata[i] | (moddata[i+1] << 8);
        /* uint16_to_le(&sensor); */
        
        char ch[6] = {};
        snprintf(ch, sizeof(ch), "adc%d", (i / 2) + 1);

        if (sensor == 0xFFFF) {
            add_value_pair(mqtt_msg, ch, "null");
        }
        else {
            snprintf(buf, sizeof(buf), "%d", sensor);
            add_value_pair(mqtt_msg, ch, buf);
        }
    }
    return true;
}
