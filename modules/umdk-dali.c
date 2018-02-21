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
 * @file	umdk-dali.c
 * @brief   umdk-dali message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

#define DALI_DEBUG 0

typedef enum {   

	DALI_FRONTIER			= 0xF0,
    DALI_INIT_RAND			= 0xF1,		/* Random address allocation */
    DALI_INIT_SELECT		= 0xF2,		/* Physical selection address allocation*/
	DALI_INIT_SINGLE		= 0xF3,		/* Simplified addressing method (only for one ballast separately connected) */

    DALI_CMD_POWER_CONTROL			= 0x00,		/* Direct arc power control */

    DALI_CMD_OFF					= 0x01,		/* Swicth off */
	
	DALI_CMD_UP						= 0x02,		/* Dim up */
	DALI_CMD_DOWN					= 0x03,		/* Dim down */
    DALI_CMD_STEP_UP				= 0x04,		/* One power step higher immediately without fading */
    DALI_CMD_STEP_DOWN				= 0x05,		/* One power step lower immediately without fading */
	DALI_CMD_MAX_LEVEL				= 0x06,		/* Set the max level immediately without fading */
	DALI_CMD_MIN_LEVEL				= 0x07,		/* Set the min level immediately without fading */
    DALI_CMD_STEP_DOWN_OFF			= 0x08,		/* One power step lower immediately without fading. Switch off if level is min */
    DALI_CMD_STEP_UP_ON				= 0x09,		/* One power step higher immediately without fading. Switch on/*/
	
	DALI_CMD_GOTO_SCENE				= 0x0A,		/* Dimming to power scene */
	
/* ----------------------------- */	
	DALI_CMD_RESET					= 0x0B,		/* Reset all values by default */
    
	DALI_CMD_SAVE_ACTUAL_LEVEL		= 0x0C,		/* Store actual level in the DTR */
	
    DALI_CMD_STORE_MAX_LEVEL		= 0x0D,		/* Store new max level value */
	DALI_CMD_STORE_MIN_LEVEL		= 0x0E,		/* Store new min level value */
    DALI_CMD_STORE_SYS_FAIL_LEVEL	= 0x0F,		/* Store new system failure level value */
    DALI_CMD_STORE_POWER_ON_LEVEL	= 0x10,		/* Store new power on level value */
    DALI_CMD_STORE_FADE_TIME		= 0x11,		/* Store new fade time value */
	DALI_CMD_STORE_FADE_RATE		= 0x12,		/* Store new fade rate value */
    DALI_CMD_STORE_SCENE			= 0x13,		/* Store new scene level value */
	
	DALI_CMD_REMOVE_SCENE			= 0x14,		/* Remove the ballast from scene */
    DALI_CMD_ADD_GROUP				= 0x15,		/* Add the ballast to group */
	DALI_CMD_REMOVE_GROUP			= 0x16,		/* Remove the ballast from group */
 /* ----------------------------- */	   

/********************************************/
	DALI_CMD_QUERY_STATUS			= 0x17,		/* Query ballast status */
	DALI_CMD_QUERY_BALLAST			= 0x18,		/* Query ballast communicate status */
	DALI_CMD_QUERY_LAMP_FAIL		= 0x19,		/* Query lamp status */
	DALI_CMD_QUERY_LAMP_POWER_ON	= 0x1A,		/* Query lamp power */
	DALI_CMD_QUERY_LIMIT_ERROR		= 0x1B,		/*  */
	DALI_CMD_QUERY_RESET_STATE		= 0x1C,		/* Query ballast reset status */
	DALI_CMD_QUERY_MISS_SHORT_ADDR	= 0x1D,		/* Query ballast short address status */
	DALI_CMD_QUERY_VERSION			= 0x1E,		/* Query ballast version */
	
	DALI_CMD_QUERY_CONTENT_DTR		= 0x1F,		/* Query DTR content */
	
	DALI_CMD_QUERY_DEVICE_TYPE		= 0x20,		/* Query device type */
	DALI_CMD_QUERY_PHYS_MIN_LEVEL	= 0x21,		/* Query physical minimum level */
	DALI_CMD_QUERY_POWER_FAIL		= 0x22,		/* Query power failure */
	DALI_CMD_QUERY_ACTUAL_LEVEL		= 0x23,		/* Query actual level */
	DALI_CMD_QUERY_MAX_LEVEL		= 0x24,		/* Query max level value */
	DALI_CMD_QUERY_MIN_LEVEL		= 0x25,		/* Query min level value */
	DALI_CMD_QUERY_POWER_ON_LEVEL	= 0x26,		/* Query power on level value */
	DALI_CMD_QUERY_SYS_FAIL_LEVEL	= 0x27,		/* Query system failure level value */
	DALI_CMD_QUERY_FADE				= 0x28,		/* Query fade time and fade rate */
	DALI_CMD_QUERY_SCENE_LEVEL		= 0x29,		/* Query scene level */
	DALI_CMD_QUERY_GROUPS_0_7		= 0x2A,		/* Query group(0-7) belonging */
	DALI_CMD_QUERY_GROUPS_8_15		= 0x2B,		/* Query group(8-15) belonging */
/********************************************/	
} dali_cmd_t;


