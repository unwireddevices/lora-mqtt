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
#include <math.h>

#include "unwds-modules.h"
#include "utils.h"

#define DALI_DEBUG 1

typedef enum {   
/*****/	
    DALI_CMD_POWER_CONTROL			= 0xFF,		/* Direct arc power control */

    DALI_CMD_OFF					= 0x00,		/* Swicth off */
	DALI_CMD_UP						= 0x01,		/* Dim up */
	DALI_CMD_DOWN					= 0x02,		/* Dim down */
    DALI_CMD_STEP_UP				= 0x03,		/* One power step higher immediately without fading */
    DALI_CMD_STEP_DOWN				= 0x04,		/* One power step lower immediately without fading */
	DALI_CMD_MAX_LEVEL				= 0x05,		/* Set the max level immediately without fading */
	DALI_CMD_MIN_LEVEL				= 0x06,		/* Set the min level immediately without fading */
    DALI_CMD_STEP_DOWN_OFF			= 0x07,		/* One power step lower immediately without fading. Switch off if level is min */
    DALI_CMD_STEP_UP_ON				= 0x08,		/* One power step higher immediately without fading. Switch on/*/	
	DALI_CMD_GOTO_SCENE				= 0x10,		/* Dimming to power scene */
/*****/	
	DALI_CMD_RESET					= 0x20,		/* Reset all values by default */    
	DALI_CMD_SAVE_ACTUAL_LEVEL		= 0x21,		/* Store actual level in the DTR */	
    DALI_CMD_STORE_MAX_LEVEL		= 0x2A,		/* Store new max level value */
	DALI_CMD_STORE_MIN_LEVEL		= 0x2B,		/* Store new min level value */
    DALI_CMD_STORE_SYS_FAIL_LEVEL	= 0x2C,		/* Store new system failure level value */
    DALI_CMD_STORE_POWER_ON_LEVEL	= 0x2D,		/* Store new power on level value */
    DALI_CMD_STORE_FADE_TIME		= 0x2E,		/* Store new fade time value */
	DALI_CMD_STORE_FADE_RATE		= 0x2F,		/* Store new fade rate value */
    DALI_CMD_STORE_SCENE			= 0x40,		/* Store new scene level value */	
	DALI_CMD_REMOVE_SCENE			= 0x50,		/* Remove the ballast from scene */
    DALI_CMD_ADD_GROUP				= 0x60,		/* Add the ballast to group */
	DALI_CMD_REMOVE_GROUP			= 0x70,		/* Remove the ballast from group */
	DALI_CMD_STORE_SHORT_ADDR		= 0x80,		/* Store DTR as short address */
/*****/	
	DALI_CMD_QUERY_STATUS			= 0x90,		/* Query ballast status */
	DALI_CMD_QUERY_BALLAST			= 0x91,		/* Query ballast communicate status */
	DALI_CMD_QUERY_LAMP_FAIL		= 0x92,		/* Query lamp status */
	DALI_CMD_QUERY_LAMP_POWER_ON	= 0x93,		/* Query lamp power */
	DALI_CMD_QUERY_LIMIT_ERROR		= 0x94,		/*  */
	DALI_CMD_QUERY_RESET_STATE		= 0x95,		/* Query ballast reset status */
	DALI_CMD_QUERY_MISS_SHORT_ADDR	= 0x96,		/* Query ballast short address status */
	DALI_CMD_QUERY_VERSION			= 0x97,		/* Query ballast version */
	
	DALI_CMD_QUERY_CONTENT_DTR		= 0x98,		/* Query DTR content */
	
	DALI_CMD_QUERY_DEVICE_TYPE		= 0x99,		/* Query device type */
	DALI_CMD_QUERY_PHYS_MIN_LEVEL	= 0x9A,		/* Query physical minimum level */
	DALI_CMD_QUERY_POWER_FAIL		= 0x9B,		/* Query power failure */
	DALI_CMD_QUERY_ACTUAL_LEVEL		= 0xA0,		/* Query actual level */
	DALI_CMD_QUERY_MAX_LEVEL		= 0xA1,		/* Query max level value */
	DALI_CMD_QUERY_MIN_LEVEL		= 0xA2,		/* Query min level value */
	DALI_CMD_QUERY_POWER_ON_LEVEL	= 0xA3,		/* Query power on level value */
	DALI_CMD_QUERY_SYS_FAIL_LEVEL	= 0xA4,		/* Query system failure level value */
	DALI_CMD_QUERY_FADE				= 0xA5,		/* Query fade time and fade rate */
	DALI_CMD_QUERY_SCENE_LEVEL		= 0xB0,		/* Query scene level */
	DALI_CMD_QUERY_GROUPS_0_7		= 0xC0,		/* Query group(0-7) belonging */
	DALI_CMD_QUERY_GROUPS_8_15		= 0xC1,		/* Query group(8-15) belonging */
/*****/	
	DALI_INIT_COMMANDS		= 0xF0,

    DALI_INIT_RAND			= 0xF1,		/* Random address allocation */
	DALI_INIT_SINGLE		= 0xF2,		/* Simplified addressing method (only for one ballast separately connected) */
	
} dali_cmd_t;

