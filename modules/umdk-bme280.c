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
 * @file	umdk-bme280.c
 * @brief   umdk-bme280 message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_bme280_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
}

bool umdk_bme280_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
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

    int16_t temp = 0;
    int16_t hum = 0;
    uint16_t press = 0;
    
    if (is_big_endian()) {
        /* We're in big endian here, swap bytes */
        temp = (moddata[1] << 8) | moddata[0];
        hum = (moddata[3] << 8) | moddata[2];
        press = (moddata[5] << 8) | moddata[4];
    }
    else {
        temp = (moddata[0] << 8) | moddata[1];
        hum = (moddata[2] << 8) | moddata[3];
        press = (moddata[4] << 8) | moddata[5];
    }

    snprintf(buf, sizeof(buf), "%d.%d", temp/10, abs(temp%10));
    add_value_pair(mqtt_msg, "temperature", buf);
    
    snprintf(buf, sizeof(buf), "%d.%d", hum/10, hum%10);
    add_value_pair(mqtt_msg, "humidity", buf);
    
    snprintf(buf, sizeof(buf), "%d", press);
    add_value_pair(mqtt_msg, "pressure", buf);
    
    return true;
}
