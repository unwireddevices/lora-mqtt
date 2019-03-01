/* Copyright (c) 2019 Unwired Devices LLC [info@unwds.com]
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
 * @file	umdk-st95.c
 * @brief   umdk-st95 message parser
 * @author  Mikhail Perkov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

#define UMDK_ST95_ERROR 0x01

void umdk_st95_command(char *param, char *out, int bufsize) 
{
    
}

bool umdk_st95_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[50];

    if (moddatalen == 1) {
        if (moddata[0] == UMDK_ST95_ERROR) {
            add_value_pair(mqtt_msg, "msg", "invalid uid");
        }
        else {
            add_value_pair(mqtt_msg, "msg", "error");
        }
        return true;
    }
    
    uint8_t num_char = 0;
    int i;
    for(i = 0; i < moddatalen; i++) {
        num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%02x", moddata[i]);
    }
       
    add_value_pair(mqtt_msg, "uid", buf);
    
    return true;
}