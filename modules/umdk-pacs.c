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
 * @file	umdk-pacs.c
 * @brief   umdk-pacs message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
	UMDK_PACS_OK 		= 0x01,		/* OK */
	UMDK_PACS_ERROR 		= 0x00,		/* ERROR */
	UMDK_PACS_GRANTED 	= 0x11,		/* ACCESS GRANTED */
	UMDK_PACS_DENIED	 	= 0x1F,		/* ACCESS DENIED */
	UMDK_PACS_UPDATED 	= 0x20,		/* ID key removed by timer access */
} umdk_pacs_reply_t;

typedef enum {
	UMDK_PACS_CMD_RESET_LIST = 0x00,
	UMDK_PACS_CMD_ADD_ID = 0x01,
	UMDK_PACS_CMD_REMOVE_ID = 0x02,
}umdk_pacs_cmd_t;

void umdk_pacs_command(char *param, char *out, int bufsize)
 {
	if (strstr(param, "reset") == param) {
		snprintf(out, bufsize, "%02x", UMDK_PACS_CMD_RESET_LIST);
	}
	else if (strstr(param, "add ") == param) {
		param += strlen("add ");    // Skip command
		uint64_t id = strtoll(param, &param, 16);
		param += strlen(" ");    						// Skip space
		uint16_t time = strtol(param, NULL, 10);
		snprintf(out, bufsize, "%02x%016lx%04x", UMDK_PACS_CMD_ADD_ID, id, time);
	}
	else if (strstr(param, "remove ") == param) {
		param += strlen("remove ");    // Skip command
		uint64_t id = strtoll(param, &param, 16);
		snprintf(out, bufsize, "%02x%016lx", UMDK_PACS_CMD_REMOVE_ID, id);
	}
}

bool umdk_pacs_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

	umdk_pacs_reply_t cmd = moddata[0];
		
    if (moddatalen == 1) {
        if (cmd == UMDK_PACS_OK) {
            add_value_pair(mqtt_msg, "msg", "OK");
        } 
		else {
            add_value_pair(mqtt_msg, "msg", "ERROR");
        }
        return true;
    }

	uint64_t *id = ((uint64_t *)&moddata[1]);

	snprintf(buf, sizeof(buf), "%016lX", *id);
			
	switch(cmd) {
		case UMDK_PACS_GRANTED: {        
			add_value_pair(mqtt_msg, "GRANTED", buf);		
			return true;
			break;
		}
		
		case UMDK_PACS_DENIED: {
			add_value_pair(mqtt_msg, "DENIED", buf);		
			return true;
			break;
		}

		case UMDK_PACS_UPDATED: {
			add_value_pair(mqtt_msg, "Removed by timer", buf);		
			return true;
			break;
		}
		
		default:
			break;
	}
	return true;
}
