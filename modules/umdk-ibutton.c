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
 * @file	 umdk-ibutton.c
 * @brief    umdk-ibutton message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
	UMDK_IBUTTON_OK 		= 0x01,		/* OK */
	UMDK_IBUTTON_ERROR 		= 0x00,		/* ERROR */
} umdk_ibutton_reply_t;

typedef enum {
	UMDK_IBUTTON_CMD_NONE = 0x00,
}umdk_ibutton_cmd_t;

void umdk_ibutton_command(char *param, char *out, int bufsize)
 {
	return;
}

bool umdk_ibutton_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
		
    if (moddatalen == 1) {
        if (moddata[0] == UMDK_IBUTTON_OK) {
            add_value_pair(mqtt_msg, "msg", "OK");
        } 
		else {
            add_value_pair(mqtt_msg, "msg", "ERROR");
        }
        return true;
    }

    int  i = 0;
    for (i=7; i >= 0; i--) {
        snprintf(&buf[14 - i*2], 3, "%02X", moddata[i]);
    }
    printf("\n");
    add_value_pair(mqtt_msg, "id", buf);
	return true;
}
