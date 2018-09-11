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
#include <math.h>

typedef enum {
	UMDK_IDCARD_CMD_SET_PERIOD = 0,
	UMDK_IDCARD_CMD_COLLECT,
	UMDK_IDCARD_CMD_AUTH,
    UMDK_IDCARD_CMD_ALARM,
    UMDK_IDCARD_CMD_LOCATION,
    UMDK_IDCARD_CMD_SWITCH_GPS,
} umdk_idcard_cmd_t;

typedef enum {
    UMDK_IDCARD_REPLY_FINGERPRINT_SET = 0,
    UMDK_IDCARD_REPLY_PACKET,
    UMDK_IDCARD_REPLY_ACK_OK,
    UMDK_IDCARD_REPLY_ACK_ERR,
} umdk_reply_action_t;

typedef enum {
    UMDK_IDCARD_PACKET_ID = 0,
    UMDK_IDCARD_BIOMETRY = 1,
    UMDK_IDCARD_GPS = 2,
    UMDK_IDCARD_RESERVED = 9,
    UMDK_IDCARD_ALARM = 10,
    UMDK_IDCARD_MESSAGE_ID = 11,
    UMDK_IDCARD_MESSAGE_ID2 = 12,
    UMDK_IDCARD_MESSAGE_LENGTH = 13,
} umdk_idcard_packet_fields_t;

#include "unwds-modules.h"
#include "utils.h"

void umdk_idcard_command(char *param, char *out, int bufsize) {
    
}

bool umdk_idcard_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    return true;
}
