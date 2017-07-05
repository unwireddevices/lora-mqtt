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

typedef enum {
	UMDK_GPIO_REPLY_OK_0 = 0,
	UMDK_GPIO_REPLY_OK_1 = 1,
	UMDK_GPIO_REPLY_OK = 2,
	UMDK_GPIO_REPLY_ERR_PIN = 3,
	UMDK_GPIO_REPLY_ERR_FORMAT = 4,
    UMDK_GPIO_REPLY_OK_AINAF = 5,
    UMDK_GPIO_REPLY_OK_ALL = 6
} umdk_gpio_reply_t;

typedef enum {
	UMDK_GPIO_GET = 0,
	UMDK_GPIO_SET_0 = 1,
	UMDK_GPIO_SET_1 = 2,
	UMDK_GPIO_TOGGLE = 3,
    UMDK_GPIO_GET_ALL = 4,
} umdk_gpio_action_t;

void umdk_gpio_command(char *param, char *out, int bufsize) {
    if (strstr(param, "set ") == param) {
        param += strlen("set "); // skip command

        uint8_t pin = strtol(param, &param, 10);
        uint8_t value = strtol(param, NULL, 10);

        uint8_t gpio_cmd = 0;
        if (value == 1) {
            gpio_cmd = UMDK_GPIO_SET_1 << 5;  // 10 in upper two bits of cmd byte is SET TO ONE command
        }
        else if (value == 0) {
            gpio_cmd = UMDK_GPIO_SET_0 << 5;  // 01 in upper two bits of cmd byte is SET TO ZERO command

        }
        // Append pin number bits and mask exceeding bits just in case
        gpio_cmd |= pin & 0x3F;

        printf("[mqtt-gpio] Set command | Pin: %d, value: %d, cmd: 0x%02x\n", pin, value, gpio_cmd);

        snprintf(out, bufsize, "%02x", gpio_cmd);
    }
    else if (strstr(param, "get ") == param) {
        param += strlen("get "); // skip command

        uint8_t pin;
        uint8_t gpio_cmd;
        
        if (strstr(param, "all") == param) {
            pin = 0;
            gpio_cmd = UMDK_GPIO_GET_ALL << 5;
        } else {
            pin = strtol(param, &param, 10);
            gpio_cmd = (UMDK_GPIO_GET << 5) | (pin & 0x1F);
        }

        printf("[mqtt-gpio] Get command | Pin: %d, cmd: 0x%02x\n", pin, gpio_cmd);

        snprintf(out, bufsize, "%02x", gpio_cmd);
    }
    else if (strstr(param, "toggle ") == param) {
        param += strlen("toggle "); // skip command

        uint8_t pin = strtol(param, &param, 10);

        uint8_t gpio_cmd = UMDK_GPIO_TOGGLE << 6;
        // Append pin number bits and mask exceeding bits just in case
        gpio_cmd |= pin & 0x3F;

        printf("[mqtt-gpio] Toggle command | Pin: %d, cmd: 0x%02x\n", pin, gpio_cmd);

        snprintf(out, bufsize, "%02x", gpio_cmd);
    }
}

bool umdk_gpio_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];
    switch (reply_type) {
        case UMDK_GPIO_REPLY_OK_ALL: {
            /* bit 0: pin value, bit 1: 0 for IN, 1 for OUT, bit 3: AF/AIN, bit 4: reserved */
            /* 0b111 means pin not used, 0b110 — AIN, 0b100 — AF */
            
            int i = 0;
            char buf[100] = {};
            char status[10];
            
            strcat(buf, "[ ");
            for (i = 0; i < ((moddatalen - 1) * 2); i++) {
                int pin_data = (moddata[1 + i/2] >> (4*(i%2))) & 0b111;
                snprintf(status, sizeof(status), "%d, ", pin_data);
                strcat(buf, status);
            }
            strcat(buf, "]");
            add_value_pair(mqtt_msg, "gpios", buf);
            
            /*
            for (i = 0; i < ((moddatalen - 1) * 2); i++) {
                int pin_data = (moddata[1 + i/2] >> (4*(i%2))) & 0b111;
                if (pin_data == 0b111) {
                    snprintf(status, 10, "NC");
                } else {
                    switch(pin_data >> 2) {
                        case 0b00:
                            snprintf(status, 10, "DI");
                            break;
                        case 0b01:
                            snprintf(status, 10, "DO");
                            break;
                        case 0b10:
                            snprintf(status, 10, "AF");
                            break;
                        case 0b11:
                            snprintf(status, 10, "AI");
                            break;
                    }
                }
                snprintf(buf, sizeof(buf), "{ \"s\": \"%s\", \"v\": %d}", status, pin_data & 0x1);
                snprintf(status, sizeof(status), "DIO %d", i);
                add_value_pair(mqtt_msg, status, buf);
                
                printf("Pin: %d, status: %s, value: %d\n", i, status, pin_data & 0x1);
            }
            */
            break;
        }

        case UMDK_GPIO_REPLY_OK_0: { /*  */
            add_value_pair(mqtt_msg, "value", "0");
            break;
        }
        case UMDK_GPIO_REPLY_OK_1: { /*  */
            add_value_pair(mqtt_msg, "value", "1");
            break;
        }
        case UMDK_GPIO_REPLY_OK: { /*  */
            add_value_pair(mqtt_msg, "msg", "set ok");
            break;
        }
        case UMDK_GPIO_REPLY_ERR_PIN: { /*  */
            add_value_pair(mqtt_msg, "msg", "invalid pin");
            break;
        }
        case UMDK_GPIO_REPLY_ERR_FORMAT: { /*  */
            add_value_pair(mqtt_msg, "msg", "invalid format");
            break;
        }
        case UMDK_GPIO_REPLY_OK_AINAF: {
            add_value_pair(mqtt_msg, "value", "3");
            break;
        }
        default:
            puts("ERROR DECODING COMMAND");
            break;
    }
    return true;
}
