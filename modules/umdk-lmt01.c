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
 * @file	umdk-lmt01.c
 * @brief   umdk-lmt01 message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_lmt01_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
    else if (strstr(param, "set_gpios ") == param) {
    /*	 param += strlen("set_gpios ");	// Skip command

         uint8_t gpio = 0;
         while ((gpio = strtol(param, param, 10))

         snprintf(out, bufsize, "02");*/
    }
}

bool umdk_lmt01_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
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
    for (i = 0; i < 8; i += 2) {
        int16_t sensor = 0;
        if (is_big_endian()) {
            sensor = (moddata[i + 1] << 8) | moddata[i]; /* We're in big endian there, swap bytes */
        }
        else {
            sensor = (moddata[i] << 8) | moddata[i + 1];
        }

        char ch[3] = {};
        snprintf(ch, sizeof(ch), "s%d", (i / 2) + 1);

        if (sensor == 0x7FFF) {
            add_value_pair(mqtt_msg, ch, "null");
        }
        else {
            snprintf(buf, sizeof(buf), "%d.%d", sensor/10, abs(sensor%10));
            add_value_pair(mqtt_msg, ch, buf);
        }
    }
    return true;
}
