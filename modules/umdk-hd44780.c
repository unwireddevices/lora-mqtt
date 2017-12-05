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
 * @file	umdk-hd44780.c
 * @brief   umdk-hd44780 message parser
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
	UMDK_HD44780_CMD_PRINT = 0,
} umdk_hd44780_cmd_t;

typedef enum {
	UMDK_HD44780_REPLY_OK = 0,
    UMDK_HD44780_REPLY_ERR = 255,
} umdk_hd44780_reply_t;

void umdk_hd44780_command(char *param, char *out, int bufsize) {
    if (strstr(param, "print ") == param) {
        param += strlen("print "); // skip command
        
        uint8_t clear = 0;
        if (strstr(param, "clear ") == param) {
            param += strlen("clear "); // skip command
            clear = (1<<7);
        }

        uint8_t row = strtol(param, &param, 10);
        uint8_t col = strtol(param, NULL, 10);
        
        uint8_t position = ((row << 5) & 0x60) | (col & 0x1f) | (clear & 0x80);
        
        char *line = param + 3; // skip a space
        int linesize = strlen(line);
        
        printf("Line: %s\n", line);
        if (linesize > 20) {
            linesize = 20;
        }
            
        printf("Size: %d\n", linesize);
        
        char buf[21];
        snprintf(buf, linesize + 1, "%s", line);
        printf("Buf: %s\n", buf);

        snprintf(out, bufsize, "%02x%02x", UMDK_HD44780_CMD_PRINT, position);
        
        int  i = 0;
        for (i = 0; i < linesize; i++ ) {
            snprintf(out + 4 + 2*i, 3, "%02X", (uint8_t)buf[i]);
        }
    }
}

bool umdk_hd44780_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    uint8_t reply_type = moddata[0];
    switch (reply_type) {
        case UMDK_HD44780_REPLY_OK: { /*  */
            add_value_pair(mqtt_msg, "msg", "ok");
            break;
        }
        case UMDK_HD44780_REPLY_ERR: { /*  */
            add_value_pair(mqtt_msg, "msg", "error");
            break;
        }
        default:
            puts("ERROR DECODING COMMAND");
            break;
    }
    return true;
}
