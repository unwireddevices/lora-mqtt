/*
 * Copyright (C) 2016 Unwired Devices [info@unwds.com]
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
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

        snprintf(out, bufsize, "0700%s", hex);
    }
    else if (strstr(param, "set_baudrate ") == param) {
        param += strlen("set_baudrate "); // Skip commands

        uint8_t baudrate = atoi(param);
        printf("baudrate: %d\n", baudrate);
        if (baudrate > 10) {
            return;
        }

        snprintf(out, bufsize, "0701%02x", baudrate);
    }
    else if (strstr(param, "set ") == param) {
        param += strlen("set "); // Skip commands

        if (strlen(param) > strlen("115200-8N1")) {
            return;
        }

        snprintf(out, bufsize, "0702");
        
        /* convert string to hex */
        uint8_t k;
        for (k = 0; k < strlen(param); k++) {
            snprintf(out + 4 + 2*k, 3, "%02x", param[k]);
        }
        
        printf("UART mode: %s, Command: %s\n", param, out);
    }
    else {
        return;
    }
}

bool umdk_uart_reply(uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];

    strcpy(topic, "uart");

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
