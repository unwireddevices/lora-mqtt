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
 * @file	umdk-4btn.c
 * @brief   umdk-4btn message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

bool umdk_4btn_reply(uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    
    strcpy(topic, "4btn");
    uint8_t btn = moddata[0];
    uint8_t dir = moddata[1];

    if (moddatalen != 2) {
        return false;
    }

    snprintf(buf, sizeof(buf), "%d", btn);
    add_value_pair(mqtt_msg, "btn", buf);
    if (dir) {
        add_value_pair(mqtt_msg, "state", "released");
    } else {
        add_value_pair(mqtt_msg, "state", "pressed");
    }
    
    return true;
}
