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
		MERCURY_CMD_RESET = 0xFF,		/* Clear database */
    MERCURY_CMD_ADD_ADDR = 0xFE,		/* Add address  in database */
		MERCURY_CMD_REMOVE_ADDR = 0xFD,		/* Remove address from database */

		MERCURY_CMD_SET_SCHED_HOLIDAY = 0xF5,
		MERCURY_CMD_SET_SCHED_WEEKDAY = 0xF4,
		MERCURY_CMD_SET_SCHED_WEEKEND = 0xF3,
		MERCURY_CMD_SET_SCHED_DAY = 0xF2,
		MERCURY_CMD_SET_SCHED_YEAR = 0xF1,
				
		MERCURY_CMD_PROPRIETARY_COMMAND = 0xF0,		/* Less this value single command of mercury */

    MERCURY_CMD_GET_ADDR = 0x00,		/* Read the address */
    MERCURY_CMD_GET_SERIAL = 0x01,		/* Read the serial number */
    MERCURY_CMD_SET_NEW_ADDR = 0x02,		/* Set new address */
    MERCURY_CMD_GET_CURR_TARIFF = 0x03,		/* Read the current tariff */
    MERCURY_CMD_GET_LAST_OPEN = 0x04,		/* Read the time of last opening */
    MERCURY_CMD_GET_LAST_CLOSE = 0x05,		/* Read the time of last closing */
    MERCURY_CMD_GET_U_I_P = 0x06,		/* Read the value of the voltage, current and power */
    MERCURY_CMD_GET_TIMEDATE = 0x07,		/* Read the internal time and date */
    MERCURY_CMD_GET_LIMIT_POWER = 0x08,		/* Read the limit of power */
    MERCURY_CMD_GET_CURR_POWER_LOAD = 0x09,	/* Read the current power load */
    MERCURY_CMD_GET_TOTAL_VALUE = 0x0A,		/* Read the total values of power after reset */
    MERCURY_CMD_GET_LAST_POWER_OFF = 0xB,	/* Read the time of last power off */
    MERCURY_CMD_GET_LAST_POWER_ON = 0x0C,	/* Read the time of last power on */
    MERCURY_CMD_GET_HOLIDAYS = 0x0D,		/* Read the table of holidays */
    MERCURY_CMD_GET_SCHEDULE = 0x0E,		/* Read the schedule of tariffs */
    MERCURY_CMD_GET_VALUE = 0x0F,		/* Read the month's value */
    MERCURY_CMD_GET_NUM_TARIFFS = 0x10,		/* Read the number of tariffs */
    MERCURY_CMD_SET_NUM_TARIFFS = 0x11,		/* Set number of tariffs */
    MERCURY_CMD_SET_TARIFF = 0x12,		/* Set the tariff */
    MERCURY_CMD_SET_HOLIDAYS = 0x13,		/* Set the table of holidays */
    MERCURY_CMD_SET_SCHEDULE = 0x14,		/* Set the schedule of tariffs */
    MERCURY_CMD_GET_WORKING_TIME = 0x15,	/* Read the total working time of battery and device */
		MERCURY_CMD_SET_TIMEDATE = 0x16,	/* Set the internal time */
} mercury_cmd_t;

typedef enum {
		ALL_YEAR = 0x0F,
		
		ALL_DAYS = 0x0F,
		WEEKDAYS = 0x0E,
		WEEKENDS = 0x0D,
		HOLIDAYS = 0x0C,
} mercury_scheduler_t;

