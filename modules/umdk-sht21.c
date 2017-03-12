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
 * @file	umdk-sht21.c
 * @brief   umdk-sht21 message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_sht21_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set_period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "0800%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "0801");
    }
    else if (strstr(param, "set_i2c ") == param) { 
         param += strlen("set_i2c ");	// Skip command

         uint8_t i2c = atoi(param);

         snprintf(out, bufsize, "0802%02x", i2c);
    }
}

bool umdk_sht21_reply(uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    strcpy(topic, "sht21");

    if (moddatalen == 1) {
        if (moddata[0] == 0) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else {
            add_value_pair(mqtt_msg, "msg", "error");
        }
        return true;
    }

    int16_t temp = 0;
    if (is_big_endian()) {
        temp = (moddata[1] << 8) | moddata[0]; /* We're in big endian there, swap bytes */
    }
    else {
        temp = (moddata[0] << 8) | moddata[1];
    }

    int16_t humid = moddata[2];
    
    snprintf(buf, sizeof(buf), "%d.%d", temp/10, abs(temp%10));
    add_value_pair(mqtt_msg, "temp", buf);
    
    snprintf(buf, sizeof(buf), "%d.%d", humid/10, humid%10);
    add_value_pair(mqtt_msg, "humid", buf);
    
    return true;
}
