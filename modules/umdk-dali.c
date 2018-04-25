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

static const float ftime[16] = { 0.0, 0.7, 1.0, 1.4, 2.0, 2.8, 4.0, 5.7, 8.0, 11.3, 16.0, 22.6, 32.0, 45.3, 64.0, 90.5 };
static const float frate[16] = { 0.0, 357.8, 253.0, 178.9, 126.5, 89.5, 63.2, 44.7, 31.6, 22.3, 15.8, 11.2, 7.9, 5.6, 4.0, 2.8 };

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
    DALI_CMD_STORE_SCENE_MIN		= 0x40,		/* Store new scene 0 level value */
	DALI_CMD_STORE_SCENE_MAX		= 0x4F,		/* Store new scene 15 level value */
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
	DALI_CMD_QUERY_SCENE_LEVEL_MIN	= 0xB0,		/* Query scene 0 level */
	DALI_CMD_QUERY_SCENE_LEVEL_MAX	= 0xBF,		/* Query scene 15 level */
	DALI_CMD_QUERY_GROUPS_0_7		= 0xC0,		/* Query group(0-7) belonging */
	DALI_CMD_QUERY_GROUPS_8_15		= 0xC1,		/* Query group(8-15) belonging */
/*****/	
	DALI_INIT_COMMANDS		= 0xF0,

    DALI_INIT_RAND			= 0xF1,		/* Random address allocation */
	DALI_INIT_SINGLE		= 0xF2,		/* Simplified addressing method (only for one ballast separately connected) */

} dali_cmd_t;

