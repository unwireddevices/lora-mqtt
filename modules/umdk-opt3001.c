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
 * @file	umdk-opt3001.c
 * @brief   umdk-opt3001 message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_opt3001_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set_period ") == param) {
        param += strlen("set period ");    // Skip command

        uint8_t period = atoi(param);
        snprintf(out, bufsize, "00%02x", period);
    }
    else if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "01");
    }
    else if (strstr(param, "set_i2c ") == param) { 
         param += strlen("set_i2c ");	// Skip command

         uint8_t i2c = atoi(param);

         snprintf(out, bufsize, "02%02x", i2c);
    }
}

bool umdk_opt3001_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
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

	uint16_t lum = 0;
	if (is_big_endian()) {
        lum = (moddata[1] << 8) | moddata[0]; /* We're in big endian there, swap bytes */
    } else {
        lum = (moddata[0] << 8) | moddata[1];
    }

    snprintf(buf, sizeof(buf), "%d", lum);
    add_value_pair(mqtt_msg, "luminocity", buf);

	return true;
}
