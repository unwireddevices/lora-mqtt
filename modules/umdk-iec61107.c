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

#define IEC61107_BRACKET_OPEN 0x28 /* '(' */
#define IEC61107_BRACKET_CLOSE 0x29 /* ')' */
#define	IEC61107_WRITE 0x57	/* 'W' - Write */
#define	IEC61107_READ 0x52	/* 'R' - Read */

#define IEC61107_MODE_SP_READ 0x36

typedef enum {   
    UMDK_IEC61107_CMD_DATABASE_RESET 		= 0xFE,		/* Clear database */
    UMDK_IEC61107_CMD_DATABASE_ADD 			= 0xFD,		/* Add address  in database */
    UMDK_IEC61107_CMD_DATABASE_REMOVE 		= 0xFC,		/* Remove address from database */
	UMDK_IEC61107_CMD_DATABASE_FIND 		= 0xFB,		/* Find address in database */

    IEC61107_CMD_PROPRIETARY_COMMAND 	= 0xF0,		/* Less this value - single command of CE102M */

    IEC61107_CMD_SPECIFIC				= 0x00,		/* Manufacturer Special cmd */
	
	IEC61107_CMD_OPTIONS				= 0x01,		/* Set programming mode without DSTP-button */
    IEC61107_CMD_TIME					= 0x02,		/* Read/write the internal time */
    IEC61107_CMD_DATE					= 0x03,		/* Read/write the internal date */
    IEC61107_CMD_SERIAL					= 0x04,		/* Read/write serial number */
	IEC61107_CMD_ID_DEV					= 0x05,		/* Read/write id of the device */
	IEC61107_CMD_STATUS					= 0x06,		/* Read device status */
		
    IEC61107_CMD_GET_VALUE_TOTAL_ALL	= 0x07,		/* Read the total values of energy after reset */
	IEC61107_CMD_GET_VALUE_TOTAL_MONTH	= 0x08,		/* Read the total values of monthly energy */
    IEC61107_CMD_GET_VALUE_TOTAL_DAY	= 0x09,		/* Read the total values of daily energy */
    IEC61107_CMD_GET_VALUE_END_MONTH	= 0x0A,		/* Read the values of monthly energy */
    IEC61107_CMD_GET_VALUE_END_DAY		= 0x0B,		/* Read the values of daily energy */ 
    IEC61107_CMD_GET_VALUE_DATE_MONTH	= 0x0C,		/* Read the date array of monthly energy savings */
    IEC61107_CMD_GET_VALUE_DATE_DAY		= 0x0D,		/* Read the date array of daily energy savings */
	IEC61107_CMD_ERASAE_VALUES			= 0x0E,		/* Erase all energy  values */
	
	IEC61107_CMD_GET_VOLT				= 0x0F,		/* Read the voltage value */
	IEC61107_CMD_GET_CURR				= 0x10,		/* Read the current value */
	IEC61107_CMD_GET_POWER				= 0x11,		/* Read the power value */
	
	IEC61107_CMD_SCHEDULE				= 0x12,		/* Read/write schedule ot tariffs */
	IEC61107_CMD_HOLIDAYS				= 0x13,		/* Read/write list of holidays */
	
	IEC61107_CMD_TARIFF_DEFAULT			= 0x14,		/* Set/get default tariff */	
	// IEC61107_CMD_GET_SAVING_END_MONTH	= 0x14,
} umdk_iec61107_cmd_t;

typedef enum {
	NONE	= 0,
	ADDRESS = 1,
	DEVICE	= 2,
	FREE	= 3,
} iec61107_database_param_t;

typedef enum {
	UMDK_IEC61107_ERROR_REPLY 		= 0,
    UMDK_IEC61107_OK_REPLY 			= 1,
    UMDK_IEC61107_NO_RESPONSE_REPLY 	= 2,
	UMDK_IEC61107_INVALID_CMD_REPLY 	= 0xFF,
} iec61107_reply_t;

