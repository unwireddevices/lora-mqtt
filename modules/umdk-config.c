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
 * @file	umdk-config.c
 * @brief   umdk-config message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
	UMDK_CONFIG_REPLY_OK = 0,
	UMDK_CONFIG_REPLY_ERR = 253,
} umdk_config_reply_t;

typedef enum {
	UMDK_CONFIG_MODULES = 0,
} umdk_config_action_t;

void umdk_config_command(char *param, char *out, int bufsize) {
    if (strstr(param, "mod ") == param) {
        param += strlen("mod "); // skip command

        uint8_t id = strtol(param, &param, 10);
        uint8_t onoff = strtol(param, NULL, 10);

        snprintf(out, bufsize, "%02x%02x%02x", UMDK_CONFIG_MODULES, id, onoff);
    }
}

bool umdk_config_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];
    
    switch (reply_type) {
        case UMDK_CONFIG_REPLY_OK: {
            add_value_pair(mqtt_msg, "msg", "ok");
            break;
        }
        case UMDK_CONFIG_REPLY_ERR: {
            add_value_pair(mqtt_msg, "msg", "error");
            break;
        }
    }
    return true;
}
