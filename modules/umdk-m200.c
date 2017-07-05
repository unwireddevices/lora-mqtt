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

#define M200_ADDR_DEF 0xFFFFFFFF

typedef enum {
    M200_CMD_RESET 				= 0xFF,		/* Clear database */
    M200_CMD_ADD_ADDR 			= 0xFE,		/* Add address  in database */
    M200_CMD_REMOVE_ADDR 		= 0xFD,		/* Remove address from database */
	M200_CMD_GET_LIST 			= 0xFC,		/* Send database of addresses */
 
	M200_CMD_SET_IFACE			= 0xFB,		/* Set interfase => CAN or RS485 */
 
    M200_CMD_SET_TABLE_HOLIDAYS 	= 0xF1, 	/* Set the full table of holidays */
    
    M200_CMD_PROPRIETARY_COMMAND = 0xF0,		/* Less this value single command of mercury */

    M200_CMD_GET_ADDR			= 0x00,		/* Read the address */
    M200_CMD_GET_SERIAL 			= 0x01,		/* Read the serial number */
    M200_CMD_SET_NEW_ADDR 		= 0x02,		/* Set new address */
    M200_CMD_GET_CURR_TARIFF 	= 0x03,		/* Read the current tariff */
    M200_CMD_GET_LAST_OPEN 		= 0x04,		/* Read the time of last opening */
    M200_CMD_GET_LAST_CLOSE 		= 0x05,		/* Read the time of last closing */
    M200_CMD_GET_U_I_P 			= 0x06,		/* Read the value of the voltage, current and power */
    M200_CMD_GET_TIMEDATE 		= 0x07,		/* Read the internal time and date */
    M200_CMD_GET_LIMIT_POWER 	= 0x08,		/* Read the limit of power */
    M200_CMD_GET_CURR_POWER_LOAD = 0x09,		/* Read the current power load */
    M200_CMD_GET_TOTAL_VALUE 	= 0x0A,		/* Read the total values of power after reset */
    M200_CMD_GET_LAST_POWER_OFF 	= 0x0B,		/* Read the time of last power off */
    M200_CMD_GET_LAST_POWER_ON 	= 0x0C,		/* Read the time of last power on */
    M200_CMD_GET_HOLIDAYS 		= 0x0D,		/* Read the table of holidays */
    M200_CMD_GET_SCHEDULE 		= 0x0E,		/* Read the schedule of tariffs */
    M200_CMD_GET_VALUE 			= 0x0F,		/* Read the month's value */
    M200_CMD_GET_NUM_TARIFFS 	= 0x10,		/* Read the number of tariffs */
    M200_CMD_SET_NUM_TARIFFS 	= 0x11,		/* Set number of tariffs */
    M200_CMD_SET_TARIFF 			= 0x12,		/* Set the tariff */
    M200_CMD_SET_HOLIDAYS 		= 0x13,		/* Set the table of holidays */
    M200_CMD_SET_SCHEDULE 		= 0x14,		/* Set the schedule of tariffs */
    M200_CMD_GET_WORKING_TIME 	= 0x15,		/* Read the total working time of battery and device */
    M200_CMD_SET_TIMEDATE		= 0x16,		/* Set the internal time */
} m200_cmd_t;

typedef enum {
	M200_ALL_YEAR = 0x0F,
	M200_ALL_DAYS = 0x0F,
	M200_WEEKDAYS = 0x0E,
	M200_WEEKENDS = 0x0D,
	M200_HOLIDAYS = 0x0C,
} m200_scheduler_t;

typedef enum {
	ERROR_REPLY 		= 0,
    OK_REPLY 			= 1,
    NO_RESPONSE_REPLY	= 2,
} m200_reply_t;

