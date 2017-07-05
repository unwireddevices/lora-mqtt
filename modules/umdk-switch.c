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
 * @file	umdk-switch.c
 * @brief   umdk-switch message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_SWITCH_REPLY_OK,
    UMDK_SWITCH_REPLY_UNKNOWN_COMMAND,
    UMDK_SWITCH_REPLY_INV_PARAMETER,
    UMDK_SWITCH_REPLY_DATA,
    UMDK_SWITCH_STATUS_DATA,
} umdk_switch_reply_t;

typedef enum {
    UMDK_SWITCH_CMD_SET_PERIOD,
    UMDK_SWITCH_CMD_RESET,
    UMDK_SWITCH_CMD_POLL,
} umdk_switch_cmd_t;

#define UMDK_SWITCH_NUM_SENS  4

void umdk_switch_command(char *param, char *out, int bufsize) {
    if (strstr(param, "period ") == param) {
        param += strlen("period "); // skip command
        uint8_t period = strtol(param, &param, 10);
        snprintf(out, bufsize, "%02x%02x", UMDK_SWITCH_CMD_SET_PERIOD, period);
    }
    else if (strstr(param, "reset ") == param) {
        snprintf(out, bufsize, "%02x", UMDK_SWITCH_CMD_RESET);
    }
    else if (strstr(param, "poll ") == param) {
        snprintf(out, bufsize, "%02x", UMDK_SWITCH_CMD_POLL);
    }
}

bool umdk_switch_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[50] = {};
    
    uint8_t reply_type = moddata[0];
    switch (reply_type) {
        case UMDK_SWITCH_REPLY_DATA: { /*  */
            uint8_t sw = moddata[1];
            
            /* switch number  */
            snprintf(buf, sizeof(buf), "switch %d", (sw & 0x7F));
            uint8_t val = (sw >> 7);
            
            if (val) {
                add_value_pair(mqtt_msg, buf, "1");
            } else {
                add_value_pair(mqtt_msg, buf, "0");
            }
            
            break;
        }
        case UMDK_SWITCH_STATUS_DATA: { /*  */
            uint8_t sw = moddata[1];
            
            int i = 0;
            
            char strtmp[10];
            snprintf(buf, 3, "[ ");
            for (i = 0; i < UMDK_SWITCH_NUM_SENS; i++) {
                snprintf(strtmp, 10, "%" PRIu8, (sw >> i) & 1);
                strcat(buf, strtmp);
                
                if (i < UMDK_SWITCH_NUM_SENS - 1 ) {
                    strcat(buf, ",");
                }
                strcat(buf, " ");
            }
            strcat(buf, "]");
        
            add_value_pair(mqtt_msg, "status", buf);
            break;
        }
        case UMDK_SWITCH_REPLY_OK: { /*  */
            add_value_pair(mqtt_msg, "msg", "set ok");
            break;
        }
        case UMDK_SWITCH_REPLY_UNKNOWN_COMMAND: { /*  */
            add_value_pair(mqtt_msg, "msg", "invalid command");
            break;
        }
        case UMDK_SWITCH_REPLY_INV_PARAMETER: { /*  */
            add_value_pair(mqtt_msg, "msg", "invalid parameter");
            break;
        }
        default:
            puts("ERROR DECODING COMMAND");
            break;
    }
    return true;
}