static char str_dow[8][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Hol" };

void umdk_mercury_command(char *param, char *out, int bufsize) {
	uint32_t destination;
		
	if (strstr(param, "set address ") == param) {
		param += strlen("set address ");    // Skip command
		uint32_t new_address = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		uint32_to_le(&new_address);
		snprintf(out, bufsize, "%02x%08x%08x", MERCURY_CMD_SET_NEW_ADDR, destination, new_address);
	}
	else if (strstr(param, "get serial ") == param) {
		param += strlen("get serial ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x", MERCURY_CMD_GET_SERIAL, destination);
	}
	else if (strstr(param, "get total ") == param) { 
		param += strlen("get total ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x", MERCURY_CMD_GET_TOTAL_VALUE, destination);
	}
	else if (strstr(param, "get value ") == param) { 
		param += strlen("get value ");    // Skip command
		uint8_t month = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x%02x", MERCURY_CMD_GET_VALUE, destination, month);
	}
	else if (strstr(param, "get current ") == param) { 
		param += strlen("get current ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x0F", MERCURY_CMD_GET_VALUE, destination);
	}
	else if (strstr(param, "get schedule ") == param) { 
		param += strlen("get schedule "); // skip command
		uint8_t month = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint8_t dow = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		uint8_t date = (uint8_t)((month << 4) + (dow << 0));
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x%02x", MERCURY_CMD_GET_SCHEDULE, destination, date);
	}
	else if (strstr(param, "get timedate ") == param) { 
		param += strlen("get timedate ");    // Skip command
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		snprintf(out, bufsize, "%02x%08x", MERCURY_CMD_GET_TIMEDATE, destination);
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
														MERCURY_CMD_SET_TIMEDATE, destination, dow, hour, min, sec, day, month, year);
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
			month = (uint8_t)ALL_YEAR;
		}
		else if (strstr(param, "month ") == param) {
			param += strlen("month ");				// Skip command
			month = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
		}
	
		if(strstr(param, "day ") == param) {
			param += strlen("day ");				// Skip command
			day = strtol(param, &param, 10);
		}
		else if(strstr(param, "all") == param) {
			param += strlen("all");				// Skip command
			day = (uint8_t)ALL_DAYS;
		}
		else if(strstr(param, "weekdays") == param) {
			param += strlen("weekdays");				// Skip command			
			day = (uint8_t)WEEKDAYS;
		}
		else if(strstr(param, "weekends") == param) {
			param += strlen("weekends");				// Skip command			
			day = (uint8_t)WEEKENDS;
		}
		else if(strstr(param, "holidays") == param) {
			param += strlen("holidays");				// Skip command			
			day = (uint8_t)HOLIDAYS;
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
			//tariff_hour[i] = (uint8_t)((tariff << 6) + ( ((((hour_tmp >> 4) & 0x3) * 10) + (hour_tmp & 0x0F)) << 0));
			param += strlen(" ");    						// Skip space
			min_tmp = strtol(param, &param, 10);
			min[i] = (uint8_t)(((min_tmp /10) << 4) + ((min_tmp %10) << 0));
		}
	
		param += strlen(" ");    						// Skip space	
		destination = strtol(param, &param, 10);
		uint32_to_le(&destination);
		
		date = (uint8_t)((month << 4) + (day << 0));
		
		uint8_t num_char;
		num_char = snprintf(out, bufsize, "%02x%08x", MERCURY_CMD_SET_SCHEDULE, destination);
		for(i = 0; i < sizeof(tariff_hour); i++) {
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x%02x", tariff_hour[i], min[i]);
		}
		snprintf(out + num_char, bufsize - num_char, "%02x", date);
	}
	else if (strstr(param, "add ") == param) { 
		param += strlen("add ");    // Skip command
		uint8_t number = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		uint32_t add_address = strtol(param, &param, 10);
		uint32_to_le(&add_address);
		snprintf(out, bufsize, "%02x%02x%08x", MERCURY_CMD_ADD_ADDR, number, add_address);
	}
	else if (strstr(param, "remove ") == param) { 
		param += strlen("remove ");    // Skip command
		uint8_t number = strtol(param, &param, 10);
		snprintf(out, bufsize, "%02x%02x", MERCURY_CMD_REMOVE_ADDR, number);
	}
	else if (strstr(param, "reset") == param) { 
		snprintf(out, bufsize, "%02x", MERCURY_CMD_RESET);
	}
}

bool umdk_mercury_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
		char buf[100];
		
		if (moddatalen == 1) {
			if (moddata[0] == 1) {
				add_value_pair(mqtt_msg, "Msg", "Ok");
			} 
			else {
				add_value_pair(mqtt_msg, "Msg", "Error");
			}
			return true;
		}
		
		char buf_addr[20];
		uint32_t *address = (uint32_t *)(&moddata[0]);
		uint32_to_le(address);
		snprintf(buf_addr, sizeof(buf_addr), "%u", *address);	
		
		add_value_pair(mqtt_msg, "Address", buf_addr);
						
    if (moddatalen == 5) {
        if (moddata[4] == 1) {
            add_value_pair(mqtt_msg, "Msg", "Ok");
        } else {
            add_value_pair(mqtt_msg, "Msg", "Error");
        }
        return true;
    }
		
	mercury_cmd_t cmd = moddata[4];	
  
	uint8_t i;
	uint32_t * ptr_value;
	
	switch(cmd) {
		case MERCURY_CMD_GET_SERIAL: {
			uint32_t *serial = (uint32_t *)(&moddata[5]);
			uint32_to_le(serial);      
			snprintf(buf, sizeof(buf), "%u", *serial);
			add_value_pair(mqtt_msg, "Serial number", buf);		
			return true;
			break;
		}
		
		case MERCURY_CMD_GET_TOTAL_VALUE: {
			uint32_t value[5] = { 0 };
			for(i = 0; i < 5; i++) {
				ptr_value = (uint32_t *)(&moddata[4*i + 5]);
				uint32_to_le(ptr_value);      
				value[i] = *ptr_value;
			}
			 		
			char tariff[5] = { };
			for(i = 0; i < 4; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d", i);		
				snprintf(buf, sizeof(buf), "%u.%u", value[i]/100, value[i]%100);
				add_value_pair(mqtt_msg, tariff, buf);								
			}
			snprintf(buf, sizeof(buf), "%u.%u", value[4]/100, value[4]%100);
			add_value_pair(mqtt_msg, "Total", buf);		
			
			return true;
			break;
		}

		case MERCURY_CMD_GET_VALUE: {
			uint32_t value[5] = { 0 };
			for(i = 0; i < 5; i++) {
				ptr_value = (uint32_t *)(&moddata[4*i + 5]);
				uint32_to_le(ptr_value);      
				value[i] = *ptr_value;
			}
			
			char tariff[5] = { };
			for(i = 0; i < 4; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d", i);
				snprintf(buf, sizeof(buf), "%u.%u", value[i]/100, value[i]%100);
				add_value_pair(mqtt_msg, tariff, buf);				
			}
			snprintf(buf, sizeof(buf), "%u.%u", value[4]/100, value[4]%100);
			add_value_pair(mqtt_msg, "Total", buf);		
	
			return true;
			break;
		}		
		
		case MERCURY_CMD_GET_SCHEDULE: {
			
			char tariff[5] = { };
			for(i = 0; i < moddatalen; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d", moddata[3*i + 5] + 1);		
				snprintf(buf, sizeof(buf), "%02d:%02d",  moddata[3*i + 6],  moddata[3*i + 7]);
				add_value_pair(mqtt_msg, tariff, buf);			
			}
			
			return true;
			break;
		}		
		
		case MERCURY_CMD_GET_TIMEDATE: {
			
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