static char str_dow[8][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Hol" };

void umdk_m200_command(char *param, char *out, int bufsize) {
	uint32_t destination;
		
	if (strstr(param, "set address ") == param) {
		param += strlen("set address ");    // Skip command
		uint32_t new_address = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		uint32_to_le(&new_address);
		snprintf(out, bufsize, "%02x%08x%08x", M200_CMD_SET_NEW_ADDR, destination, new_address);
	}
	else if (strstr(param, "get serial ") == param) {
		param += strlen("get serial ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x", M200_CMD_GET_SERIAL, destination);
	}
	else if (strstr(param, "get number tariffs ") == param) {
		param += strlen("get number tariffs ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x", M200_CMD_GET_NUM_TARIFFS, destination);
	}	
	else if (strstr(param, "set number tariffs ") == param) {
		param += strlen("set number tariffs ");    // Skip command
		uint8_t tarif = strtol(param, &param, 10);
		// tarif--;
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x%02x", M200_CMD_SET_NUM_TARIFFS, destination, tarif);
	}			
	else if (strstr(param, "get value ") == param) { 
		param += strlen("get value ");    // Skip command
		uint8_t month = 0;
		if(strstr(param, "total") == param) {
			param += strlen("total");				// Skip command
			month = 0xFF;
		}
		else if(strstr(param, "current") == param) {
			param += strlen("current");				// Skip command
			month = 0x0F;
		}
		else if(strstr(param, "month ") == param) {
			param += strlen("month ");				// Skip command			
			month = strtol(param, &param, 10);
			month--;
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		if(month == 0xFF) {
			snprintf(out, bufsize, "%02x%08x", M200_CMD_GET_TOTAL_VALUE, destination);			
		}
		else {
			snprintf(out, bufsize, "%02x%08x%02x", M200_CMD_GET_VALUE, destination, month);			
		}
	}
	else if (strstr(param, "get schedule ") == param) { 
		param += strlen("get schedule "); // skip command
		uint8_t month = strtol(param, &param, 10);
		month--;
		param += strlen(" ");    						// Skip space
		uint8_t dow = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint8_t date = (uint8_t)((month << 4) + (dow << 0));
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x%02x", M200_CMD_GET_SCHEDULE, destination, date);
	}
	else if (strstr(param, "get timedate ") == param) { 
		param += strlen("get timedate ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x", M200_CMD_GET_TIMEDATE, destination);
	}
	else if (strstr(param, "set timedate ") == param) { 
		param += strlen("set timedate "); // skip command
		uint8_t dow = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t hour = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t min = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t sec = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t day = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t month = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t year = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x%02d%02d%02d%02d%02d%02d%02d", 
														M200_CMD_SET_TIMEDATE, destination, dow, hour, min, sec, day, month, year);
	}	
	else if (strstr(param, "set holidays ") == param) { 
		param += strlen("set holidays "); // skip command
		
		uint8_t day[16] = { 0xFF };
		uint8_t month[16] = { 0xFF };
		uint8_t i = 0;
		memset(day, 0xFF, sizeof(day));
		memset(month, 0xFF, sizeof(month));
		
		uint8_t number = strtol(param, &param, 10);
		for(i = 0; i < number; i++) {
			param += strlen(" ");    						// Skip space
			day[i] = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			month[i] = strtol(param, &param, 10);
		}
	
		param += strlen(" ");    						// Skip space	
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
	
		uint8_t num_char;
		num_char = snprintf(out, bufsize, "%02x%08x", M200_CMD_SET_TABLE_HOLIDAYS, destination);
		
		for(i = 0; i < sizeof(day); i++) {
			if(day[i] != 0xFF) {
				num_char += snprintf(out + num_char, bufsize - num_char, "%02d%02d", day[i], month[i]);
			}
			else {
				num_char += snprintf(out + num_char, bufsize - num_char, "%02x%02x", day[i], month[i]);				
			}
		}
		
	}
	else if (strstr(param, "set schedule ") == param) { 
		uint8_t i = 0;
		uint8_t tariff_hour[8] = { 0xFF };
		uint8_t hour_tmp = 0xFF;
		uint8_t tariff = 0xFF;
		uint8_t min[8] = { 0xFF };
		uint8_t min_tmp = 0xFF;
		uint8_t day = 0;
		uint8_t month = 0;
		uint8_t date = 0;
		memset(tariff_hour, 0xFF, sizeof(tariff_hour));
		memset(min, 0xFF, sizeof(min));

		param += strlen("set schedule "); // skip command
		
		if(strstr(param, "year ") == param) {
			param += strlen("year ");				// Skip command
			month = (uint8_t)M200_ALL_YEAR;
		}
		else if (strstr(param, "month ") == param) {
			param += strlen("month ");				// Skip command
			month = strtol(param, &param, 10);
			month--;
			param += strlen(" ");    						// Skip space
		}
	
		if(strstr(param, "day ") == param) {
			param += strlen("day ");				// Skip command
			day = strtol(param, &param, 10);
		}
		else if(strstr(param, "all") == param) {
			param += strlen("all");				// Skip command
			day = (uint8_t)M200_ALL_DAYS;
		}
		else if(strstr(param, "weekdays") == param) {
			param += strlen("weekdays");				// Skip command			
			day = (uint8_t)M200_WEEKDAYS;
		}
		else if(strstr(param, "weekends") == param) {
			param += strlen("weekends");				// Skip command			
			day = (uint8_t)M200_WEEKENDS;
		}
		else if(strstr(param, "holidays") == param) {
			param += strlen("holidays");				// Skip command			
			day = (uint8_t)M200_HOLIDAYS;
		}
			
		param += strlen(" ");    						// Skip space
		uint8_t checkpoint = strtol(param, &param, 10);

		for(i = 0; i < checkpoint; i++) {
			param += strlen(" ");    						// Skip space
			tariff = strtol(param, &param, 10);
			tariff--;
			param += strlen(" ");    						// Skip space
			hour_tmp = strtol(param, &param, 10);
			tariff_hour[i] = (uint8_t)((tariff << 6) + ((hour_tmp / 10) << 4) + ((hour_tmp % 10) << 0) );
			param += strlen(" ");    						// Skip space
			min_tmp = strtol(param, &param, 10);
			min[i] = (uint8_t)(((min_tmp / 10) << 4) + ((min_tmp % 10) << 0));
		}
	
		param += strlen(" ");    						// Skip space	
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		
		date = (uint8_t)((month << 4) + (day << 0));
		
		uint8_t num_char;
		num_char = snprintf(out, bufsize, "%02x%08x", M200_CMD_SET_SCHEDULE, destination);
		for(i = 0; i < sizeof(tariff_hour); i++) {
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x%02x", tariff_hour[i], min[i]);
		}
		snprintf(out + num_char, bufsize - num_char, "%02x", date);
	}
	else if (strstr(param, "add ") == param) { 
		param += strlen("add ");    // Skip command
		uint32_t addr = strtol(param, &param, 10);
		uint32_to_le(&addr);
		snprintf(out, bufsize, "%02x%08x", M200_CMD_ADD_ADDR, addr);
	}
	else if (strstr(param, "remove ") == param) { 
		param += strlen("remove ");    // Skip command
		uint32_t addr = strtol(param, &param, 10);
		uint32_to_le(&addr);
		snprintf(out, bufsize, "%02x%02x", M200_CMD_REMOVE_ADDR, addr);
	}
	else if (strstr(param, "reset") == param) { 
		snprintf(out, bufsize, "%02x", M200_CMD_RESET);
	}
	else if (strstr(param, "get list") == param) { 
		snprintf(out, bufsize, "%02x", M200_CMD_GET_LIST);
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
		
		snprintf(out, bufsize, "%02x%02x", M200_CMD_SET_IFACE, interface);
	}
}

bool umdk_m200_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
    char strbuf[20];
	char buf_addr[30];

   if (moddatalen == 1) {
        if (moddata[0] == OK_REPLY) {
            add_value_pair(mqtt_msg, "Msg", "Ok");
        } else if(moddata[0] == ERROR_REPLY){
            add_value_pair(mqtt_msg, "Msg", "Error");
		}
        return true;
    }	
	
	m200_cmd_t cmd = moddata[0];	
	
	if(cmd < M200_CMD_PROPRIETARY_COMMAND) {
		uint32_t *address = (uint32_t *)(&moddata[1]);
		uint32_to_le(address);
		snprintf(buf_addr, sizeof(buf_addr), "%u", *address);	
		add_value_pair(mqtt_msg, "Address", buf_addr);
							
		if (moddatalen == 5) {
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
		case M200_CMD_GET_LIST: {
			uint8_t cnt = 0;
			char number[15];
			uint8_t num_devices = (moddatalen - 1) / 4;
			uint32_t *address_dev;
			
			for( i  = 0; i < num_devices; i++) {
				address_dev = (uint32_t *)(&moddata[4*i + 1]);
				if(*address_dev != M200_ADDR_DEF) {
					cnt++;
					uint32_to_le(address_dev);			

					snprintf(number, sizeof(number), "Address %d", cnt);		
					snprintf(buf_addr, sizeof(buf_addr), "%u", *address_dev);	
					add_value_pair(mqtt_msg, number, buf_addr);								
				}
			}
			if(cnt == 0) {
				add_value_pair(mqtt_msg, "Msg", "Empty");				
				}
			return true;
			break;
		}		
			
		case M200_CMD_GET_SERIAL: {
			uint32_t *serial = (uint32_t *)(&moddata[5]);
			uint32_to_le(serial);      
			snprintf(buf, sizeof(buf), "%u", *serial);
			add_value_pair(mqtt_msg, "Serial number", buf);		
			return true;
			break;
		}
		
		case M200_CMD_GET_NUM_TARIFFS: {
			uint8_t number = moddata[5];  	
			snprintf(buf, sizeof(buf), "%u", number);
			add_value_pair(mqtt_msg, "Number of tariffs", buf);		
			return true;
			break;
		}
		
		case M200_CMD_GET_TOTAL_VALUE: {
			uint32_t value[5] = { 0 };
			for(i = 0; i < 5; i++) {
				ptr_value = (uint32_t *)(&moddata[4*i + 5]);
				uint32_to_le(ptr_value);      
				value[i] = *ptr_value;
			}
			 		
			char tariff[5] = { };
			for(i = 0; i < 4; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d", i + 1);
                int_to_float_str(strbuf, value[i], 2);
				snprintf(buf, sizeof(buf), "%s", strbuf);
				add_value_pair(mqtt_msg, tariff, buf);								
			}
            int_to_float_str(strbuf, value[4], 2);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, "Total", buf);		
			
			return true;
			break;
		}

		case M200_CMD_GET_VALUE: {
			uint32_t value[5] = { 0 };
			for(i = 0; i < 5; i++) {
				ptr_value = (uint32_t *)(&moddata[4*i + 5]);
				uint32_to_le(ptr_value);      
				value[i] = *ptr_value;
			}
			
			char tariff[5] = { };
			for(i = 0; i < 4; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d", i + 1);
                int_to_float_str(strbuf, value[i], 2);
				snprintf(buf, sizeof(buf), "%s", strbuf);
				add_value_pair(mqtt_msg, tariff, buf);				
			}
            int_to_float_str(strbuf, value[4], 2);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, "Total", buf);		
	
			return true;
			break;
		}		
		
		case M200_CMD_GET_SCHEDULE: {
			
			char tariff[5] = { };
			uint8_t num_schedule = (moddatalen  - 5) / 3;
			for(i = 0; i < num_schedule; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d", moddata[3*i + 5] + 1);		
				snprintf(buf, sizeof(buf), "%02d:%02d",  moddata[3*i + 6],  moddata[3*i + 7]);
				add_value_pair(mqtt_msg, tariff, buf);			
			}
			
			return true;
			break;
		}		
		
		case M200_CMD_GET_TIMEDATE: {
			
			char time_buf[10] = { };
			uint8_t time[7] = { 0 };
			
			for(i = 0; i < 7; i++) {
				time[i] = moddata[i + 5];
			}

			add_value_pair(mqtt_msg, "Day", str_dow[time[0]]);
			
			snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", time[1], time[2], time[3]);	
			add_value_pair(mqtt_msg, "Time", time_buf);
			
			snprintf(time_buf, sizeof(time_buf), "%02d/%02d/%02d", time[4], time[5], time[6]);	
			add_value_pair(mqtt_msg, "Date", time_buf);
			
			return true;
			break;
		}		
		
		default:
			break;
	}
	return true;
}
