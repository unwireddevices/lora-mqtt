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
 * @file	umdk-iec61107.c
 * @brief   umdk-iec61107 message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"


typedef enum {
	UMDK_IEC61107_ERROR_REPLY 		= 0,
    UMDK_IEC61107_OK_REPLY 			= 1,
    UMDK_IEC61107_NO_RESPONSE_REPLY 	= 2,
	UMDK_IEC61107_INVALID_CMD_REPLY 	= 0xFF,
} iec61107_reply_t;

// static char str_dow[8][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Hol" };

void umdk_iec61107_command(char *param, char *out, int bufsize) {
		
	if (strstr(param, "set address ") == param) {
		param += strlen("set address ");    // Skip command
	}
	else if (strstr(param, "get serial ") == param) {
		param += strlen("get serial ");    // Skip command
	}
	else if (strstr(param, "get value ") == param) { 
		param += strlen("get value ");    // Skip command
		// uint8_t month = 0;
		if(strstr(param, "total") == param) {
			param += strlen("total");				// Skip command
			// month = 0xFF;
		}
		// else if(strstr(param, "current") == param) {
			// param += strlen("current");				// Skip command
			// month = 0x0F;
		// }
		// else if(strstr(param, "month ") == param) {
			// param += strlen("month ");				// Skip command			
			// month = strtol(param, &param, 10);
			// month--;
		// }
		else {
			snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
			return;
		}
		
		param += strlen(" ");    						// Skip space
		
		char *param_ptr = param;
		uint8_t symb_nex = 0;
		uint8_t length = (uint8_t)strlen(param_ptr);

		printf("Length: [ %d ]\n", length);

		uint16_t num_char;		
		num_char = snprintf(out, bufsize, "%02x%02x", 0x01, length);
		uint8_t i = 0;
		for(i = 0; i < length; i++) {
			symb_nex = *param_ptr;
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x", symb_nex);			
			param_ptr++;
		}
		
		num_char += snprintf(out + num_char, bufsize - num_char, "%02x", 0x28);	
		num_char += snprintf(out + num_char, bufsize - num_char, "%02x", 0x29);	
		
	}
	else {
		snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
		return;
	}
}

bool umdk_iec61107_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	// char buf[100];
    // char strbuf[20];
	// char buf_addr[30];
	
   if (moddatalen == 1) {
        if (moddata[0] == UMDK_IEC61107_OK_REPLY) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else if(moddata[0] == UMDK_IEC61107_ERROR_REPLY){
            add_value_pair(mqtt_msg, "msg", "error");
		}
		else if(moddata[0] == UMDK_IEC61107_INVALID_CMD_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		}
        return true;
    }	
	
	// m200_cmd_t cmd = moddata[0];	
	
	// if(cmd < M200_CMD_PROPRIETARY_COMMAND) {
		// uint32_t *address = (uint32_t *)(&moddata[1]);
		// uint32_to_le(address);
		// snprintf(buf_addr, sizeof(buf_addr), "%u", *address);	
		// add_value_pair(mqtt_msg, "Address", buf_addr);
							
		// if (moddatalen == 5) {
			// if (moddata[0] == UMDK_IEC61107_OK_REPLY) {
				// add_value_pair(mqtt_msg, "Msg", "Ok");
			// } else if(moddata[0] == UMDK_IEC61107_ERROR_REPLY){
				// add_value_pair(mqtt_msg, "Msg", "Error");
			// } else if(moddata[0] == UMDK_IEC61107_NO_RESPONSE_REPLY){
				// add_value_pair(mqtt_msg, "Msg", "No response");					
			// }
			// return true;
		// }
	// }
  
	// uint8_t i;
	// uint32_t * ptr_value;
	
	// switch(cmd) {			
		// case M200_CMD_GET_TOTAL_VALUE: {
			// uint32_t value[5] = { 0 };
			// for(i = 0; i < 5; i++) {
				// ptr_value = (uint32_t *)(&moddata[4*i + 5]);
				// uint32_to_le(ptr_value);      
				// value[i] = *ptr_value;
			// }
			 		
			// char tariff[5] = { };
			// for(i = 0; i < 4; i++) {
				// snprintf(tariff, sizeof(tariff), "T%02d", i + 1);
                // int_to_float_str(strbuf, value[i], 2);
				// snprintf(buf, sizeof(buf), "%s", strbuf);
				// add_value_pair(mqtt_msg, tariff, buf);								
			// }
            // int_to_float_str(strbuf, value[4], 2);
			// snprintf(buf, sizeof(buf), "%s", strbuf);
			// add_value_pair(mqtt_msg, "Total", buf);		
			
			// return true;
			// break;
		// }

		// case M200_CMD_GET_VALUE: {
			// uint32_t value[5] = { 0 };
			// for(i = 0; i < 5; i++) {
				// ptr_value = (uint32_t *)(&moddata[4*i + 5]);
				// uint32_to_le(ptr_value);      
				// value[i] = *ptr_value;
			// }
			
			// char tariff[5] = { };
			// for(i = 0; i < 4; i++) {
				// snprintf(tariff, sizeof(tariff), "T%02d", i + 1);
                // int_to_float_str(strbuf, value[i], 2);
				// snprintf(buf, sizeof(buf), "%s", strbuf);
				// add_value_pair(mqtt_msg, tariff, buf);				
			// }
            // int_to_float_str(strbuf, value[4], 2);
			// snprintf(buf, sizeof(buf), "%s", strbuf);
			// add_value_pair(mqtt_msg, "Total", buf);		
	
			// return true;
			// break;
		// }		
				
		// default:
			// break;
	// }
	return true;
}
