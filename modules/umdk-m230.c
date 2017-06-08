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
 * @file	umdk-mercury.c
 * @brief   umdk-mercury message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"


typedef enum {
    M230_CMD_RESET 				= 0xFF,		/* Clear database */
    M230_CMD_ADD_ADDR 			= 0xFE,		/* Add address  in database */
    M230_CMD_REMOVE_ADDR 		= 0xFD,		/* Remove address from database */
	M230_CMD_GET_LIST 			= 0xFC,		/* Send database of addresses */
    
    M230_CMD_PROPRIETARY_COMMAND = 0xF0,	/* Less this value single command of mercury */

    M230_CMD_TEST_RESPONSE		= 0x00,		/* Testing channel - response from device */
	M230_CMD_OPEN_LINE			= 0x01,		/* Open a communication channel */
	M230_CMD_CLOSE_LINE			= 0x02,		/* Close a communication channel */
		
	M230_CMD_GET_VALUE			= 0x03,		/* Read the values of power */

} m230_cmd_t;


typedef enum {
	ERROR_REPLY 		= 0,
    OK_REPLY 			= 1,
    NO_RESPONSE_REPLY	= 2,
} mercury_reply_t;

void umdk_m230_command(char *param, char *out, int bufsize) {
		
	if (strstr(param, "get value ") == param) { 
		param += strlen("get value ");    // Skip command
		uint8_t month = 0;
		if(strstr(param, "total") == param) {
			param += strlen("total");				// Skip command
			month = 0x00;
		}
		else if(strstr(param, "current_year") == param) {
			param += strlen("current_year");				// Skip command
			month = 0x10;
		}
		else if(strstr(param, "last_year") == param) {
			param += strlen("last_year");				// Skip command			
			month = 0x20;
		}
		else if (strstr(param, "month ") == param) { 
			param += strlen("month ");    // Skip command
			month = 0x30;
			month |= strtol(param, &param, 10);
		}
		else if(strstr(param, "current_day") == param) {
			param += strlen("current_day");				// Skip command
			month = 0x40;
		}
		else if(strstr(param, "last_day") == param) {
			param += strlen("last_day");				// Skip command			
			month = 0x50;
		}
		
		param += strlen(" ");    						// Skip space
		uint8_t tariff = strtol(param, &param, 10);
		
		param += strlen(" ");    						// Skip space
		uint8_t destination = strtol(param, &param, 10);
		
		snprintf(out, bufsize, "%02x%08x%02x%02x", M230_CMD_GET_VALUE, destination, month, tariff);		
	}
}

bool umdk_m230_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
	char buf_addr[30];
	
	m230_cmd_t cmd = moddata[0];	
	
	if(cmd < M230_CMD_PROPRIETARY_COMMAND) {
		
		uint8_t address = moddata[1];

		snprintf(buf_addr, sizeof(buf_addr), "%u", address);	
		add_value_pair(mqtt_msg, "Address", buf_addr);
							
		if (moddatalen == 2) {
			if (moddata[0] == OK_REPLY) {
				add_value_pair(mqtt_msg, "Msg", "Ok");
			} else if(moddata[0] == ERROR_REPLY){
				add_value_pair(mqtt_msg, "Msg", "Error");
			} else if(moddata[0] == NO_RESPONSE_REPLY){
				add_value_pair(mqtt_msg, "Msg", "No response");					
			}
			return true;
		}
		
	}
  
	uint8_t i;
	uint32_t * ptr_value;
	
	switch(cmd) {
		
		case M230_CMD_GET_VALUE: {
			uint32_t value[2] = { 0 };
			for(i = 0; i < 2; i++) {
				ptr_value = (uint32_t *)(&moddata[4*i + 2]);
				uint32_to_le(ptr_value);      
				value[i] = *ptr_value;
			}

			snprintf(buf, sizeof(buf), "%u.%u", value[0]/1000, value[0]%1000);
			add_value_pair(mqtt_msg, "A+", buf);				
			snprintf(buf, sizeof(buf), "%u.%u", value[1]/1000, value[1]%1000);
			add_value_pair(mqtt_msg, "R+", buf);				
	
			return true;
			break;
		}		
				
		default:
			break;
	}
	return true;
}
