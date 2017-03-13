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
 * @file	umdk-rssiecho.c
 * @brief   umdk-rssiecho message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_rssiecho_command(char *param, char *out, int bufsize) {
    if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "00");
    }
}

bool umdk_rssiecho_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    if (moddatalen < 2) {
        return false;
    }

    /* Extract RSSI value */
    int16_t rssi = 0;

    if (is_big_endian()) {
        rssi = moddata[0];
        rssi += (moddata[1] << 8);
    }
    else {
        rssi = moddata[1];
        rssi += (moddata[0] << 8);
    }

    snprintf(buf, sizeof(buf), "%d", rssi);
    add_value_pair(mqtt_msg, "rssi", buf);
    
    return true;
}
