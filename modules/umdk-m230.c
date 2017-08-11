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

	M230_CMD_SET_IFACE			= 0xFB,		/* Set interfase => CAN or RS485 */
    
    M230_CMD_PROPRIETARY_COMMAND = 0xF0,	/* Less this value single command of mercury */

    M230_CMD_TEST_RESPONSE		= 0x00,		/* Testing channel - response from device */
	M230_CMD_OPEN_LINE			= 0x01,		/* Open a communication channel */
	M230_CMD_CLOSE_LINE			= 0x02,		/* Close a communication channel */
		
	M230_CMD_GET_VALUE			= 0x03,		/* Read the values of power */
	M230_CMD_GET_TIMEDATE		= 0x04,		/* Read the internal time and date */
	M230_CMD_GET_SERIAL			= 0x05,		/* Read serial number of the device */
	M230_CMD_SET_TIMEDATE		= 0x06,		/* Set the internal time and date */
	M230_CMD_SET_LOAD		= 0x07,		/* Set on/off control of powerload */
	M230_CMD_GET_LOAD		= 0x08,		/* Read mode of control of powerload */	
	M230_CMD_SET_SCHEDULE		= 0x09,		/* Set the schedule of tariffs */
	M230_CMD_SET_LIMIT_POWER	= 0x0A,		/* Set the value of the active power limit */
	M230_CMD_SET_MODE_LIMIT_POWER	= 0x0B,	/* Set on/off the active power limit */
	M230_CMD_SET_LIMIT_ENERGY	= 0x0C,		/* Set the value of the active energy limit */
	M230_CMD_SET_MODE_LIMIT_ENERGY	= 0x0D,	/* Set on/off the active energy limit */
	M230_CMD_SET_MODE_TARIFF	= 0x0E,		/* Set mode of tariffs */	
	M230_CMD_GET_MODE_TARIFF	= 0x0F,		/* Get mode of tariffs */	
	M230_CMD_GET_ERROR_STATUS	= 0x10,		/* Get status of errors */	
	M230_CMD_GET_VERSION	= 0x11,		/* Read the version of device */		
	M230_CMD_SET_MODE_PULSE_OUT	= 0x12,		/* Changing the pulse output mode */	
	M230_CMD_GET_SOFTWARE	= 0x13,			/* Read the version software of device */	
	M230_CMD_GET_SCHEDULE	= 0x14,			/* Read the schedule of tariffs */
	M230_CMD_GET_INFO	= 0x15,			/* Read the info of device */
	M230_CMD_GET_HOLIDAYS	= 0x16,			/* Read holidays */
	M230_CMD_GET_STATUS_LONG_CMD	= 0x17,			/* Read status of the long-time operations */
} m230_cmd_t;

typedef enum {
	M230_ALL_YEAR = 0x0FFF,
	M230_ALL_DAYS = 0xFF,
	M230_WEEKDAYS = 0x1F,
	M230_WEEKENDS = 0x60,
	M230_HOLIDAYS = 0x80,
} m230_scheduler_t;

typedef enum {
	M230_ERROR_REPLY 		= 0xF0,
    M230_OK_REPLY 			= 0xF1,
    M230_NO_RESPONSE_REPLY 	= 0xF2,
	
	M230_ERROR_NOT_FOUND	= 0xF3,
	M230_ERROR_OFFLINE		= 0xF4,
} m230_reply_t;

typedef enum {
	M230_OK 			= 0x00,
	M230_WRONG_CMD 		= 0x01,
    M230_INTERNAL_ERROR = 0x02,
	M230_ACCESS_ERROR 	= 0x03,
	M230_TIME_CORRECTED	= 0x04,
	M230_OFFLINE 		= 0x05,
} m230_status_byte_t;

