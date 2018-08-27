/* Copyright (c) 2018 Unwired Devices LLC [info@unwds.com]
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
 * @file	umdk-wiegand.c
 * @brief   umdk-wiegand message parser
 * @author  Mikhail Perkov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

/**
 * @brief Commands list
 */
typedef enum {   
    UMDK_WIEGAND_CMD_DATABASE_RESET 	= 0xFE,		/* Clear database */
    UMDK_WIEGAND_CMD_DATABASE_ADD 		= 0xFD,		/* Add address  in database */
    UMDK_WIEGAND_CMD_DATABASE_REMOVE 	= 0xFC,		/* Remove address from database */
	
	UMDK_WIEGAND_CMD_DATABASE_FIND 		= 0xFB,		/* Find address in database */
} umdk_wiegand_cmd_t;

/**
 * @brief Reply messages values
 */
typedef enum {
	UMDK_WIEGAND_OK_REPLY           = 0,
    UMDK_WIEGAND_ERROR_REPLY        = 1,
	UMDK_WIEGAND_NOT_SUPP_REPLY		= 2,
	UMDK_WIEGAND_NOT_VALID_REPLY	= 3,
	
	UMDK_WIEGAND_REMOVED_REPLY = 4,
	UMDK_WIEGAND_GRANTED_REPLY = 5,
    UMDK_WIEGAND_DENIED_REPLY = 6,
	
	UMDK_WIEGAND_INVALID_CMD_REPLY  = 0xFF,
} wiegand_reply_t;


#define WIEGAND_DEBUG 0

void umdk_wiegand_command(char *param, char *out, int bufsize)
 {
	uint8_t cmd = 0;
	uint16_t time = 0;
	uint64_t id = 0;
	
	if (strstr(param, "reset") == param) {
		cmd = UMDK_WIEGAND_CMD_DATABASE_RESET;
		snprintf(out, bufsize, "%02x", cmd);
		return;
	}
	else if (strstr(param, "add ") == param) {
		param += strlen("add ");    // Skip 
		cmd = UMDK_WIEGAND_CMD_DATABASE_ADD;
	
		id = strtoll(param, &param, 10);
		param += strlen(" ");    						// Skip space
		time = strtol(param, NULL, 10);		
	}
	else if (strstr(param, "remove ") == param) {
		param += strlen("remove ");    // Skip 
		cmd = UMDK_WIEGAND_CMD_DATABASE_REMOVE;		
		id = strtoll(param, &param, 10);
		time = 0;
	}
	else  {
		snprintf(out, bufsize, "%02x", UMDK_WIEGAND_INVALID_CMD_REPLY);
		return;
	}

	snprintf(out, bufsize, "%02x%010llx%04x", cmd, id, time);
	
	return;
}

bool umdk_wiegand_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{	
#if WIEGAND_DEBUG	
	uint8_t ii;
	printf("[wiegand] RX data:  ");
	for(ii = 0; ii < moddatalen; ii++) {
		printf(" %02X ", moddata[ii]);
	}
	puts("\n");
#endif
	
	if (moddatalen == 1) {
		if (moddata[0] == UMDK_WIEGAND_OK_REPLY) {
			add_value_pair(mqtt_msg, "msg", "ok");
		} else if(moddata[0] == UMDK_WIEGAND_ERROR_REPLY){
			add_value_pair(mqtt_msg, "msg", "error");
		} else if(moddata[0] == UMDK_WIEGAND_INVALID_CMD_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		} else if(moddata[0] == UMDK_WIEGAND_NOT_SUPP_REPLY){
			add_value_pair(mqtt_msg, "msg", "not support format");
		}
		return true;
	}
	char card_buf[50];
	char id_buf[50];
	
	uint8_t cmd = moddata[0];
	uint64_t card = 0;
	uint16_t id = (moddata[4] << 8) | moddata[5];
	uint16_t facility = moddata[3];
	uint32_t facility34 = (moddata[1] << 16) | (moddata[2] << 8) | moddata[3];
	
	card = (facility34 << 16) | id;
	
	snprintf(card_buf, sizeof(card_buf), "%llu", card);
	add_value_pair(mqtt_msg, "card", card_buf);
	
	snprintf(id_buf, sizeof(id_buf), "%d,%05d", facility, id);
	add_value_pair(mqtt_msg, "ID", id_buf);
	
	
	switch(cmd) {
		case UMDK_WIEGAND_CMD_DATABASE_ADD: {
			add_value_pair(mqtt_msg, "action", "added");
			break;
		}
		case UMDK_WIEGAND_CMD_DATABASE_REMOVE: {
			add_value_pair(mqtt_msg, "action", "removed");
			break;
		}
		case UMDK_WIEGAND_REMOVED_REPLY: {
			add_value_pair(mqtt_msg, "action", "removed by timer");
			break;
		}
		case UMDK_WIEGAND_GRANTED_REPLY: {
			add_value_pair(mqtt_msg, "action", "granted");
			break;
		}
		case UMDK_WIEGAND_DENIED_REPLY: {
			add_value_pair(mqtt_msg, "action", "denied");
			break;
		}
		default:
			break;
	}
	
    return true;
}