#define DALI_BROADCAST 0x3F

#define DALI_S_BIT_POWER 0			/* Selector bit: direct arc power control */
#define DALI_S_BIT_CMD 1			/* Selector bit: command */

#define DALI_SHORT_ADDR 0			/* Type address: short address */
#define DALI_GROUP_ADDR 1			/* Type address: broadcast or group address */

#define DALI_REPEAT 	1			/* Need to repeat command */
#define DALI_NOT_REPEAT 0			/* Need to no repeat command */

#define DALI_STORE 		1			/* Permission for save data in the DTR */
#define DALI_NOT_STORE 	0			/* Not permission for save data in the DTR */

#define DALI_NO_ANSWER 0			/* DALI ballast will not answer */
#define	DALI_ANSWER	   1			/* DALI ballast will answer - YES/NO */

#define DALI_RX_DATA 1				/* Answer -> data */
#define DALI_RX_YES_NO 0			/* Answer -> yes/no */

#define DALI_YES 0xFF  
#define DALI_NO 0x00   
#define DALI_DATA_ERR 0xFF 

typedef enum {
    UMDK_DALI_ERROR_REPLY        = 0,
    UMDK_DALI_OK_REPLY           = 1,
	UMDK_DALI_WAIT_REPLY		 = 3,
    UMDK_DALI_INVALID_CMD_REPLY  = 0xFF,
} dali_reply_t;


