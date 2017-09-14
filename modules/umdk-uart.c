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
 * @file	umdk-uart.c
 * @brief   umdk-uart message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_uart_command(char *param, char *out, int bufsize) {
    if (strstr(param, "send ") == param) {
        uint8_t bytes[200] = {};
        char *hex = param + strlen("send "); // Skip command

        if (!hex_to_bytes(hex, bytes, true)) {
            return;
        }

        snprintf(out, bufsize, "00%s", hex);
    }
    else if (strstr(param, "set_baudrate ") == param) {
        param += strlen("set_baudrate "); // Skip commands

        uint8_t baudrate = atoi(param);
        printf("baudrate: %d\n", baudrate);
        if (baudrate > 10) {
            return;
        }

        snprintf(out, bufsize, "01%02x", baudrate);
    }
    else if (strstr(param, "set ") == param) {
        param += strlen("set "); // Skip commands

        if (strlen(param) > strlen("115200-8N1")) {
            return;
        }

        snprintf(out, bufsize, "02");
        
        /* convert string to hex */
        uint8_t k;
        char tmpbuf[5];
        for (k = 0; k < strlen(param); k++) {
            snprintf(tmpbuf, 5, "%02x", param[k]);
            strcat(out, tmpbuf);
            //snprintf(out + 4 + 2*k, 3, "%02x", param[k]);
        }
        
        printf("UART mode: %s\n", param);
    }
    else {
        return;
    }
}

bool umdk_uart_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];

    switch (reply_type) {
        case 0: /* UMDK_UART_REPLY_SENT */
            add_value_pair(mqtt_msg, "type", "0");
            add_value_pair(mqtt_msg, "msg", "sent ok");
            return true;

        case 1: { /* UMDK_UART_REPLY_RECEIVED */
            char hexbuf[255] = { 0 };
            char hex[3] = { 0 };
            int k;
            for (k = 0; k < moddatalen - 1; k++) {
                snprintf(hex, 3, "%02x", moddata[k+1]);
                strcat(hexbuf, hex);
            }
            add_value_pair(mqtt_msg, "type", "1");
            add_value_pair(mqtt_msg, "msg", hexbuf);
            return true;
        }

        case 2:
            add_value_pair(mqtt_msg, "type", "2");
            add_value_pair(mqtt_msg, "msg", "baud rate set");
            return true;

        case 253: /* UMDK_UART_REPLY_ERR_OVF */
            add_value_pair(mqtt_msg, "type", "253");
            add_value_pair(mqtt_msg, "msg", "rx buffer overrun");
            return true;

        case 254: /* UMDK_UART_REPLY_ERR_FMT */
            add_value_pair(mqtt_msg, "type", "254");
            add_value_pair(mqtt_msg, "msg", "invalid format");
            return true;

        case 255: /* UMDK_UART_REPLY_ERR */
            add_value_pair(mqtt_msg, "type", "255");
            add_value_pair(mqtt_msg, "msg", "UART interface error");
            return true;
    }

    return false;
}