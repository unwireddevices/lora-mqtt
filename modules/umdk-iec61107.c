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

#define IEC61107_DEBUG 0

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
	
	IEC61107_ERROR_PROTOCOL_DEVICE      = 0xEF,		/* Returning error from device */

    IEC61107_CMD_SPECIFIC				= 0x00,		/* Manufacturer Special cmd */
	
	IEC61107_CMD_OPTIONS				= 0x01,		/* Set programming mode without DSTP-button */
    IEC61107_CMD_TIME					= 0x02,		/* Read/write the internal time */
    IEC61107_CMD_DATE					= 0x03,		/* Read/write the internal date */
    IEC61107_CMD_SERIAL					= 0x04,		/* Read/write serial number */
	IEC61107_CMD_ID_DEV					= 0x05,		/* Read/write id of the device */
	IEC61107_CMD_STATUS					= 0x06,		/* Read device status */
		
    IEC61107_CMD_GET_VALUE_TOTAL_ALL	= 0x07,		/* Read the total values of energy after reset */
	IEC61107_CMD_GET_VALUE_MONTH		= 0x08,		/* Read the values of monthly energy */
    IEC61107_CMD_GET_VALUE_DAY			= 0x09,		/* Read the values of daily energy */
    IEC61107_CMD_GET_VALUE_TOTAL_MONTH	= 0x0A,		/* Read the total values of monthly energy */
    IEC61107_CMD_GET_VALUE_TOTAL_DAY	= 0x0B,		/* Read the total values of daily energy */
	
	IEC61107_CMD_GET_VOLT				= 0x0C,		/* Read the voltage value */
	IEC61107_CMD_GET_CURR				= 0x0D,		/* Read the current value */
	IEC61107_CMD_GET_POWER				= 0x0E,		/* Read the power value */
	
	IEC61107_CMD_SCHEDULE				= 0x0F,		/* Read/write schedule ot tariffs */
	IEC61107_CMD_HOLIDAYS				= 0x10,		/* Read/write list of holidays */	
	IEC61107_CMD_TARIFF_DEFAULT			= 0x11,		/* Set/get default tariff */	
} umdk_iec61107_cmd_t;

typedef enum {
	NONE	= 0,
	ADDRESS = 1,
	DEVICE	= 2,
	FREE	= 3,
} iec61107_database_param_t;