static char str_dow[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

void umdk_iec61107_command(char *param, char *out, int bufsize) {
	uint8_t cmd = 0;		
	uint8_t mode = 0;
	uint8_t zz_param = 0;

	iec61107_database_param_t param_database = NONE;
	uint8_t device = 0;
	char *address_ptr = NULL;
	uint8_t symb_addr = 0;
	uint8_t i = 0;
	uint8_t length_addr = 0;
	uint16_t num_char = 0;
	
	uint8_t tariff_hour = 0;
	uint8_t min = 0;
	
	uint16_t date = 0;
	
	if (strstr(param, "reset") == param) {
		cmd = UMDK_IEC61107_CMD_DATABASE_RESET;
		snprintf(out, bufsize, "%02x", cmd);
		return;
	}
	else if (strstr(param, "add ") == param) {
		param += strlen("add ");    // Skip 
		cmd = UMDK_IEC61107_CMD_DATABASE_ADD;
		param_database = NONE;
		device = 0;
		address_ptr = param;
		length_addr = (uint8_t)strlen(address_ptr);

		num_char = snprintf(out, bufsize, "%02x%02x%02x", cmd, param_database, device);
		for(i = 0; i < length_addr; i++) {
			symb_addr = *address_ptr;
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x", symb_addr);
			address_ptr++;
		}			
		return;		
	}
	else if (strstr(param, "remove ") == param) {
		param += strlen("remove ");    // Skip 
		cmd = UMDK_IEC61107_CMD_DATABASE_REMOVE;
		if (strstr(param, "device ") == param) {
			param += strlen("device ");    // Skip 
			param_database = DEVICE;			
			device = strtol(param, &param, 10);
			symb_addr = 0;
			snprintf(out, bufsize, "%02x%02x%02x%02x", cmd, param_database, device, symb_addr);
		}
		else if (strstr(param, "address ") == param) {
			param += strlen("address ");    // Skip 
			param_database = ADDRESS;
			device = 0;
			
			address_ptr = param;
			length_addr = (uint8_t)strlen(address_ptr);

			num_char = snprintf(out, bufsize, "%02x%02x%02x", cmd, param_database, device);
			for(i = 0; i < length_addr; i++) {
				symb_addr = *address_ptr;
				num_char += snprintf(out + num_char, bufsize - num_char, "%02x", symb_addr);
				address_ptr++;
			}						
		}
		else  {
			snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
		}
		return;
	}
	else if (strstr(param, "find ") == param) {
		param += strlen("find ");    // Skip 
		cmd = UMDK_IEC61107_CMD_DATABASE_FIND;
		if (strstr(param, "device ") == param) {
			param += strlen("device ");    // Skip 
			param_database = DEVICE;			
			device = strtol(param, &param, 10);
			symb_addr = 0;
			snprintf(out, bufsize, "%02x%02x%02x%02x", cmd, param_database, device, symb_addr);
		}
		else if (strstr(param, "address ") == param) {
			param += strlen("address ");    // Skip 
			param_database = ADDRESS;
			device = 0;
			
			address_ptr = param;
			length_addr = (uint8_t)strlen(address_ptr);

			num_char = snprintf(out, bufsize, "%02x%02x%02x", cmd, param_database, device);
			for(i = 0; i < length_addr; i++) {
				symb_addr = *address_ptr;
				num_char += snprintf(out + num_char, bufsize - num_char, "%02x", symb_addr);
				address_ptr++;
			}						
		}
		else  {
			snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
		}
		return;
	}
	else if (strstr(param, "set init") == param) {
		cmd = IEC61107_CMD_OPTIONS;
		mode = IEC61107_WRITE;
	}
	else if (strstr(param, "get init") == param) {
		param += strlen("get init");    // Skip 
		cmd = IEC61107_CMD_OPTIONS;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get serial") == param) {
		param += strlen("get serial");    // Skip 
		cmd = IEC61107_CMD_SERIAL;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get id_device") == param) {
		param += strlen("get id_device");    // Skip 
		cmd = IEC61107_CMD_ID_DEV;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get volt") == param) {
		param += strlen("get volt");    // Skip 
		cmd = IEC61107_CMD_GET_VOLT;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get current") == param) {
		param += strlen("get current");    // Skip 
		cmd = IEC61107_CMD_GET_CURR;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get power") == param) {
		param += strlen("get power");    // Skip 
		cmd = IEC61107_CMD_GET_POWER;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get time") == param) { 
		param += strlen("get time");    // Skip command
		cmd = IEC61107_CMD_TIME;	
		mode = IEC61107_READ;		
	}
	else if (strstr(param, "set time") == param) { 
		param += strlen("get time");    // Skip command
		cmd = IEC61107_CMD_TIME;	
		mode = IEC61107_WRITE;		
	}	
	else if (strstr(param, "get date") == param) { 
		param += strlen("get date");    // Skip command
		cmd = IEC61107_CMD_DATE;	
		mode = IEC61107_READ;		
	}
	else if (strstr(param, "set date") == param) { 
		param += strlen("set date");    // Skip command
		cmd = IEC61107_CMD_DATE;	
		mode = IEC61107_WRITE;		
	}
	else if (strstr(param, "get value ") == param) { 
		param += strlen("get value ");    // Skip command
		mode = IEC61107_READ;
		if(strstr(param, "total_sp") == param) {
			param += strlen("total_sp");				// Skip command
			cmd = IEC61107_CMD_SPECIFIC;
			mode = IEC61107_MODE_SP_READ;
		}
		else if(strstr(param, "total_all") == param) { 
			param += strlen("total_all");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_TOTAL_ALL;
		}
		else if(strstr(param, "total_month") == param) { 
			param += strlen("total_month");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_TOTAL_MONTH;
		}
		else if(strstr(param, "total_day") == param) { 
			param += strlen("total_day");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_TOTAL_DAY;
		}
		else if(strstr(param, "end_month") == param) { 
			param += strlen("end_month");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_END_MONTH;
		}
		else if(strstr(param, "end_day") == param) { 
			param += strlen("end_day");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_END_DAY;
		}	
		else {
			snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
			return;
		}
	}
	else if (strstr(param, "get schedule") == param) { 
		param += strlen("get schedule");    // Skip command
		cmd = IEC61107_CMD_SCHEDULE;	
		mode = IEC61107_READ;		
		param += strlen(" ");    						// Skip space
		zz_param = strtol(param, &param, 10);
	}
	else if (strstr(param, "set schedule") == param) { 
		param += strlen("set schedule");    // Skip command
		cmd = IEC61107_CMD_SCHEDULE;	
		mode = IEC61107_WRITE;		
		param += strlen(" ");    						// Skip space
		zz_param = strtol(param, &param, 10);			
	}
	else if (strstr(param, "get holidays") == param) { 
		param += strlen("get holidays");    // Skip command
		cmd = IEC61107_CMD_HOLIDAYS;	
		mode = IEC61107_READ;		
	}
	else if (strstr(param, "set holidays") == param) { 
		param += strlen("set holidays");    // Skip command
		cmd = IEC61107_CMD_HOLIDAYS;	
		mode = IEC61107_WRITE;		
		param += strlen(" ");    						// Skip space
		if(strstr(param, "part_1") == param) { 
			param += strlen("part_1");    // Skip command
			zz_param = 1;
		}
		else if(strstr(param, "part_2") == param) { 
			param += strlen("part_");    // Skip command
			zz_param = 2;
		}
		else {
			snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
			return;
		}
	}
	else {
		snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
		return;
	}
		
	param += strlen(" ");    						// Skip space
		
	char *parametr_ptr = param;
	uint8_t symb_param = 0;
	uint8_t length_total = 0;
	uint8_t length_param = 0;
		
	length_total = (uint8_t)strlen(parametr_ptr);
	length_param = length_total;
	
	if(length_total < 2) {
		snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_CMD_REPLY);
		return;				
	}
	
	address_ptr = parametr_ptr + length_total - 1;
	while(*address_ptr != ' '){
		address_ptr--;
		length_param--;
	}
	 
	address_ptr++;	
	
	if(address_ptr != parametr_ptr) {
		length_param--;
	}
	
	device = strtol(address_ptr, &address_ptr, 10);
	
	if(cmd == IEC61107_CMD_OPTIONS) {
		length_param = 5;
		parametr_ptr = "18290";
	}
	
	num_char = snprintf(out, bufsize, "%02x%02x%02x%02x", cmd, mode, zz_param, device);
	
	num_char += snprintf(out + num_char, bufsize - num_char, "%02x", IEC61107_BRACKET_OPEN);
	
	for(i = 0; i < length_param; i++) {
		if((cmd == IEC61107_CMD_SCHEDULE) && (mode == IEC61107_WRITE)) {
			tariff_hour = strtol(parametr_ptr, &parametr_ptr, 10) << 5;
			parametr_ptr += strlen(" ");    						// Skip space
			tariff_hour |= strtol(parametr_ptr, &parametr_ptr, 10);
			parametr_ptr += strlen(" ");    						// Skip space
			min = strtol(parametr_ptr, &parametr_ptr, 10);
			parametr_ptr += strlen(" ");    						// Skip space
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x%02x", tariff_hour, min);			
			i += 8;
		}
		else if((cmd == IEC61107_CMD_HOLIDAYS) && (mode == IEC61107_WRITE)) {
			date = strtol(parametr_ptr, &parametr_ptr, 10) << 6;	// Day
			parametr_ptr += strlen(" ");    						// Skip space
			date |= strtol(parametr_ptr, &parametr_ptr, 10) << 12;	// Month
			parametr_ptr += strlen(" ");    						// Skip space
			date |= strtol(parametr_ptr, &parametr_ptr, 10) << 0;	// Schedule			
			parametr_ptr += strlen(" ");    						// Skip space
			printf("[IEC61107]: %04X ", date);
			num_char += snprintf(out + num_char, bufsize - num_char, "%04x", date);			
			i += 8;
		}
		else {
			symb_param = *parametr_ptr;
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x", symb_param);
			parametr_ptr++;
		}
	}
	
	num_char += snprintf(out + num_char, bufsize - num_char, "%02x", IEC61107_BRACKET_CLOSE);	
		
}

