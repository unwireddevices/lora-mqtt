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
 * @file	umdk-m230.c
 * @brief   umdk-m230 message parser
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
	M230_CMD_GET_TIMEDATE		= 0x04,		/* Read the internal time and date */
	M230_CMD_GET_SERIAL			= 0x05,		/* Read serial number of the device */
	M230_CMD_SET_TIMEDATE		= 0x06,		/* Set the internal time and date */
} m230_cmd_t;


typedef enum {
	M230_ERROR_REPLY 		= 0,
    M230_OK_REPLY 			= 1,
    M230_NO_RESPONSE_REPLY	= 2,
} m230_reply_t;

static char season[2][7] = { "Summer", "Winter" };
static char str_dow[8][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Hol" };

void umdk_m230_command(char *param, char *out, int bufsize) {
		
	uint8_t destination;	
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
		destination = strtol(param, &param, 10);
		
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_GET_VALUE, destination, month, tariff);		
	}
	else if (strstr(param, "get timedate ") == param) {
		param += strlen("get timedate ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x00;
		snprintf(out, bufsize, "%02x%08x%02x", M230_CMD_GET_TIMEDATE, destination, tmp);
	}	
	else if (strstr(param, "get serial ") == param) {
		param += strlen("get serial ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x00;
		snprintf(out, bufsize, "%02x%08x%02x", M230_CMD_GET_TIMEDATE, destination, tmp);
	}	
}

bool umdk_m230_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
    char strbuf[20];
	char buf_addr[30];
	
	m230_cmd_t cmd = moddata[0];	
	
	if(cmd < M230_CMD_PROPRIETARY_COMMAND) {
		
		uint8_t address = moddata[1];

		snprintf(buf_addr, sizeof(buf_addr), "%u", address);	
		add_value_pair(mqtt_msg, "Address", buf_addr);
							
		if (moddatalen == 2) {
			if (moddata[0] == M230_OK_REPLY) {
				add_value_pair(mqtt_msg, "Msg", "Ok");
			} else if(moddata[0] == M230_ERROR_REPLY){
				add_value_pair(mqtt_msg, "Msg", "Error");
			} else if(moddata[0] == M230_NO_RESPONSE_REPLY){
				add_value_pair(mqtt_msg, "Msg", "No response");					
			}
			return true;
		}
		
	}
	
    int i = 0;
    
	switch(cmd) {
		
		case M230_CMD_GET_VALUE: {
			uint32_t value[4] = { 0 };
            for (i = 0; i < 4; i++) {
                value[i] = moddata[4*i + 2] | moddata[4*i + 3] << 8 | moddata[4*i + 4] << 16 | moddata[4*i + 5] << 24;
            }


			int_to_float_str(strbuf, value[0], 3);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, "A+", buf);
 
			if(value[1] != 0xFFFFFFFF) {			
				int_to_float_str(strbuf, value[1], 3);
				snprintf(buf, sizeof(buf), "%s", strbuf);
				add_value_pair(mqtt_msg, "A-", buf);		
			}
			else {
				add_value_pair(mqtt_msg, "A-", "Not support");									
			}
			
			int_to_float_str(strbuf, value[2], 3);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, "R+", buf);
			
			if(value[3] != 0xFFFFFFFF) {						
				int_to_float_str(strbuf, value[3], 3);
				snprintf(buf, sizeof(buf), "%s", strbuf);
				add_value_pair(mqtt_msg, "R-", buf);						
			}
			else {
				add_value_pair(mqtt_msg, "R-", "Not support");					
			}
			
			return true;
			break;
		}		
		
		case M230_CMD_GET_TIMEDATE: {
			
			char time_buf[10] = { };
			uint8_t time[8] = { 0 };
			
			for(i = 0; i < 8; i++) {
				time[i] = moddata[i + 2];
			}

			add_value_pair(mqtt_msg, "Day", season[time[3]]);
			
			snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", time[2], time[1], time[0]);	
			add_value_pair(mqtt_msg, "Time", time_buf);
			
			snprintf(time_buf, sizeof(time_buf), "%02d/%02d/%02d", time[4], time[5], time[6]);	
			add_value_pair(mqtt_msg, "Date", time_buf);			
			
			add_value_pair(mqtt_msg, "Season", str_dow[time[7]]);			
			
			return true;
			break;
		}
		
		case M230_CMD_GET_SERIAL: {
			
			char time_buf[10] = { };
			    
			snprintf(buf, sizeof(buf), "%02d%02d%02d%02d", moddata[2], moddata[3], moddata[4], moddata[5]);
			add_value_pair(mqtt_msg, "Serial number", buf);		
			
			snprintf(time_buf, sizeof(time_buf), "%02d/%02d/%02d", moddata[6], moddata[7], moddata[8]);	
			add_value_pair(mqtt_msg, "Release date", time_buf);		
			
			return true;
			break;
		}
				
		default:
			break;
	}
	return true;
}