void umdk_dali_command(char *param, char *out, int bufsize)
{
	uint8_t cmd = 0;
	uint8_t data = 0;
	uint8_t address = 0;
	uint8_t repeat = DALI_NOT_REPEAT;
	uint8_t answer = DALI_NO_ANSWER;
	uint8_t type_rx = DALI_RX_DATA;
	uint8_t type_addr = 0;
	uint8_t s_bit = DALI_S_BIT_CMD;
	uint8_t store_dtr = DALI_NOT_STORE;
	uint8_t service = 0;
	uint8_t data_tmp = 0;
	uint8_t intensity = 0;
	
	if (strstr(param, "init ") == param) {
		param += strlen("init ");    // Skip
		if (strstr(param, "rand") == param) {
			param += strlen("rand");    // Skip
			cmd = DALI_INIT_RAND;
			data = 0;
		}
		else if (strstr(param, "single ") == param) {
			param += strlen("single ");    // Skip
			
			data = strtol(param, &param, 10);
		
			cmd = DALI_INIT_SINGLE;			
		}
		else {
			snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
			return;
		}
		
		service = 0;
		address = 0;
		snprintf(out, bufsize, "%02x%02x%02x%02x", cmd, service, data, address);
		return;
	}
	else if (strstr(param, "set power ") == param) {
		param += strlen("set power ");    // Skip 
		
		intensity = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		data = (uint8_t)((253*log10(intensity) + 256) / 3);
		
		cmd = DALI_CMD_POWER_CONTROL;		
		s_bit = DALI_S_BIT_POWER;
	}
	else if (strstr(param, "off ") == param) {
		param += strlen("off ");    // Skip 		
		cmd = DALI_CMD_OFF;
	}
	else if (strstr(param, "up ") == param) {
		param += strlen("up ");    // Skip 		
		cmd = DALI_CMD_UP;
	}
	else if (strstr(param, "down ") == param) {
		param += strlen("down ");    // Skip 		
		cmd = DALI_CMD_DOWN;
	}
	else if (strstr(param, "step_up ") == param) {
		param += strlen("step_up ");    // Skip 		
		cmd = DALI_CMD_STEP_UP;
	}
	else if (strstr(param, "step_down ") == param) {
		param += strlen("step_down ");    // Skip 		
		cmd = DALI_CMD_STEP_DOWN;
	}
	else if (strstr(param, "max ") == param) {
		param += strlen("max ");    // Skip 		
		cmd = DALI_CMD_MAX_LEVEL;
	}
	else if (strstr(param, "min ") == param) {
		param += strlen("min ");    // Skip 		
		cmd = DALI_CMD_MIN_LEVEL;
	}
	else if (strstr(param, "step_down_off ") == param) {
		param += strlen("step_down_off ");    // Skip 		
		cmd = DALI_CMD_STEP_DOWN_OFF;
	}
	else if (strstr(param, "step_up_on ") == param) {
		param += strlen("step_up_on ");    // Skip 		
		cmd = DALI_CMD_STEP_UP_ON;
	}
	else if (strstr(param, "goto scene ") == param) {
		param += strlen("goto scene ");    // Skip 		
		data_tmp = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		cmd = DALI_CMD_GOTO_SCENE | (data_tmp & 0x0F);		
	}
	else if (strstr(param, "reset ") == param) {
		param += strlen("reset ");    // Skip 		
		cmd = DALI_CMD_RESET;
	}
	else if (strstr(param, "save level ") == param) {
		param += strlen("save level ");    // Skip
		cmd = DALI_CMD_SAVE_ACTUAL_LEVEL;
	}
	else if (strstr(param, "store ") == param) {
		param += strlen("store ");    // Skip 		
		if (strstr(param, "max ") == param) {
			param += strlen("max ");    // Skip
			cmd = DALI_CMD_STORE_MAX_LEVEL;
			
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*log10(intensity) + 256) / 3);
		}
		else if (strstr(param, "min ") == param) {
			param += strlen("min ");    // Skip 		
			cmd = DALI_CMD_STORE_MIN_LEVEL;
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*log10(intensity) + 256) / 3);
		}
		else if (strstr(param, "sys fail ") == param) {
			param += strlen("sys fail ");    // Skip
			cmd = DALI_CMD_STORE_SYS_FAIL_LEVEL;
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*log10(intensity) + 256) / 3);
		}
		else if (strstr(param, "on ") == param) {
			param += strlen("on ");    // Skip 		
			cmd = DALI_CMD_STORE_POWER_ON_LEVEL;
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*log10(intensity) + 256) / 3);
		}
		else if (strstr(param, "fadetime ") == param) {
			param += strlen("fadetime ");    // Skip
			cmd = DALI_CMD_STORE_MAX_LEVEL;
			
			data = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		}
		else if (strstr(param, "faderate ") == param) {
			param += strlen("faderate ");    // Skip 		
			cmd = DALI_CMD_STORE_MIN_LEVEL;
			
			data = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		}
		else if (strstr(param, "scene ") == param) {
			param += strlen("scene ");    // Skip
			data_tmp = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		
			cmd = DALI_CMD_STORE_SCENE | (data_tmp & 0x0F);					
		}
		else {
			snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
			return;
		}
		data = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space

	}
	else if (strstr(param, "remove scene ") == param) {
		param += strlen("remove scene ");    // Skip 		
		
		data_tmp = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		cmd = DALI_CMD_REMOVE_SCENE | (data_tmp & 0x0F);					
	}
	else if (strstr(param, "add group ") == param) {
		param += strlen("add group ");    // Skip 		
		cmd = DALI_CMD_ADD_GROUP;			
		data_tmp = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		cmd = DALI_CMD_ADD_GROUP | (data_tmp & 0x0F);					
	}
	else if (strstr(param, "remove group ") == param) {
		param += strlen("remove group ");    // Skip 					
		data_tmp = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		cmd = DALI_CMD_REMOVE_GROUP | (data_tmp & 0x0F);					
	}
	
	else if (strstr(param, "query ") == param) {
		param += strlen("query ");    // Skip 		
		if (strstr(param, "status ") == param) {
			param += strlen("status ");    // Skip 	
			cmd = DALI_CMD_QUERY_STATUS;
		}
		else if (strstr(param, "ballast ") == param) {
			param += strlen("ballast ");    // Skip 	
			cmd = DALI_CMD_QUERY_BALLAST;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "fail ") == param) {
			param += strlen("fail ");    // Skip 	
			cmd = DALI_CMD_QUERY_LAMP_FAIL;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "on ") == param) {
			param += strlen("on ");    // Skip 	
			cmd = DALI_CMD_QUERY_LAMP_POWER_ON;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "error ") == param) {
			param += strlen("error ");    // Skip 	
			cmd = DALI_CMD_QUERY_LIMIT_ERROR;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "reset ") == param) {
			param += strlen("reset ");    // Skip 	
			cmd = DALI_CMD_QUERY_RESET_STATE;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "no_addr ") == param) {
			param += strlen("no_addr ");    // Skip 	
			cmd = DALI_CMD_QUERY_MISS_SHORT_ADDR;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "version ") == param) {
			param += strlen("version ");    // Skip 	
			cmd = DALI_CMD_QUERY_VERSION;
		}
		else if (strstr(param, "dtr ") == param) {
			param += strlen("dtr ");    // Skip 	
			cmd = DALI_CMD_QUERY_CONTENT_DTR;
		}
		else if (strstr(param, "type ") == param) {
			param += strlen("type ");    // Skip 	
			cmd = DALI_CMD_QUERY_DEVICE_TYPE;
		}
		else if (strstr(param, "phys min ") == param) {
			param += strlen("phys min ");    // Skip 	
			cmd = DALI_CMD_QUERY_PHYS_MIN_LEVEL;
		}
		else if (strstr(param, "power fail ") == param) {
			param += strlen("power fail ");    // Skip 	
			cmd = DALI_CMD_QUERY_POWER_FAIL;
			type_rx = DALI_RX_YES_NO;
		}
		else if (strstr(param, "actual ") == param) {
			param += strlen("actual ");    // Skip 	
			cmd = DALI_CMD_QUERY_ACTUAL_LEVEL;
		}
		else if (strstr(param, "max ") == param) {
			param += strlen("max ");    // Skip 	
			cmd = DALI_CMD_QUERY_MAX_LEVEL;
		}
		else if (strstr(param, "min ") == param) {
			param += strlen("min ");    // Skip 	
			cmd = DALI_CMD_QUERY_MIN_LEVEL;
		}
		else if (strstr(param, "level on ") == param) {
			param += strlen("level on ");    // Skip 	
			cmd = DALI_CMD_QUERY_POWER_ON_LEVEL;
		}
		else if (strstr(param, "sys fail ") == param) {
			param += strlen("sys fail ");    // Skip 	
			cmd = DALI_CMD_QUERY_SYS_FAIL_LEVEL;
		}
		else if (strstr(param, "fade ") == param) {
			param += strlen("fade ");    // Skip 	
			cmd = DALI_CMD_QUERY_FADE;
		}
		else if (strstr(param, "scene level ") == param) {
			param += strlen("scene level ");    // Skip 	
			data_tmp = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			cmd = DALI_CMD_QUERY_SCENE_LEVEL | (data_tmp & 0x0F);					
		}
		else if (strstr(param, "group_7 ") == param) {
			param += strlen("group_7 ");    // Skip 	
			cmd = DALI_CMD_QUERY_GROUPS_0_7;
		}
		else if (strstr(param, "group_15 ") == param) {
			param += strlen("group_15 ");    // Skip 	
			cmd = DALI_CMD_QUERY_GROUPS_8_15;
		}
		else {
			snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
			return;
		}
	}
	else  {
		snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
		return;
	}
	
	if (strstr(param, "broadcast") == param) {
		address = DALI_BROADCAST;
		type_addr = DALI_GROUP_ADDR;
	}
	else if (strstr(param, "group ") == param) {
		param += strlen("group ");    // Skip 		
		address = strtol(param, &param, 10);
		type_addr = DALI_GROUP_ADDR;
	}
	else if (strstr(param, "dev ") == param) {
		param += strlen("dev ");    // Skip 
		address = strtol(param, &param, 10);
		type_addr = DALI_SHORT_ADDR;
	}
	else  {
		snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
		return;
	}
	
	if((cmd >= DALI_CMD_OFF) && (cmd < DALI_CMD_RESET)) {
		repeat = DALI_NOT_REPEAT;
		answer = DALI_NO_ANSWER;	
	}
	else if((cmd >= DALI_CMD_RESET) && (cmd < DALI_CMD_QUERY_STATUS)) {
		repeat = DALI_REPEAT;
		answer = DALI_NO_ANSWER;	
	}
	else if((cmd >= DALI_CMD_QUERY_STATUS) && (cmd < DALI_INIT_COMMANDS)) {
		repeat = DALI_NOT_REPEAT;
		answer = DALI_ANSWER;	
	}
	else if(cmd > DALI_INIT_COMMANDS) {
		repeat = DALI_NOT_REPEAT;
		answer = DALI_NO_ANSWER;	
	}
	
	if((cmd >= DALI_CMD_SAVE_ACTUAL_LEVEL) && (cmd <= DALI_CMD_STORE_SCENE)) {
		store_dtr = DALI_STORE;
	}
	
	address = address & DALI_BROADCAST;
	address = (type_addr << 7) + (address << 1) + (s_bit << 0);
	service = (type_rx << 3) + (answer << 2) + (repeat << 1) + (store_dtr << 0);
	
	snprintf(out, bufsize, "%02x%02x%02x%02x", cmd, service, data, address);
	
	return;
}