bool umdk_iec61107_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
    char strbuf[20];
	char buf_addr[10];
	uint16_t num_char = 0;
	uint16_t i = 0;
	uint8_t symbol = 0;
		
	// uint8_t ii;
    // printf("[iec61107] RX data:  ");
    // for(ii = 0; ii < moddatalen; ii++) {
        // printf(" %02X ", moddata[ii]);
    // }
   // puts("\n");
	
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
	
	umdk_iec61107_cmd_t cmd = moddata[0];	
	uint8_t device = moddata[1];
	
	if(cmd < IEC61107_CMD_PROPRIETARY_COMMAND) {	
		snprintf(buf_addr, sizeof(buf_addr), "%02d", device);
		add_value_pair(mqtt_msg, "device", buf_addr);
		
								
		if (moddatalen == 2) {
			if (moddata[0] == UMDK_IEC61107_OK_REPLY) {
				add_value_pair(mqtt_msg, "msg", "ok");
			} else if(moddata[0] == UMDK_IEC61107_ERROR_REPLY){
				add_value_pair(mqtt_msg, "msg", "error");
			} else if(moddata[0] == UMDK_IEC61107_NO_RESPONSE_REPLY){
				add_value_pair(mqtt_msg, "msg", "no response");					
			}
			return true;
		}	
	}
	else {
		if(cmd == UMDK_IEC61107_CMD_DATABASE_ADD) {
			add_value_pair(mqtt_msg, "cmd", "added");
		}
		else if(cmd == UMDK_IEC61107_CMD_DATABASE_REMOVE) {
			add_value_pair(mqtt_msg, "cmd", "removed");
		}
		else if(cmd == UMDK_IEC61107_CMD_DATABASE_FIND) {
			add_value_pair(mqtt_msg, "cmd", "found");
		}
		
		snprintf(buf_addr, sizeof(buf_addr), "%02d", device);
		add_value_pair(mqtt_msg, "device", buf_addr);	
		
		uint8_t *address_ptr = moddata + 2;	
		uint16_t num_char = 0;

		for(i = 0; i < (moddatalen - 2); i++) {
			symbol = *address_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			address_ptr++;
		}
		add_value_pair(mqtt_msg, "address", buf);		
		return true;		
	}
 
	if(cmd == IEC61107_CMD_TIME) {
		char time_buf[10] = { };
		
		uint8_t hour = ( moddata[2] - 0x30) * 10 + ( moddata[3] - 0x30);
		uint8_t min =  ( moddata[4] - 0x30) * 10 + ( moddata[5] - 0x30);
		uint8_t sec =  ( moddata[6] - 0x30) * 10 + ( moddata[7] - 0x30);
				
		
		snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", hour, min, sec);	
		add_value_pair(mqtt_msg, "time", time_buf);
		
	}
	else if(cmd == IEC61107_CMD_DATE) {
		char date_buf[15] = { };
		
		uint8_t dow = ( moddata[2] - 0x30) * 10 + ( moddata[3] - 0x30);
		
		uint8_t day = ( moddata[4] - 0x30) * 10 + ( moddata[5] - 0x30);
		uint8_t month =  ( moddata[6] - 0x30) * 10 + ( moddata[7] - 0x30);
		uint8_t year =  ( moddata[8] - 0x30) * 10 + ( moddata[9] - 0x30);

		add_value_pair(mqtt_msg, "day", str_dow[dow]);			
		
		snprintf(date_buf, sizeof(date_buf), "%02d/%02d/%02d", day, month, year);	
		add_value_pair(mqtt_msg, "date", date_buf);
	}				
	else if(cmd == IEC61107_CMD_SERIAL) {
		uint8_t *serial_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *serial_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			serial_ptr++;
		}
	
		add_value_pair(mqtt_msg, "serial", buf);
	}
	else if(cmd == IEC61107_CMD_ID_DEV) {
		uint8_t *serial_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *serial_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			serial_ptr++;
		}
	
		add_value_pair(mqtt_msg, "id device", buf);
	}			
	else if(cmd == IEC61107_CMD_GET_VOLT) {
		uint8_t *volt_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *volt_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			volt_ptr++;
		}
	
		add_value_pair(mqtt_msg, "voltage", buf);
	}			
	else if(cmd == IEC61107_CMD_GET_CURR) {
		uint8_t *curr_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *curr_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			curr_ptr++;
		}
	
		add_value_pair(mqtt_msg, "current", buf);
	}			
	else if(cmd == IEC61107_CMD_GET_POWER) {
		uint8_t *power_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *power_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			power_ptr++;
		}
	
		add_value_pair(mqtt_msg, "power", buf);
	}		
	else if(cmd == IEC61107_CMD_SCHEDULE) {				
		if((moddata[2] == 0) && (moddata[3] == 0)) {
			add_value_pair(mqtt_msg, "schedule", "not set");
		}
		else {
			char tariff_str[5] = { };
			uint8_t num_schedule = (moddatalen  - 2) / 2;			
			uint8_t hour = 0;
			uint8_t min = 0;
			uint8_t tariff = 0;
			
			for(i = 0; i < num_schedule; i++) {
				tariff = moddata[2 + 2*i] >> 5;
				if(tariff != 0) {
					hour = moddata[2 + 2*i] & 0x1F;
					min = moddata[3 + 2*i];
					snprintf(tariff_str, sizeof(tariff_str), "T%02d", tariff);		
					snprintf(buf, sizeof(buf), "%02d:%02d",  hour, min);
					add_value_pair(mqtt_msg, tariff_str, buf);			
				}
			}
		}		
	}			
	else if(cmd == IEC61107_CMD_HOLIDAYS) {
		
	}			
	else if((cmd >= IEC61107_CMD_GET_VALUE_TOTAL_ALL) && (cmd <= IEC61107_CMD_GET_VALUE_END_DAY)) {
		uint32_t value[5] = { 0 };
		uint32_t * ptr_value;
		for(i = 0; i < 5; i++) {
			ptr_value = (uint32_t *)(&moddata[4*i + 2]);
			uint32_to_le(ptr_value);
			value[i] = *ptr_value;
		}
				
		char tariff[5] = { };
		for(i = 1; i < 5; i++) {
			snprintf(tariff, sizeof(tariff), "T%02d", i);
			int_to_float_str(strbuf, value[i], 2);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, tariff, buf);								
		}
		int_to_float_str(strbuf, value[0], 2);
		snprintf(buf, sizeof(buf), "%s", strbuf);
		add_value_pair(mqtt_msg, "Total", buf);		
	}				
				
	return true;
}
