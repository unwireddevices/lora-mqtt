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
 * @file	umdk-counter.c
 * @brief   umdk-counter message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_COUNTER_CMD_SET_PERIOD = 0,
    UMDK_COUNTER_CMD_POLL = 1,
    UMDK_COUNTER_CMD_RESET = 2,
} umdk_counter_cmd_t;

typedef enum {
    UMDK_COUNTER_REPLY_OK = 0,
    UMDK_COUNTER_REPLY_UNKNOWN_COMMAND = 1,
    UMDK_COUNTER_REPLY_INV_PARAMETER = 2,
} umdk_counter_reply_t;

void umdk_counter_command(char *param, char *out, int bufsize)
{
    if (strstr(param, "period ") == param) {
        param += strlen("period "); // skip command

        uint8_t period = strtol(param, &param, 10);
        printf("[mqtt-counter] Set period: %" PRIu8 " hrs\n", period);

        snprintf(out, bufsize, "%02x%02x", UMDK_COUNTER_CMD_SET_PERIOD, period);
    }
    
    if (strstr(param, "reset") == param) {
        snprintf(out, bufsize, "%02x", UMDK_COUNTER_CMD_RESET);
    }
    
    if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "%02x", UMDK_COUNTER_CMD_POLL);
    }
  
    return;
}


bool umdk_counter_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

    if (moddatalen == 1) {
        switch (moddata[0]) {
            case UMDK_COUNTER_REPLY_OK:
                add_value_pair(mqtt_msg, "msg", "ok");
                break;
            case UMDK_COUNTER_REPLY_UNKNOWN_COMMAND:
                add_value_pair(mqtt_msg, "msg", "invalid command");
                break;
            case UMDK_COUNTER_REPLY_INV_PARAMETER:
                add_value_pair(mqtt_msg, "msg", "invalid parameter");
                break;
        }
        return true;
    }

    /* Extract counter values */
    uint8_t i = 0;
    uint32_t values[4] = { 0 };
    char ch[5] = {};

    /* data encoded in 3 UINT32 numbers on Little Endian system */
    /* we need to swap bytes inside UINT32s if we are on BE system (e.g. MIPS CPU) */
    /* then we can just use functions reverse to those used to encode */
    uint32_t *num = (uint32_t *)&moddata[0];
    if (is_big_endian()) {
        for (i = 0; i < 3; i++ ) {
            *num = ((*num >> 24) & 0xff) | ((*num << 8) & 0xff0000) | ((*num >> 8) & 0xff00) | ((*num << 24) & 0xff000000);
            num++;
        }
    }

    /* let's unpack 12 bytes back into 4 values */
    num = (uint32_t *)&moddata[0];
    values[0] = num[0] >> 8;
    values[1] = ((num[0] & 0xFF) << 16) | (num[1] >> 16);
    values[2] = ((num[1] & 0xFFFF) << 8) | (num[2] >> 24);
    values[3] = num[2] & 0xFFFFFF;
    
    for (i = 0; i < 4; i++) {   
        snprintf(ch, sizeof(ch), "v%d", i);
        snprintf(buf, sizeof(buf), "%u", values[i]);
        add_value_pair(mqtt_msg, ch, buf);
    }
    
    return true;
}