typedef enum {
    UMDK_IEC61107_ERROR_REPLY        	= 0,
    UMDK_IEC61107_OK_REPLY           	= 1,
    UMDK_IEC61107_NO_RESPONSE_REPLY  	= 2,
	UMDK_IEC61107_WAIT_REPLY		 	= 3,
	UMDK_IEC61107_INVALID_FORMAT_REPLY	= 4,
    UMDK_IEC61107_INVALID_CMD_REPLY  	= 0xFF,
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
	
	uint8_t date_tmp1 = 0, date_tmp2 = 0;
	uint8_t day = 0, month = 0, sch = 0;
	
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
		zz_param = 0;
	}
	else if (strstr(param, "set fin") == param) {
		cmd = IEC61107_CMD_OPTIONS;
		mode = IEC61107_WRITE;
		zz_param =  1;
	}
	else if (strstr(param, "get init") == param) {
		param += strlen("get init");    // Skip 
		cmd = IEC61107_CMD_OPTIONS;
		mode = IEC61107_READ;
	}
	else if (strstr(param, "get status") == param) {
		param += strlen("get status");    // Skip 
		cmd = IEC61107_CMD_STATUS;
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
	else if (strstr(param, "get default_tariff") == param) { 
		param += strlen("get default_tariff");    // Skip command
		cmd = IEC61107_CMD_TARIFF_DEFAULT;	
		mode = IEC61107_READ;		
	}
	else if (strstr(param, "set default_tariff") == param) { 
		param += strlen("set default_tariff");    // Skip command
		cmd = IEC61107_CMD_TARIFF_DEFAULT;	
		mode = IEC61107_WRITE;		
	}	
	else if (strstr(param, "get special_cmd") == param) { 
		param += strlen("get special_cmd");    // Skip command
		cmd = IEC61107_CMD_SPECIFIC;
		mode = IEC61107_MODE_SP_READ;
	}
	else if (strstr(param, "get value ") == param) { 
		param += strlen("get value ");    // Skip command
		mode = IEC61107_READ;
		if(strstr(param, "total_all") == param) { 
			param += strlen("total_all");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_TOTAL_ALL;
		}
		else if(strstr(param, "month") == param) { 
			param += strlen("month");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_MONTH;
		}
		else if(strstr(param, "day") == param) { 
			param += strlen("day");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_DAY;
		}
		else if(strstr(param, "total_month") == param) { 
			param += strlen("total_month");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_TOTAL_MONTH;
		}
		else if(strstr(param, "total_day") == param) { 
			param += strlen("total_day");    // Skip command
			cmd = IEC61107_CMD_GET_VALUE_TOTAL_DAY;
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
			param += strlen("part_2");    // Skip command
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
		snprintf(out, bufsize, "%02x", UMDK_IEC61107_INVALID_FORMAT_REPLY);
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
		if(zz_param == 0) {
			parametr_ptr = "18290";
		}
		else {
			parametr_ptr = "18288";
		}
		zz_param = 0;
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
			
			day = strtol(parametr_ptr, &parametr_ptr, 10);	// Day
			parametr_ptr += strlen(" ");    						// Skip space
			month = strtol(parametr_ptr, &parametr_ptr, 10);	// Month
			parametr_ptr += strlen(" ");    						// Skip space
			sch = strtol(parametr_ptr, &parametr_ptr, 10);	// Schedule			
			parametr_ptr += strlen(" ");    						// Skip space
			
			date_tmp1 = (month << 4) + (day >> 2);
			date_tmp2 = (sch) + ((day & 0x03) << 6);

			num_char += snprintf(out + num_char, bufsize - num_char, "%02x%02x", date_tmp1, date_tmp2);			
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
	uint8_t * data_ptr = NULL;
	
#if IEC61107_DEBUG	
	uint8_t ii;
    printf("[iec61107] RX data:  ");
    for(ii = 0; ii < moddatalen; ii++) {
        printf(" %02X ", moddata[ii]);
    }
   puts("\n");
#endif
	
   if (moddatalen == 1) {
        if (moddata[0] == UMDK_IEC61107_OK_REPLY) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else if(moddata[0] == UMDK_IEC61107_ERROR_REPLY){
            add_value_pair(mqtt_msg, "msg", "error");
		} else if(moddata[0] == UMDK_IEC61107_INVALID_CMD_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		} else if(moddata[0] == UMDK_IEC61107_INVALID_FORMAT_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid format");
		}
        return true;
    }	
	
	umdk_iec61107_cmd_t cmd = moddata[0];	
	uint8_t device = moddata[1];
	
	if(cmd < IEC61107_CMD_PROPRIETARY_COMMAND) {

		snprintf(buf_addr, sizeof(buf_addr), "%d", device);
		add_value_pair(mqtt_msg, "device", buf_addr);
										
		if (moddatalen == 2) {
			if (moddata[0] == UMDK_IEC61107_OK_REPLY) {
				add_value_pair(mqtt_msg, "msg", "ok");
			} else if(moddata[0] == UMDK_IEC61107_ERROR_REPLY){
				add_value_pair(mqtt_msg, "msg", "error");
			} else if(moddata[0] == UMDK_IEC61107_NO_RESPONSE_REPLY){
				add_value_pair(mqtt_msg, "msg", "no response");					
			} else if(moddata[0] == UMDK_IEC61107_WAIT_REPLY){
				add_value_pair(mqtt_msg, "msg", "please wait");					
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
		
		snprintf(buf_addr, sizeof(buf_addr), "%d", device);
		add_value_pair(mqtt_msg, "device", buf_addr);
		
		uint8_t *address_ptr = moddata + 2;	
		
		for(i = 0; i < (moddatalen - 2); i++) {
			symbol = *address_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			address_ptr++;
		}
		add_value_pair(mqtt_msg, "address", buf);		
		return true;		
	}
 
	if(cmd == IEC61107_CMD_TIME) {
		data_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "time", buf);				
	}
	else if(cmd == IEC61107_CMD_DATE) {
		uint8_t dow = ( moddata[2] - 0x30) * 10 + ( moddata[3] - 0x30);
		data_ptr = moddata + 5;	
		add_value_pair(mqtt_msg, "day", str_dow[dow]);			
			
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "date", buf);
	}				
	else if(cmd == IEC61107_CMD_SERIAL) {
		data_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "serial", buf);
	}
	else if(cmd == IEC61107_CMD_ID_DEV) {
		data_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "id device", buf);
	}			
	else if(cmd == IEC61107_CMD_GET_VOLT) {
		data_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "voltage", buf);
	}			
	else if(cmd == IEC61107_CMD_GET_CURR) {
		data_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "current", buf);
	}			
	else if(cmd == IEC61107_CMD_GET_POWER) {
		data_ptr = moddata + 2;	
		for(i = 2; i < moddatalen; i++) {
			symbol = *data_ptr;
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%c", symbol);
			data_ptr++;
		}
	
		add_value_pair(mqtt_msg, "power", buf);
	}		
	else if(cmd == IEC61107_CMD_SCHEDULE) {				
		char tariff_str[5] = { };
		uint8_t num_schedule = (moddatalen  - 2) / 2;			
		uint8_t hour = 0;
		uint8_t min = 0;
		uint8_t tariff = 0;
		
		for(i = 0; i < num_schedule; i++) {
			tariff = moddata[2 + 2*i] >> 5;
			hour = moddata[2 + 2*i] & 0x1F;
			min = moddata[3 + 2*i];
			snprintf(tariff_str, sizeof(tariff_str), "T%02d", tariff);		
			snprintf(buf, sizeof(buf), "%02d:%02d",  hour, min);
			add_value_pair(mqtt_msg, tariff_str, buf);			
		}	
		
		for(i = num_schedule; i < 12; i++) {
			add_value_pair(mqtt_msg, "T00", "00:00");			
		}	
	}			
	else if(cmd == IEC61107_CMD_HOLIDAYS) {
		uint8_t list = moddata[2];
		
		char list_str[5] = { };
		char schedule_str[10] = { };
		uint8_t num_holidays = (moddatalen  - 3) / 2;			
		uint8_t day = 0;
		uint8_t month = 0;
		uint8_t schedule = 0;
		
		snprintf(list_str, sizeof(list_str), "%d/%d", list >> 4, list & 0x0F);		
		add_value_pair(mqtt_msg, "holidays list", list_str);		
		
		for(i = 0; i < num_holidays; i++) {
			month = moddata[3 + 2*i] >> 4;
			day = ((moddata[3 + 2*i] & 0x07) << 2) + ((moddata[4 + 2*i] & 0xC0) >> 6);
			schedule = moddata[4 + 2*i] & 0x3F;
			snprintf(buf, sizeof(buf), "%02d.%02d", day, month);

			snprintf(schedule_str, sizeof(schedule_str), "%d", schedule);		

			add_value_pair(mqtt_msg, buf, schedule_str);			
		}
		for(i = num_holidays; i < 16; i++) {
			add_value_pair(mqtt_msg, "00.00", "0");			
		}
		
	}			
	else if((cmd >= IEC61107_CMD_GET_VALUE_TOTAL_ALL) && (cmd <= IEC61107_CMD_GET_VALUE_TOTAL_DAY)) {
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
	else if(cmd == IEC61107_CMD_TARIFF_DEFAULT) {
		data_ptr = moddata + 2;	
		snprintf(buf, sizeof(buf), "%c", *data_ptr + 1);
		add_value_pair(mqtt_msg, "default tariff", buf);				
	}
	else if (cmd == IEC61107_CMD_STATUS){
		char curr_tariff[5] = { };
		data_ptr = moddata + 2;	
		
		uint8_t err_schedule = (*data_ptr & 0x01);
		data_ptr++;
		uint8_t tariff_sch = (*data_ptr & 0x0F);
		data_ptr++;
		uint8_t cs_metrolog = (*data_ptr & 0x02);
		uint8_t cs_mem = (*data_ptr & 0x01);
		data_ptr++;
		uint8_t life_batt = (*data_ptr & 0x08);
		uint8_t stat_cover = (*data_ptr & 0x02);
		uint8_t cs_energy = (*data_ptr & 0x02);
		data_ptr++;
		uint8_t season = (*data_ptr & 0x04);
		uint8_t stat_time = (*data_ptr & 0x01);
		data_ptr++;
		uint8_t stat_volt = (*data_ptr & 0x0C);
		uint8_t correct_time = (*data_ptr & 0x02);
		uint8_t load = (*data_ptr & 0x01);
		data_ptr++;
		uint8_t energy_direct = (*data_ptr & 0x08);
		data_ptr++;
		uint8_t stat_batt = (*data_ptr & 0x08);
		uint8_t curr_tar = (*data_ptr & 0x07);
	
		snprintf(curr_tariff, sizeof(curr_tariff), "T%02d", curr_tar);
		add_value_pair(mqtt_msg, "current tariff", curr_tariff);
		
		num_char = snprintf(buf, sizeof(buf), "[ ");		
		for(i = 0; i < 4; i++) {
			if(tariff_sch & (1 << i)) {
				num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%d, ", i + 1);
			}									
		}	
		num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "]");
		add_value_pair(mqtt_msg, "schedule tariffs", buf);
		
		if(err_schedule == 0x01){
			add_value_pair(mqtt_msg, "schedule status", "error");			
		}
		else if(err_schedule == 0x00) {
			add_value_pair(mqtt_msg, "schedule status", "normal");
		}		
		
		if(season == 0x01){
			add_value_pair(mqtt_msg, "season", "summer");			
		}
		else if(season == 0x00) {
			add_value_pair(mqtt_msg, "season", "winter");
		}		
		
		if(stat_time == 0x01){
			add_value_pair(mqtt_msg, "time status", "failure");			
		}
		else if(stat_time == 0x00) {
			add_value_pair(mqtt_msg, "time status", "normal");
		}					
		if(correct_time == 0x01){
			add_value_pair(mqtt_msg, "time correction", "not allowed");			
		}
		else if(correct_time == 0x00) {
			add_value_pair(mqtt_msg, "time correction", "allowed");
		}			
		
		if(stat_batt == 0x01){
			add_value_pair(mqtt_msg, "battery status", "discharged");			
		}
		else if(stat_batt == 0x00) {
			add_value_pair(mqtt_msg, "battery status", "charged");
		}
		if(life_batt == 0x01){
			add_value_pair(mqtt_msg, "battery lifetime", "expired");			
		}
		else if(life_batt == 0x00) {
			add_value_pair(mqtt_msg, "battery lifetime", "normal");
		}		

		if(stat_volt == 0x01){
			add_value_pair(mqtt_msg, "voltage status", "overvoltage");			
		}
		else if(stat_volt == 0x00) {
			add_value_pair(mqtt_msg, "voltage status", "normal");
		}	
		else if(stat_volt == 0x02) {
			add_value_pair(mqtt_msg, "voltage status", "undervoltage");
		}			
		
		if(load == 0x01){
			add_value_pair(mqtt_msg, "load", "inductive");			
		}
		else if(load == 0x00) {
			add_value_pair(mqtt_msg, "load", "capacitive");
		}			
		
		if(energy_direct == 0x01){
			add_value_pair(mqtt_msg, "direction", "reverse");			
		}
		else if(energy_direct == 0x00) {
			add_value_pair(mqtt_msg, "direction", "direct");
		}		
			
		if(cs_energy == 0x01){
			add_value_pair(mqtt_msg, "energy values", "checksum error");			
		}
		else if(cs_energy == 0x00) {
			add_value_pair(mqtt_msg, "energy values", "normal");
		}				
		
		if(stat_cover == 0x01){
			add_value_pair(mqtt_msg, "tamper", "failure");			
		}
		else if(stat_cover == 0x00) {
			add_value_pair(mqtt_msg, "tamper", "normal");
		}						
		
		if(cs_mem == 0x01){
			add_value_pair(mqtt_msg, "program memory", "checksum error");			
		}
		else if(cs_mem == 0x00) {
			add_value_pair(mqtt_msg, "program memory", "normal");
		}		

		if(cs_metrolog == 0x01){
			add_value_pair(mqtt_msg, "metrology", "checksum error");			
		}
		else if(cs_metrolog == 0x00) {
			add_value_pair(mqtt_msg, "metrology", "normal");
		}		
		
	}
	else if(cmd == IEC61107_ERROR_PROTOCOL_DEVICE) {
		char err_buf[5];
		uint8_t error = moddata[2];
		
		snprintf(err_buf, sizeof(err_buf), "%d", error);
		add_value_pair(mqtt_msg, "err", err_buf);
		
		if(error == 10) {
			add_value_pair(mqtt_msg, "msg", "invalid number of parameters");
		}
		else if(error == 11) {
			add_value_pair(mqtt_msg, "msg", "not supported");
		}
		else if(error == 12) {
			add_value_pair(mqtt_msg, "msg", "unknown parameter");
		}
		else if(error == 13) {
			add_value_pair(mqtt_msg, "msg", "invalid format");
		}
		else if(error == 14) {
			add_value_pair(mqtt_msg, "msg", "not initialized");
		}
		else if(error == 15) {
			add_value_pair(mqtt_msg, "msg", "access denied");
		}
		else if(error == 16) {
			add_value_pair(mqtt_msg, "msg", "no programming rights");
		}
		else if(error == 17) {
			add_value_pair(mqtt_msg, "msg", "invalid parameter value");
		}
		else if(error == 18) {
			add_value_pair(mqtt_msg, "msg", "nonexistent parameter value");
		}
	}
				
	return true;
}