bool umdk_dali_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
    char strbuf[20];
	char buf_addr[10];
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
	
	uint8_t address = moddata[0];
	address = address >> 1;	
	if((address >> 6) == DALI_GROUP_ADDR) {
		address = address & DALI_BROADCAST;
		if(address == DALI_BROADCAST) {
			snprintf(buf_addr, sizeof(buf_addr), "%d", 127);
			add_value_pair(mqtt_msg, "address", buf_addr);
		}
		else if(address < DALI_BROADCAST) {
			snprintf(buf_addr, sizeof(buf_addr), "%d", address);
			add_value_pair(mqtt_msg, "group", buf_addr);
		}
	}
	else if((address >> 6) == DALI_SHORT_ADDR) {
		address = address & DALI_BROADCAST;		
		snprintf(buf_addr, sizeof(buf_addr), "%d", address);
		add_value_pair(mqtt_msg, "address", buf_addr);
	}
	
	if (moddatalen == 2) {
		if (moddata[1] == UMDK_DALI_OK_REPLY) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else if(moddata[1] == UMDK_DALI_ERROR_REPLY){
            add_value_pair(mqtt_msg, "msg", "error");
		}
		
		return true;
	}
	
	uint8_t cmd = moddata[1];
	uint8_t data = moddata[2];
	
	snprintf(strbuf, sizeof(strbuf), "%02X", cmd);
	add_value_pair(mqtt_msg, "cmd", strbuf);
	
	snprintf(buf, sizeof(buf), "%02X", data);
	add_value_pair(mqtt_msg, "data", buf);
		
	switch(cmd) {
		
		case DALI_CMD_QUERY_BALLAST:
		case DALI_CMD_QUERY_LAMP_FAIL:
		case DALI_CMD_QUERY_LAMP_POWER_ON:
		case DALI_CMD_QUERY_LIMIT_ERROR:		
		case DALI_CMD_QUERY_RESET_STATE:
		case DALI_CMD_QUERY_MISS_SHORT_ADDR:		
		case DALI_CMD_QUERY_POWER_FAIL:	{
			
			break;
		}
		
		case DALI_CMD_QUERY_STATUS:
		case DALI_CMD_QUERY_VERSION:
		case DALI_CMD_QUERY_CONTENT_DTR:
		case DALI_CMD_QUERY_DEVICE_TYPE:		
		case DALI_CMD_QUERY_PHYS_MIN_LEVEL:
		case DALI_CMD_QUERY_MAX_LEVEL:		
		case DALI_CMD_QUERY_MIN_LEVEL:
		case DALI_CMD_QUERY_POWER_ON_LEVEL:		
		case DALI_CMD_QUERY_SYS_FAIL_LEVEL:	
		case DALI_CMD_QUERY_FADE:		
		case DALI_CMD_QUERY_SCENE_LEVEL: {
			break;
		}
		case DALI_CMD_QUERY_GROUPS_0_7: {
			break;
		}
		case DALI_CMD_QUERY_GROUPS_8_15: {
			break;
		}
		case DALI_CMD_QUERY_ACTUAL_LEVEL: {
			break;
		}
	}
				
	return true;
}