#define UMDK_DALI_PRECISION (0.01)

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
	UMDK_DALI_WAIT_REPLY		 = 2,
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
		
		data = (uint8_t)((253*(log10(intensity) + UMDK_DALI_PRECISION) + 256) / 3);
		
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
			
			data = (uint8_t)((253*(log10(intensity) + UMDK_DALI_PRECISION) + 256) / 3);
		}
		else if (strstr(param, "min ") == param) {
			param += strlen("min ");    // Skip
			cmd = DALI_CMD_STORE_MIN_LEVEL;
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*(log10(intensity) + UMDK_DALI_PRECISION) + 256) / 3);
		}
		else if (strstr(param, "sys fail ") == param) {
			param += strlen("sys fail ");    // Skip
			cmd = DALI_CMD_STORE_SYS_FAIL_LEVEL;
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*(log10(intensity) + UMDK_DALI_PRECISION) + 256) / 3);
		}
		else if (strstr(param, "power_on ") == param) {
			param += strlen("power_on ");    // Skip
			cmd = DALI_CMD_STORE_POWER_ON_LEVEL;
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			
			data = (uint8_t)((253*(log10(intensity) + UMDK_DALI_PRECISION) + 256) / 3);
		}
		else if (strstr(param, "fadetime ") == param) {
			param += strlen("fadetime ");    // Skip
			cmd = DALI_CMD_STORE_FADE_TIME;
			
			data = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		}
		else if (strstr(param, "faderate ") == param) {
			param += strlen("faderate ");    // Skip
			cmd = DALI_CMD_STORE_FADE_RATE;
			
			data = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		}
		else if (strstr(param, "scene ") == param) {
			param += strlen("scene ");    // Skip
			
			data_tmp = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		
			cmd = DALI_CMD_STORE_SCENE_MIN | (data_tmp & 0x0F);
			
			intensity = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			data = (uint8_t)((253*(log10(intensity) + UMDK_DALI_PRECISION) + 256) / 3);
		}
		else {
			snprintf(out, bufsize, "%02x", UMDK_DALI_INVALID_CMD_REPLY);
			return;
		}
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
		else if (strstr(param, "lim error ") == param) {
			param += strlen("lim error ");    // Skip 	
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
		else if (strstr(param, "power_on ") == param) {
			param += strlen("power_on ");    // Skip
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
			
			cmd = DALI_CMD_QUERY_SCENE_LEVEL_MIN | (data_tmp & 0x0F);
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
	
	if((cmd >= DALI_CMD_SAVE_ACTUAL_LEVEL) && (cmd <= DALI_CMD_STORE_SCENE_MAX)) {
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
	int i = 0;
	
#if DALI_DEBUG	
	uint8_t ii;
    printf("[DALI] RX data[%d]:  ", moddatalen);
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
		} else if(moddata[0] == UMDK_DALI_INVALID_CMD_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		} else if(moddata[0] == UMDK_DALI_WAIT_REPLY){
			add_value_pair(mqtt_msg, "msg", "please wait");
		}
        return true;
    }	


	uint8_t cmd = moddata[0];
	uint8_t address = moddata[1];
	
	if(cmd < DALI_INIT_COMMANDS){
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
	}

	if (moddatalen == 2) {
		if (moddata[0] == UMDK_DALI_OK_REPLY) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else if(moddata[0] == UMDK_DALI_ERROR_REPLY){
            add_value_pair(mqtt_msg, "msg", "error");
		}

		return true;
	}
	
	uint8_t data = moddata[2];
	
#if DALI_DEBUG	
	printf("[DALI] cmd: 0x%02X [%d]		data: 0x%02X [%d]\n", cmd, cmd, data, data);
#endif
	
	uint8_t level = 0;
	uint8_t scene_tmp = 0;
	if((cmd >= DALI_CMD_QUERY_SCENE_LEVEL_MIN) && (cmd <= DALI_CMD_QUERY_SCENE_LEVEL_MAX)){
		scene_tmp = cmd - DALI_CMD_QUERY_SCENE_LEVEL_MIN;
		cmd = DALI_CMD_QUERY_SCENE_LEVEL_MIN;
	}
	
	switch(cmd) {

		case DALI_INIT_SINGLE: {
			snprintf(strbuf, sizeof(strbuf), "%d", data);
			add_value_pair(mqtt_msg, "init address", strbuf);
			break;
		}
		case DALI_INIT_RAND: {
			if(data == DALI_DATA_ERR) {
				add_value_pair(mqtt_msg, "init addresses", "not init");
			}
			else {
				strcat(buf, "[ ");
				for (i = -1; i < data; i++) {
					snprintf(strbuf, sizeof(strbuf), "%d, ", i);
					strcat(buf, strbuf);
				}
				strcat(buf, "]");
				add_value_pair(mqtt_msg, "init addresses", buf);
			}			
			break;
		}

		case DALI_CMD_QUERY_BALLAST: {
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "ballast", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "ballast", "no");
			}
			break;
		}
		case DALI_CMD_QUERY_LAMP_FAIL: {
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "lamp failure", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "lamp failure", "no");
			}
			break;
		}
		case DALI_CMD_QUERY_LAMP_POWER_ON: {
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "lamp operating", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "lamp operating", "no");
			}
			break;
		}
		case DALI_CMD_QUERY_LIMIT_ERROR: {
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "limit error", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "limit error", "no");
			}
			break;
		}
		case DALI_CMD_QUERY_RESET_STATE: {
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "reset state", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "reset state", "no");
			}
			break;
		}
		case DALI_CMD_QUERY_MISS_SHORT_ADDR: {
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "missing address", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "missing address", "no");
			}
			break;
		}
		case DALI_CMD_QUERY_POWER_FAIL:	{
			if(data == DALI_YES) {
				add_value_pair(mqtt_msg, "power failure", "yes");
			}
			else if(data == DALI_NO) {
				add_value_pair(mqtt_msg, "power failure", "no");
			}
			break;
		}

		case DALI_CMD_QUERY_STATUS: {
			if(((data >> 7) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "power failure", "yes");
			}
			else {
				add_value_pair(mqtt_msg, "power failure", "no");
			}
			if(((data >> 6) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "missing address", "yes");
			}
			else {
				add_value_pair(mqtt_msg, "missing address", "no");
			}
			if(((data >> 5) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "reset state", "yes");
			}
			else {
				add_value_pair(mqtt_msg, "reset state", "no");
			}
			if(((data >> 4) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "fade", "running");
			}
			else {
				add_value_pair(mqtt_msg, "fade", "ready");
			}
			if(((data >> 3) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "limit error", "yes");
			}
			else {
				add_value_pair(mqtt_msg, "limit error", "no");
			}
			if(((data >> 2) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "power", "on");
			}
			else {
				add_value_pair(mqtt_msg, "power", "off");
			}
			if(((data >> 1) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "lamp failure", "yes");
			}
			else {
				add_value_pair(mqtt_msg, "lamp failure", "no");
			}
			if(((data >> 0) & 0x01) == 1) {
				add_value_pair(mqtt_msg, "ballast", "no");
			}
			else {
				add_value_pair(mqtt_msg, "ballast", "yes");
			}
			break;
		}
		case DALI_CMD_QUERY_VERSION: {
			snprintf(buf, sizeof(buf), "%02X", data);
			add_value_pair(mqtt_msg, "version", buf);
			break;
		}
		case DALI_CMD_QUERY_CONTENT_DTR: {
			snprintf(buf, sizeof(buf), "%d", data);
			add_value_pair(mqtt_msg, "content", buf);
			break;
		}
		case DALI_CMD_QUERY_DEVICE_TYPE: {
			if(data == 0) {
				add_value_pair(mqtt_msg, "type", "fluorescent lamp");
			}
			else if(data == 1) {
				add_value_pair(mqtt_msg, "type", "emergency lighting");
			}
			else if(data == 2) {
				add_value_pair(mqtt_msg, "type", "HID lamp");
			}
			else if(data == 3) {
				add_value_pair(mqtt_msg, "type", "dimming bulb");
			}
			else if(data == 4) {
				add_value_pair(mqtt_msg, "type", "incandescent lamp");
			}
			else if(data == 5) {
				add_value_pair(mqtt_msg, "type", "digital signal");
			}
			else if(data == 6) {
				add_value_pair(mqtt_msg, "type", "LED");
			}
			else {
				snprintf(buf, sizeof(buf), "%d", data);
				add_value_pair(mqtt_msg, "type", buf);
			}
			break;
		}
		case DALI_CMD_QUERY_PHYS_MIN_LEVEL: {
			snprintf(buf, sizeof(buf), "%d", data);
			add_value_pair(mqtt_msg, "physical min", buf);
			break;
		}
		case DALI_CMD_QUERY_MAX_LEVEL: {
			level = (uint8_t)pow(10.0, ((3*((float)data) - 256) / 253));
			snprintf(buf, sizeof(buf), "%d", level);
			add_value_pair(mqtt_msg, "max level", buf);
			break;
		}
		case DALI_CMD_QUERY_MIN_LEVEL: {
			level = (uint8_t)pow(10.0, ((3*((float)data) - 256) / 253));
			snprintf(buf, sizeof(buf), "%d", level);
			add_value_pair(mqtt_msg, "min level", buf);
			break;
		}
		case DALI_CMD_QUERY_POWER_ON_LEVEL: {
			level = (uint8_t)pow(10.0, ((3*((float)data) - 256) / 253));
			snprintf(buf, sizeof(buf), "%d", level);
			add_value_pair(mqtt_msg, "power on level", buf);
			break;
		}
		case DALI_CMD_QUERY_SYS_FAIL_LEVEL: {
			level = (uint8_t)pow(10.0, ((3*((float)data) - 256) / 253));
			snprintf(buf, sizeof(buf), "%d", level);
			add_value_pair(mqtt_msg, "system failure level", buf);
			break;
		}
		case DALI_CMD_QUERY_FADE: {
			uint8_t fadetime = (data >> 4) & 0x0F;
			uint8_t faderate = data & 0x0F;
			char str_time[10] = { 0 };
			char str_rate[10] = { 0 };
			
			if(fadetime > 0){
				snprintf(str_time, sizeof(str_time), "%.1f", ftime[fadetime]);
			}
			else {
				snprintf(str_time, sizeof(str_time), "<0.7");
			}
			add_value_pair(mqtt_msg, "fadetime", str_time);
			
			if(faderate > 0){
				snprintf(str_rate, sizeof(str_rate), "%.1f", frate[faderate]);
			}
			else {
				snprintf(str_rate, sizeof(str_rate), "not applicable");
			}
			add_value_pair(mqtt_msg, "faderate", str_rate);
			break;
		}
		case DALI_CMD_QUERY_SCENE_LEVEL_MIN: {
			snprintf(buf, sizeof(buf), "%d scene level", scene_tmp);
			
			if(data == DALI_DATA_ERR) {
				add_value_pair(mqtt_msg, buf, "not set");
			}
			else {
				level = (uint8_t)pow(10.0, ((3*((float)data) - 256) / 253));
				snprintf(strbuf, sizeof(strbuf), "%d", level);
				add_value_pair(mqtt_msg, buf, strbuf);
			}
			break;
		}
		case DALI_CMD_QUERY_GROUPS_0_7: {
			if(data == 0) {
				add_value_pair(mqtt_msg, "groups 0-7", "not belong");
			}
			else {
				strcat(buf, "[ ");
				for (i = 0; i < 8; i++) {
					if(data & (1 << i)) {
						snprintf(strbuf, sizeof(strbuf), "%d, ", i);
						strcat(buf, strbuf);
					}
				}
				strcat(buf, "]");
				add_value_pair(mqtt_msg, "groups 0-7", buf);
			}
			break;
		}
		case DALI_CMD_QUERY_GROUPS_8_15: {
			if(data == 0) {
				add_value_pair(mqtt_msg, "groups 8-15", "not belong");
			}
			else {
				strcat(buf, "[ ");
				for (i = 0; i < 8; i++) {
					if(data & (1 << i)) {
						snprintf(strbuf, sizeof(strbuf), "%d, ", i + 8);
						strcat(buf, strbuf);
					}
				}
				strcat(buf, "]");
				add_value_pair(mqtt_msg, "groups 8-15", buf);
			}
			break;
		}
		case DALI_CMD_QUERY_ACTUAL_LEVEL: {
			if(data == DALI_DATA_ERR) {
				add_value_pair(mqtt_msg, "actual level", "error");
			}
			else {
				level = (uint8_t)pow(10.0, ((3*((float)data) - 256) / 253) + UMDK_DALI_PRECISION);
				snprintf(buf, sizeof(buf), "%d", level);
				add_value_pair(mqtt_msg, "actual level", buf);			
			}
			break;
		}
	}

	return true;
}
