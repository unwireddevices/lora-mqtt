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
 * @file	umdk-idcard.c
 * @brief   umdk-idcard message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	UMDK_IDCARD_CMD_SET_PERIOD = 0,
	UMDK_IDCARD_CMD_COLLECT,
    UMDK_IDCARD_CMD_AUTH,
} umdk_idcard_cmd_t;

typedef enum {
    UMDK_IDCARD_REPLY_FINGERPRINT_SET = 0,
    UMDK_IDCARD_REPLY_PACKET,
    UMDK_IDCARD_REPLY_ACK_OK,
    UMDK_IDCARD_REPLY_ACK_ERR,
} umdk_reply_action_t;

#include "unwds-modules.h"
#include "utils.h"

void umdk_idcard_command(char *param, char *out, int bufsize) {
    if (strstr(param, "fingerprint ") == param) {
        param += strlen("fingerprint ");    // Skip command
        if (strstr(param, "set ") == param) {
            param += strlen("set ");
            uint8_t cell = strtol(param, &param, 10);
            uint16_t cmdid = strtol(param, &param, 10);
            snprintf(out, bufsize, "%02x%02x%04x", UMDK_IDCARD_CMD_COLLECT, cell, cmdid);
        } 
    } else if (strstr(param, "get ") == param) {
        param += strlen("get ");    // Skip command
        if (strstr(param, "fingerprint ") == param) {
            param += strlen("fingerprint ");
            uint16_t cmdid = strtol(param, &param, 10);
            snprintf(out, bufsize, "%02x%04x", UMDK_IDCARD_CMD_AUTH, cmdid);
        }
    }
}

bool umdk_idcard_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    uint16_t message_id;

    switch (moddata[0]) {
        case UMDK_IDCARD_REPLY_ACK_OK:
            add_value_pair(mqtt_msg, "msg", "ok");
            break;
        case UMDK_IDCARD_REPLY_ACK_ERR:
            add_value_pair(mqtt_msg, "msg", "error");
            break;
        case UMDK_IDCARD_REPLY_FINGERPRINT_SET:
            add_value_pair(mqtt_msg, "fingerprint", "ok");
            break;
        case UMDK_IDCARD_REPLY_PACKET:
            if (moddata[1] == 0) {
                add_value_pair(mqtt_msg, "biometry", "passed");
            } else if (moddata[1] == 1) {
                add_value_pair(mqtt_msg, "biometry", "failed");
            } else if (moddata[1] == 2) {
                add_value_pair(mqtt_msg, "biometry", "skipped");
            }
            
            add_value_pair(mqtt_msg, "fingerprints", "1");
            add_value_pair(mqtt_msg, "gps", "absent");
            if (moddata[10]) {
                add_value_pair(mqtt_msg, "alarm", "1");
            } else {
                add_value_pair(mqtt_msg, "alarm", "0");
            }
            break;
    }
    
    message_id = moddata[11] | moddata[12] << 8;
    uint16_to_le(&message_id);
    sprintf(buf, "%" PRIu16, message_id);
    add_value_pair(mqtt_msg, "mid", buf);
    return true;
}
