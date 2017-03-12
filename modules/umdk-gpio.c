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
 * @file	umdk-gpio.c
 * @brief   umdk-gpio message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_gpio_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set ") == param) {
        param += strlen("set "); // skip command

        uint8_t pin = strtol(param, &param, 10);
        uint8_t value = strtol(param, NULL, 10);

        uint8_t gpio_cmd = 0;
        if (value == 1) {
            gpio_cmd = UNWDS_GPIO_SET_1 << 6;  // 10 in upper two bits of cmd byte is SET TO ONE command
        }
        else if (value == 0) {
            gpio_cmd = UNWDS_GPIO_SET_0 << 6;  // 01 in upper two bits of cmd byte is SET TO ZERO command

        }
        // Append pin number bits and mask exceeding bits just in case
        gpio_cmd |= pin & 0x3F;

        printf("[mqtt-gpio] Set command | Pin: %d, value: %d, cmd: 0x%02x\n", pin, value, gpio_cmd);

        snprintf(out, bufsize, "01%02x", gpio_cmd);
    }
    else if (strstr(param, "get ") == param) {
        param += strlen("get "); // skip command

        uint8_t pin = strtol(param, &param, 10);

        uint8_t gpio_cmd = UNWDS_GPIO_GET << 6;
        // Append pin number bits and mask exceeding bits just in case
        gpio_cmd |= pin & 0x3F;

        printf("[mqtt-gpio] Get command | Pin: %d, cmd: 0x%02x\n", pin, gpio_cmd);

        snprintf(out, bufsize, "01%02x", gpio_cmd);
    }
    else if (strstr(param, "toggle ") == param) {
        param += strlen("toggle "); // skip command

        uint8_t pin = strtol(param, &param, 10);

        uint8_t gpio_cmd = UNWDS_GPIO_TOGGLE << 6;
        // Append pin number bits and mask exceeding bits just in case
        gpio_cmd |= pin & 0x3F;

        printf("[mqtt-gpio] Toggle command | Pin: %d, cmd: 0x%02x\n", pin, gpio_cmd);

        snprintf(out, bufsize, "01%02x", gpio_cmd);
    }
}

bool umdk_gpio_reply(uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];
    strcpy(topic, "gpio");
    
    switch (reply_type) {
        case 0: { /* UNWD_GPIO_REPLY_OK_0 */
            add_value_pair(mqtt_msg, "type", "0");
            add_value_pair(mqtt_msg, "msg", "0");
            break;
        }
        case 1: { /* UNWD_GPIO_REPLY_OK_1 */
            add_value_pair(mqtt_msg, "type", "1");
            add_value_pair(mqtt_msg, "msg", "1");
            break;
        }
        case 2: { /* UNWD_GPIO_REPLY_OK */
            add_value_pair(mqtt_msg, "type", "0");
            add_value_pair(mqtt_msg, "msg", "set ok");
            break;
        }
        case 3: { /* UNWD_GPIO_REPLY_ERR_PIN */
            add_value_pair(mqtt_msg, "type", "3");
            add_value_pair(mqtt_msg, "msg", "invalid pin");
            break;
        }
        case 4: { /* UNWD_GPIO_REPLY_ERR_FORMAT */
            add_value_pair(mqtt_msg, "type", "4");
            add_value_pair(mqtt_msg, "msg", "invalid format");
            break;
        }
    }
    return true;
}