typedef enum {
    UMDK_DALI_ERROR_REPLY        = 0,
    UMDK_DALI_OK_REPLY           = 1,
    UMDK_DALI_NO_RESPONSE_REPLY  = 2,
	UMDK_DALI_WAIT_REPLY		 = 3,
    UMDK_DALI_INVALID_CMD_REPLY  = 0xFF,
} dali_reply_t;


void umdk_dali_command(char *param, char *out, int bufsize)
{
	uint8_t cmd = 0;
	uint8_t data = 0;
	uint8_t data_dtr = 0;
	uint8_t address = 0;
	uint8_t repeat = 0;
	uint8_t answer = 0;
	uint8_t type_addr = 0;
	uint8_t s_bit = 0;
	uint8_t dtr = 0;
	uint8_t service = 0;
	
	if (strstr(param, "init_rand") == param) {
		cmd = DALI_INIT_RAND;
		snprintf(out, bufsize, "%02x", cmd);
		return;
	}
	else if (strstr(param, "init_select") == param) {
		cmd = DALI_INIT_SELECT;
		snprintf(out, bufsize, "%02x", cmd);
		return;
	}
	else if (strstr(param, "init_single") == param) {
		cmd = DALI_INIT_SINGLE;
		snprintf(out, bufsize, "%02x", cmd);
		return;
	}
	else if (strstr(param, "set power ") == param) {
		param += strlen("set power ");    // Skip 
		
		data = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
	
		cmd = DALI_CMD_POWER_CONTROL;
		
		s_bit = 0;
		dtr = 0;
		data_dtr = 0;
		repeat = 0;
		answer = 0;			
	}
	else if (strstr(param, "off ") == param) {
		param += strlen("off ");    // Skip 
		
		data = 0x00;
		cmd = DALI_CMD_OFF;
		
		s_bit = 1;
		dtr = 0;
		data_dtr = 0;
		repeat = 0;
		answer = 0;			
	}
	else  {
		snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
		return;
	}
	
	if (strstr(param, "broadcast") == param) {
		address = 0x3F;
		type_addr = 1;
	}
	else if (strstr(param, "group ") == param) {
		param += strlen("group ");    // Skip 		
		address = strtol(param, &param, 10);
		type_addr = 1;
	}
	else if (strstr(param, "dev ") == param) {
		param += strlen("dev ");    // Skip 		
		address = strtol(param, &param, 10);
		type_addr = 0;
	}
	else  {
		snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
		return;
	}
	
	address = (type_addr << 7) + (address << 1) + (s_bit << 0);
	service = (repeat << 2) + (answer << 1) + (dtr << 0);
	
	snprintf(out, bufsize, "%02x%02x%02x%02x%02x", cmd, service, data_dtr, address, data);
	
	return;
}

bool umdk_dali_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	// char buf[100];
    // char strbuf[20];
	// char buf_addr[10];
	// uint16_t num_char = 0;
	// uint16_t i = 0;
	// uint8_t symbol = 0;
	// uint8_t * data_ptr = NULL;
	
#if DALI_DEBUG	
	uint8_t ii;
    printf("[dali] RX data:  ");
    for(ii = 0; ii < moddatalen; ii++) {
        printf(" %02X ", moddata[ii]);
    }
   puts("\n");
#endif
	
   if (moddatalen == 1) {
        if (moddata[0] == UMDK_DALI_OK_REPLY) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else if(moddata[0] == UMDK_DALI_ERROR_REPLY){
            add_value_pair(mqtt_msg, "msg", "error");
		}
		else if(moddata[0] == UMDK_DALI_INVALID_CMD_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		}
        return true;
    }	
				
	return true;
}
