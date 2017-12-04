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
 * @file	umdk-irblaster.c
 * @brief   umdk-irblaster message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    UMDK_IR_CMD_SEND = 0,
} umdk_irblaster_cmd_t;

#include "unwds-modules.h"
#include "utils.h"

void umdk_irblaster_command(char *param, char *out, int bufsize) {
    if (strstr(param, "send ") == param) {
        uint8_t bytes[50] = {};
        char *hex = param + strlen("send "); // Skip command
        
        if (strlen(hex) > 100) {
            puts("umdk-irblaster: hex string too long");
        }

        if (!hex_to_bytes(hex, bytes, true)) {
            puts("umdk-irblaster: invalid hex format");
            return;
        }

        snprintf(out, bufsize, "%02x%02x%s", UMDK_IR_CMD_SEND, strlen(hex)/2, hex);
    } else {
        return;
    }
}

bool umdk_irblaster_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    if (moddatalen == 1) {
        if (moddata[0] == 0) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else {
            add_value_pair(mqtt_msg, "msg", "error");
        }
        return true;
    }

    return false;
}