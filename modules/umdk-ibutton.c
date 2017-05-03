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
 * @file	umdk-ibutton.c
 * @brief   umdk-ibutton message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
	UMDK_IBUTTON_OK = 0x01,					/* OK */
	UMDK_IBUTTON_ERROR = 0x00,			/* ERROR */
	UMDK_IBUTTON_GRANTED = 0x11,		/* ACCESS GRANTED */
	UMDK_IBUTTON_DENIED = 0x1F,			/* ACCESS DENIED */
	UMDK_IBUTTON_UPDATED = 0x20,		/* ID key removed by timer access */
} umdk_ibutton_reply_t;

typedef enum {
	UMDK_IBUTTON_CMD_RESET_LIST = 0x00,
	UMDK_IBUTTON_CMD_ADD_ID = 0x01,
	UMDK_IBUTTON_CMD_REMOVE_ID = 0x02,
}umdk_ibutton_cmd_t;

void umdk_ibutton_command(char *param, char *out, int bufsize)
 {
	if (strstr(param, "reset") == param) {
		snprintf(out, bufsize, "%02x", UMDK_IBUTTON_CMD_RESET_LIST);
	}
	else if (strstr(param, "add ") == param) {
		param += strlen("add ");    // Skip command
		uint32_t id_high = strtol(param, &param, 10);
		uint32_t id_low = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint16_t time = strtol(param, NULL, 10);
		uint32_to_le(&id_high);
		uint32_to_le(&id_low);
		uint16_to_le(&time);
		snprintf(out, bufsize, "%02x%08x%08x%04x", UMDK_IBUTTON_CMD_ADD_ID, id_high, id_low, time);
	}
	else	if (strstr(param, "remove ") == param) {
		param += strlen("remove ");    // Skip command
		uint32_t id_high = strtol(param, &param, 10);
		uint32_t id_low = strtol(param, &param, 10);
		uint32_to_le(&id_high);
		uint32_to_le(&id_low);
		snprintf(out, bufsize, "%02x%08x%08x", UMDK_IBUTTON_CMD_REMOVE_ID, id_high, id_low);
	}
}

bool umdk_ibutton_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

		umdk_ibutton_reply_t cmd = moddata[0];
		
    if (moddatalen == 1) {
        if (cmd == UMDK_IBUTTON_OK) {
            add_value_pair(mqtt_msg, "msg", "OK");
        } 
				else {
            add_value_pair(mqtt_msg, "msg", "ERROR");
        }
        return true;
    }

	uint32_t *id_high = ((uint32_t *)&moddata[1]);
	uint32_t *id_low = ((uint32_t *)&moddata[5]);
	uint32_to_le(id_high);
	uint32_to_le(id_low);	
	snprintf(buf, sizeof(buf), "%08X%08X", *id_high, *id_low);
			
	switch(cmd) {
		case UMDK_IBUTTON_GRANTED: {        
			add_value_pair(mqtt_msg, "Access GRANTED Id", buf);		
			return true;
			break;
		}
		
		case UMDK_IBUTTON_DENIED: {
			add_value_pair(mqtt_msg, "Access DENIED Id", buf);		
			return true;
			break;
		}

		case UMDK_IBUTTON_UPDATED: {
			add_value_pair(mqtt_msg, "Removed by timer access Id", buf);		
			return true;
			break;
		}
		
		default:
			break;
	}
	return true;
}