static char season[2][7] = { "Summer", "Winter" };
static char dow[8][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Hol" };

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
	else if (strstr(param, "get long_time ") == param) {
		param += strlen("get long_time ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x24;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_STATUS_LONG_CMD, destination, tmp);
	}	
	else if (strstr(param, "get timedate ") == param) {
		param += strlen("get timedate ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x00;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_TIMEDATE, destination, tmp);
	}	
	else if (strstr(param, "get serial ") == param) {
		param += strlen("get serial ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x00;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_SERIAL, destination, tmp);
	}	
	else if (strstr(param, "get soft ") == param) {
		param += strlen("get soft ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x03;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_SOFTWARE, destination, tmp);
	}	
	else if (strstr(param, "get error ") == param) {
		param += strlen("get error ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x0A;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_ERROR_STATUS, destination, tmp);
	}	
	else if (strstr(param, "get info ") == param) {
		param += strlen("get info ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x01;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_INFO, destination, tmp);
	}	
	else if (strstr(param, "get version ") == param) {
		param += strlen("get version ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x12;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_VERSION, destination, tmp);
	}	
	else if (strstr(param, "set timedate ") == param) {
		param += strlen("set timedate ");    // Skip command
	
		uint8_t hour = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t min = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t sec = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		uint8_t dow = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space		
		
		
		uint8_t day = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t month = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t year = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		uint8_t season = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x0C;
		
		snprintf(out, bufsize, "%02x%02x%02x%02d%02d%02d%02d%02d%02d%02d%02d", 
								M230_CMD_SET_TIMEDATE, destination, tmp, sec, min, hour, dow, day, month, year, season);		
		
	}	
	else if (strstr(param, "set mode_pulse ") == param) {
		param += strlen("set mode_pulse ");    // Skip command
		uint8_t mode;
		if (strstr(param, "load") == param) { 
			param += strlen("load");    // Skip command
			mode = 1;
		}
		else if (strstr(param, "telemetry") == param) { 	
			param += strlen("telemetry");    // Skip command
			mode = 0;	
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x30;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_MODE_PULSE_OUT, destination, tmp, mode);
	}			
	else if (strstr(param, "set load ") == param) {
		param += strlen("set load ");    // Skip command
		uint8_t powerload;
		if (strstr(param, "on") == param) { 
			param += strlen("on");    // Skip command
			powerload = 0;
		}
		else if (strstr(param, "off") == param) { 	
			param += strlen("off");    // Skip command
			powerload = 1;	
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x31;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_LOAD, destination, tmp, powerload);
	}	
	else if (strstr(param, "get load ") == param) {
		param += strlen("get load ");    // Skip command
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x18;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_LOAD, destination, tmp);
	}		
	else if (strstr(param, "set limit_power ") == param) {
		param += strlen("set limit_power ");    // Skip command
		uint32_t limit = 0;
		limit = strtol(param, &param, 10);
		limit = limit & 0x00FFFFFF;
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2C;
		snprintf(out, bufsize, "%02x%02x%02x%06x", M230_CMD_SET_LIMIT_POWER, destination, tmp, limit);
	}		
	else if (strstr(param, "set mode_limit_power ") == param) {
		param += strlen("set mode_limit_power ");    // Skip command
		uint8_t mode;
		if (strstr(param, "on") == param) { 
			param += strlen("on");    // Skip command
			mode = 1;
		}
		else if (strstr(param, "off") == param) { 	
			param += strlen("off");    // Skip command
			mode = 0;	
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2D;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_MODE_LIMIT_POWER, destination, tmp, mode);
	}		
	else if (strstr(param, "set limit_energy ") == param) {
		param += strlen("set limit_energy ");    // Skip command
		uint8_t tariff = 0;
		tariff = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint32_t limit = 0;
		limit = strtol(param, &param, 10);
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2E;
		snprintf(out, bufsize, "%02x%02x%02x%02x%08x", M230_CMD_SET_LIMIT_ENERGY, destination, tmp, tariff, limit);
	}	
	else if (strstr(param, "set mode_limit_energy ") == param) {
		param += strlen("set mode_limit_energy ");    // Skip command
		uint8_t mode;
		if (strstr(param, "on") == param) { 
			param += strlen("on");    // Skip command
			mode = 1;
		}
		else if (strstr(param, "off") == param) { 	
			param += strlen("off");    // Skip command
			mode = 0;	
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2F;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_MODE_LIMIT_ENERGY, destination, tmp, mode);
	}			
	else if (strstr(param, "set mode_tariff ") == param) {
		param += strlen("set mode_tariff ");    // Skip command
		uint8_t mode;
		if (strstr(param, "one") == param) { 
			param += strlen("one");    // Skip command
			mode = 1;
		}
		else if (strstr(param, "multi") == param) { 	
			param += strlen("multi");    // Skip command
			mode = 0;	
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2A;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_MODE_TARIFF, destination, tmp, mode);
	}	
	else if (strstr(param, "get mode_tariff ") == param) {
		param += strlen("get mode_tariff ");    // Skip command
		
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x17;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_MODE_TARIFF, destination, tmp);
	}		
	else if (strstr(param, "get holidays ") == param) {
		param += strlen("get holidays ");    // Skip command
		uint8_t month = 0;
		
		month = strtol(param, &param, 10);
		
		param += strlen(" ");    						// Skip space				
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x23;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_GET_HOLIDAYS, destination, tmp, month);
	}		
	else if (strstr(param, "get schedule ") == param) {
		param += strlen("get schedule ");    // Skip command
		// uint8_t half = 0;
		uint16_t month = 0;
		uint8_t day = 0;
		
		month = strtol(param, &param, 10);
		month--;
		month = 1 << month;
		// month  |= half;
		param += strlen(" ");    						// Skip space		
		day = strtol(param, &param, 10);
		day--;
		day = 1 << day;
		param += strlen(" ");    						// Skip space		
		
		destination = strtol(param, &param, 10);
		uint8_t tmp = 0x22;
		uint16_to_le(&month);
		snprintf(out, bufsize, "%02x%02x%02x%04x%02x", M230_CMD_GET_SCHEDULE, destination, tmp, month, day);
	}		
	else if (strstr(param, "set schedule ") == param) { 
		uint8_t i = 0;
		uint16_t point_schedule[8] = { 0x0038 };
		uint8_t hour_tmp;
		uint8_t tariff;
		uint8_t min_tmp;
		uint8_t day = 0;
		uint16_t month = 0;
		
		for(i = 0; i < 8; i++) {
			point_schedule[i] = 0x0038;
		}

		param += strlen("set schedule "); // skip command
		
		if(strstr(param, "year ") == param) {
			param += strlen("year ");				// Skip command
			month = (uint16_t)M230_ALL_YEAR;
		}
		else if (strstr(param, "month ") == param) {
			param += strlen("month ");				// Skip command
			month = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			month--;
			month = 1 << month;
		}
	
		if(strstr(param, "day ") == param) {
			param += strlen("day ");				// Skip command
			day = strtol(param, &param, 10);
			day--;
			day = 1 << day;
		}
		else if(strstr(param, "all") == param) {
			param += strlen("all");				// Skip command
			day = (uint8_t)M230_ALL_DAYS;
		}
		else if(strstr(param, "weekdays") == param) {
			param += strlen("weekdays");				// Skip command			
			day = (uint8_t)M230_WEEKDAYS;
		}
		else if(strstr(param, "weekends") == param) {
			param += strlen("weekends");				// Skip command			
			day = (uint8_t)M230_WEEKENDS;
		}
		else if(strstr(param, "holidays") == param) {
			param += strlen("holidays");				// Skip command			
			day = (uint8_t)M230_HOLIDAYS;
		}
			
		param += strlen(" ");    						// Skip space
		uint8_t checkpoint = strtol(param, &param, 10);

		for(i = 0; i < checkpoint; i++) {
			param += strlen(" ");    						// Skip space
			tariff = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			hour_tmp = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			min_tmp = strtol(param, &param, 10);
			
			point_schedule[i] = ((min_tmp << 8) + (tariff << 5) + (hour_tmp << 0)) & 0x1FFF;
			uint16_to_le(&point_schedule[i]);
		}
	
		param += strlen(" ");    						// Skip space	
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x1D;
		
		uint8_t num_char;
		uint16_to_le(&month);
		num_char = snprintf(out, bufsize, "%02x%02x%02x%04x%02x", M230_CMD_SET_SCHEDULE, destination, tmp, month, day);
		for(i = 0; i < 8; i++) {
			num_char += snprintf(out + num_char, bufsize - num_char, "%04x", point_schedule[i]);
		}
	}	
	else if (strstr(param, "iface ") == param) { 
		param += strlen("iface ");    // Skip command	
		uint8_t interface;
		if (strstr(param, "can") == param) { 	
			interface = 2;
		}
		else if (strstr(param, "485") == param) { 	
			interface = 1;		
		}
		
		snprintf(out, bufsize, "%02x%02x", M230_CMD_SET_IFACE, interface);
	}
}

bool umdk_m230_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
    char strbuf[20];
	char buf_addr[30];
	uint8_t ii;
    puts("[m230] RX:  ");
    for(ii = 0; ii < moddatalen; ii++) {
        printf(" %02X ", moddata[ii]);
    }
   puts("\n");
	
	
   if (moddatalen == 1) {
		if (moddata[0] == M230_OK_REPLY) {
			add_value_pair(mqtt_msg, "Msg", "Ok");
		} 
		else if(moddata[0] == M230_ERROR_REPLY){
			add_value_pair(mqtt_msg, "Msg", "Error");
		}
		return true;
	}	
	
	m230_cmd_t cmd = moddata[0];	
	
	// if(cmd < M230_CMD_PROPRIETARY_COMMAND) {
		
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
			} else if(moddata[0] == M230_ERROR_NOT_FOUND){
				add_value_pair(mqtt_msg, "Msg", "Device Not found");					
			} else if(moddata[0] == M230_ERROR_OFFLINE){
				add_value_pair(mqtt_msg, "Msg", "OFFLINE");					
			} else if(moddata[0] == M230_WRONG_CMD){
				add_value_pair(mqtt_msg, "Msg", "Invalid command or parameter");							
			} else if(moddata[0] == M230_INTERNAL_ERROR){
				add_value_pair(mqtt_msg, "Msg", "Internal error");							
			} else if(moddata[0] == M230_ACCESS_ERROR){
				add_value_pair(mqtt_msg, "Msg", "Access error");					
			} else if(moddata[0] == M230_TIME_CORRECTED){
				add_value_pair(mqtt_msg, "Msg", "Time already corrected");								
			} else if(moddata[0] == M230_OFFLINE){
				add_value_pair(mqtt_msg, "Msg", "Offline");					
			} else if(moddata[0] == M230_OK){
				add_value_pair(mqtt_msg, "Msg", "OK");					
			}			
			return true;
		}
		
	// }
	
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

			add_value_pair(mqtt_msg, "Day", dow[time[3]]);
			
			snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", time[2], time[1], time[0]);	
			add_value_pair(mqtt_msg, "Time", time_buf);
			
			snprintf(time_buf, sizeof(time_buf), "%02d/%02d/%02d", time[4], time[5], time[6]);	
			add_value_pair(mqtt_msg, "Date", time_buf);			
			
			add_value_pair(mqtt_msg, "Season", season[time[7]]);			
			
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
		
		case M230_CMD_GET_LOAD: {
			uint8_t mode_load = moddata[2] & 0x01;
			uint8_t mode_limit_energy = (moddata[2] >> 2) & 0x01;
			uint8_t mode_limit_power = (moddata[2] >> 1) & 0x01;
			
			uint8_t powerload_on_off = (moddata[3] >> 1) & 0x01;;
			
			if(mode_limit_energy == 1) {
				add_value_pair(mqtt_msg, "Control of limit energy", "Allowed");
			}
			else if(mode_limit_energy == 0) {
				add_value_pair(mqtt_msg, "Control of limit energy", "Not Allowed");				
			}		

			if(mode_limit_power == 1) {
				add_value_pair(mqtt_msg, "Control of limit power", "Allowed");
			}
			else if(mode_limit_power == 0) {
				add_value_pair(mqtt_msg, "Control of limit power", "Not Allowed");				
			}						
			
			if(mode_load == 1) {
				add_value_pair(mqtt_msg, "Mode of pulse output", "Load");
			}
			else if(mode_load == 0) {
				add_value_pair(mqtt_msg, "Mode of pulse output", "Telemetry");				
			}			
			
			if(powerload_on_off == 1) {
				add_value_pair(mqtt_msg, "Control load", "Off");
			}
			else if(powerload_on_off == 0) {
				add_value_pair(mqtt_msg, "Control load", "On");				
			}
			
			
			return true;
			break;	
		}
		
		case M230_CMD_GET_MODE_TARIFF: {
			uint8_t current_tariff = (moddata[3] & 0x0E) >> 1;
			uint8_t mode_tariff =  moddata[3] & 0x01;
			
			if(mode_tariff == 0) {
				add_value_pair(mqtt_msg, "Mode", "Multi-tariff");
			}
			else if(mode_tariff == 1) {
				add_value_pair(mqtt_msg, "Mode", "One-tariff");				
			}
						
			snprintf(buf_addr, sizeof(buf_addr), "T%02d", current_tariff + 1);	
			add_value_pair(mqtt_msg, "Current tariff", buf_addr);		

			return true;
			break;			
		}
		
		case M230_CMD_GET_ERROR_STATUS: {
			uint8_t i, j = 0;
			uint8_t error = 0;
			uint8_t status = 0;
	
			for(i = 0; i < 6; i++) {
				status = moddata[i + 2];
				for(j = 0; j < 8; j++) {
					error = (status >> j) & 1;
					if(error == 1) {
						snprintf(buf_addr, sizeof(buf_addr), "E-%02d", 8*i + j + 1);	
						add_value_pair(mqtt_msg, "Error", buf_addr);												
					}
				}
			}
			
			return true;
			break;
		}
		
		case M230_CMD_GET_VERSION: {
			
			return true;
			break;			
		}
		
		case M230_CMD_GET_SOFTWARE: {
			
			snprintf(buf, sizeof(buf), "%02d.%02d.%02d", moddata[i + 2], moddata[i + 3], moddata[i + 4]);	
			add_value_pair(mqtt_msg, "Software version", buf);
			
			return true;
			break;			
		}
		
		case M230_CMD_GET_SCHEDULE: {
			
			uint8_t tariff = 0;
			uint8_t hour = 0;
			uint8_t min = 0;
			uint8_t i = 0;
			char tariff_str[5] = { };

			for(i = 0; i < 8; i++) {
				min = moddata[2*i + 2] & 0x1F;
				tariff = (moddata[2*i + 3] >> 5) & 0x07;
				hour = moddata[2*i + 3] & 0x1F;
				
				snprintf(tariff_str, sizeof(tariff_str), "T%02d", tariff);
				snprintf(buf, sizeof(buf), "%02d:%02d",  hour, min);
				add_value_pair(mqtt_msg, tariff_str, buf);
			}
			
			return true;
			break;						
		}
		
		case M230_CMD_GET_INFO: {
			
			
			return true;
			break;						
		}
		
		case M230_CMD_GET_HOLIDAYS: {
			
			return true;
			break;						
		}
		
		case M230_CMD_GET_STATUS_LONG_CMD: {
			
			return true;
			break;					
		}
				
		default:
			break;
	}
	return true;
}
