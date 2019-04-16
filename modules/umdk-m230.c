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
    M230_CMD_RESET 			= 0xFE,		/* Clear database */
    M230_CMD_ADD_ADDR 			= 0xFD,		/* Add address  in database */
    M230_CMD_REMOVE_ADDR 		= 0xFC,		/* Remove address from database */
	M230_CMD_GET_LIST 			= 0xFB,		/* Send database of addresses */

	M230_CMD_SET_IFACE			= 0xFA,		/* Set interfase => CAN or RS485 */
	
	M230_CMD_SET_HOLIDAYS_M_FULL	= 0xF2,		/* Set the list of holidays into memory */	
	M230_CMD_SET_SCHEDULE_M_FULL	= 0xF1,		/* Set the schedule of tariffs into memory */
    
    M230_CMD_PROPRIETARY_COMMAND	= 0xF0,	/* Less this value single command of mercury */

    M230_CMD_TEST_RESPONSE			= 0x00,		/* Testing channel - response from device */
	M230_CMD_OPEN_LINE				= 0x01,		/* Open a communication channel */
	M230_CMD_CLOSE_LINE			= 0x02,		/* Close a communication channel */
		
	M230_CMD_GET_VALUE				= 0x03,		/* Read the values of power */
	M230_CMD_GET_TIMEDATE			= 0x04,		/* Read the internal time and date */
	M230_CMD_GET_SERIAL			= 0x05,		/* Read serial number of the device */
	M230_CMD_SET_TIMEDATE			= 0x06,		/* Set the internal time and date */
	M230_CMD_SET_LOAD				= 0x07,		/* Set on/off control of load */
	M230_CMD_GET_LOAD				= 0x08,		/* Read mode of control of load */	
	M230_CMD_SET_SCHEDULE			= 0x09,		/* Set the schedule of tariffs */
	M230_CMD_SET_LIMIT_POWER		= 0x0A,		/* Set the value of the active power limit */
	M230_CMD_SET_MODE_LIMIT_POWER	= 0x0B,		/* Set on/off the active power limit */
	M230_CMD_SET_LIMIT_ENERGY		= 0x0C,		/* Set the value of the active energy limit */
	M230_CMD_SET_MODE_LIMIT_ENERGY	= 0x0D,		/* Set on/off the active energy limit */
	M230_CMD_SET_MODE_TARIFF		= 0x0E,		/* Set mode of tariffs */	
	M230_CMD_GET_MODE_TARIFF		= 0x0F,		/* Get mode of tariffs */	
	M230_CMD_GET_ERROR_STATUS		= 0x10,		/* Get status of errors */		
	M230_CMD_GET_VERSION			= 0x11,		/* Read the version of device */	
	M230_CMD_SET_MODE_PULSE_OUT	= 0x12,		/* Changing the pulse output mode */	
	M230_CMD_GET_SOFTWARE			= 0x13,		/* Read the version software of device */	
	M230_CMD_GET_SCHEDULE			= 0x14,		/* Read the schedule of tariffs */
	M230_CMD_GET_INFO				= 0x15,		/* Read the info of device */
	M230_CMD_GET_HOLIDAYS			= 0x16,		/* Read holidays */
	M230_CMD_GET_STATUS_LONG_CMD	= 0x17,		/* Read status of the long-time operations */
	M230_CMD_GET_POWER_LIMIT		= 0x18,		/* Read the power limit */
	M230_CMD_GET_ENERGY_LIMIT		= 0x19,		/* Read the energy limit */
	M230_CMD_GET_SCHEDULE_M		= 0x1A,		/* Read the schedule of tariffs  from memory */
	M230_CMD_SET_SCHEDULE_M		= 0x1B,		/* Set the schedule of tariffs into memory */
	M230_CMD_GET_HOLIDAYS_M		= 0x1C,		/* Read the list of holidays from memory */
	M230_CMD_SET_HOLIDAYS_M		= 0x1D,		/* Set the list of holidays into memory */
} m230_cmd_t;

typedef enum {
	M230_MASK_ALL_YEAR = 0x0FFF,
	M230_MASK_ALL_DAYS = 0xFF,
	M230_MASK_WEEKDAYS = 0x1F,
	M230_MASK_WEEKENDS = 0x60,
	M230_MASK_HOLIDAYS = 0x80,
} m230_mask_scheduler_t;

typedef enum {
    M230_ALL_YEAR = 0x0F,
    
    M230_ALL_DAYS = 0x0F,
    M230_WEEKDAYS = 0x0E,
    M230_WEEKENDS = 0x0D,
    M230_HOLIDAYS = 0x0C,
} m230_scheduler_t;

typedef enum {
    M230_OK_REPLY 			= 0x00,
	M230_ERROR_REPLY 		= 0xF0,
    M230_NO_RESPONSE_REPLY 	= 0xF2,
	
	M230_ERROR_NOT_FOUND	= 0xF3,
	M230_ERROR_OFFLINE		= 0xF4,
	M230_WAIT_REPLY			= 0xF5,
	
	M230_ERROR_CMD			= 0xFF,
} m230_reply_t;

typedef enum {
	M230_OK 			= 0x00,
	M230_WRONG_CMD 		= 0x01,
    M230_INTERNAL_ERROR = 0x02,
	M230_ACCESS_ERROR 	= 0x03,
	M230_TIME_CORRECTED	= 0x04,
	M230_OFFLINE 		= 0x05,
} m230_status_byte_t;

#define M230_BEGIN_ADDR_SCHEDULE 0x1000
#define M230_OFFSET_SCHEDULE 0x11
#define UMDK_M230_BEGIN_ADDR_HOLIDAYS 0x1D00
#define UMDK_M230_OFFSET_HOLIDAYS 5

static char season[2][7] = { "Summer", "Winter" };
static char dow[8][4] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Hol" };

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
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
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
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
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
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
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
	else if (strstr(param, "set power_limit ") == param) {
		param += strlen("set power_limit ");    // Skip command
		uint32_t limit = 0;
		limit = strtol(param, &param, 10);
		limit = limit * 100;
		limit = limit & 0x00FFFFFF;
		uint8_t limit_8 = (uint8_t)(limit >> 16);
		uint16_t limit_16 = (uint16_t)(limit & 0xFFFF);

		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2C;
		snprintf(out, bufsize, "%02x%02x%02x%02x%04x", M230_CMD_SET_LIMIT_POWER, destination, tmp, limit_8, limit_16);
	}		
	else if (strstr(param, "set mode_power_limit ") == param) {
		param += strlen("set mode_power_limit ");    // Skip command
		uint8_t mode;
		if (strstr(param, "on") == param) { 
			param += strlen("on");    // Skip command
			mode = 1;
		}
		else if (strstr(param, "off") == param) { 	
			param += strlen("off");    // Skip command
			mode = 0;	
		}
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
		}		
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2D;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_MODE_LIMIT_POWER, destination, tmp, mode);
	}		
	else if (strstr(param, "get power_limit ") == param) {
		param += strlen("get power_limit ");    // Skip command
		
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x19;
		snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_GET_POWER_LIMIT, destination, tmp);
	}		
	else if (strstr(param, "set energy_limit ") == param) {
		param += strlen("set energy_limit ");    // Skip command
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
	else if (strstr(param, "set mode_energy_limit ") == param) {
		param += strlen("set mode_energy_limit ");    // Skip command
		uint8_t mode;
		if (strstr(param, "on") == param) { 
			param += strlen("on");    // Skip command
			mode = 1;
		}
		else if (strstr(param, "off") == param) { 	
			param += strlen("off");    // Skip command
			mode = 0;	
		}
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
		}
		
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x2F;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_MODE_LIMIT_ENERGY, destination, tmp, mode);
	}			
	else if (strstr(param, "get energy_limit ") == param) {
		param += strlen("get energy_limit ");    // Skip command
		uint8_t tariff = 0;
		tariff = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space
		destination = strtol(param, &param, 10);
		
		uint8_t tmp = 0x1A;
		snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_GET_ENERGY_LIMIT, destination, tmp, tariff);
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
		uint16_t addr_holidays = UMDK_M230_BEGIN_ADDR_HOLIDAYS;
		uint8_t num_bytes = 4;
		uint8_t tmp = 0x02;
		
		month = strtol(param, &param, 10);
		month--;
		addr_holidays = UMDK_M230_BEGIN_ADDR_HOLIDAYS + month * UMDK_M230_OFFSET_HOLIDAYS;
		param += strlen(" ");    						// Skip space				
		destination = strtol(param, &param, 10);
		snprintf(out, bufsize, "%02x%02x%02x%04x%02x", M230_CMD_GET_HOLIDAYS_M, destination, tmp, addr_holidays, num_bytes);
	}			
	else if (strstr(param, "set holidays ") == param) {
		param += strlen("set holidays ");    // Skip command
		uint16_t month = 0;
		uint8_t month_tmp = 0;
		uint32_t day[12];
		uint8_t day_tmp = 0;
		uint8_t flag_end  = 0;
		uint8_t tmp = 0x02;
		uint8_t i;
		
		for(i = 0; i < 12; i++) {
			day[i] = 0;
		}	
		
		while(flag_end == 0) {
			day_tmp = strtol(param, &param, 10);		
			day_tmp--;
			param += strlen(" ");    						// Skip space	
		
			month_tmp = strtol(param, &param, 10);
			month_tmp--;
			month |= 1 << month_tmp;
			
			day[month_tmp] |= 1 << day_tmp;
			
			if(strstr(param, ", ") == param) {
				param += strlen(", ");
			}
			else if (strstr(param, " ") == param) {
				param += strlen(" ");
				destination = strtol(param, &param, 10);
				flag_end = 1;
			}
			else {
				snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
				return;
			}		
		}
		
		uint16_t num_char;		
		num_char = snprintf(out, bufsize, "%02x%02x%02x", M230_CMD_SET_HOLIDAYS_M_FULL, destination, tmp);
		
		for(i = 0; i < 12; i++) {
			if(day[i] != 0) {
				num_char += snprintf(out + num_char, bufsize - num_char, "%02x%08x", i + 1, day[i]);
			}
		}
	}		
	else if (strstr(param, "get schedule ") == param) {
		param += strlen("get schedule ");    // Skip command
		uint8_t half = 0;
		uint16_t month = 0;
		uint8_t day = 0;
		uint16_t addr_schedule = M230_BEGIN_ADDR_SCHEDULE;
		uint8_t offset = 0;
		uint8_t num_bytes = 0x10;
		uint8_t tmp = 0x02;
		
		month = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space		
		day = strtol(param, &param, 10);
		param += strlen(" ");    						// Skip space		
		
		destination = strtol(param, &param, 10);

		offset = 8 * (month - 1) + (day - 1);
		
		addr_schedule = M230_BEGIN_ADDR_SCHEDULE + M230_OFFSET_SCHEDULE * (half + 2 * offset);		
		
		snprintf(out, bufsize, "%02x%02x%02x%04x%02x", M230_CMD_GET_SCHEDULE_M, destination, tmp, addr_schedule, num_bytes);
	}		
	else if (strstr(param, "set schedule ") == param) {
		param += strlen("set schedule ");    // Skip command
		
		uint8_t i = 0;
		uint16_t point_schedule[8] = { 0x0038 };		
		// uint8_t half = 0;
		uint8_t month = 0;
		uint8_t day = 0;
		uint8_t hour_tmp;
		uint8_t tariff;
		uint8_t min_tmp;
		uint8_t tmp = 0x02;
		uint8_t date = 0;
		
		for(i = 0; i < 8; i++) {
			point_schedule[i] = 0x0038;
		}	
		if(strstr(param, "year ") == param) {
			param += strlen("year ");				// Skip command
			month = (uint16_t)M230_ALL_YEAR;
		}
		else if (strstr(param, "month ") == param) {
			param += strlen("month ");				// Skip command
			month = strtol(param, &param, 10);
			// month--;
			param += strlen(" ");    						// Skip space
		}
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
		}		
		
		if(strstr(param, "day ") == param) {
			param += strlen("day ");				// Skip command
			day = strtol(param, &param, 10);
			// day--;
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
		else {
			snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
			return;
		}		

		param += strlen(" ");    						// Skip space
		uint8_t checkpoint = strtol(param, &param, 10);

		for(i = 0; i < checkpoint; i++) {
			param += strlen(" ");    						// Skip space
			tariff = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
			hour_tmp = strtol(param, &param, 10);
			param += strlen(":");    						// Skip space
			min_tmp = strtol(param, &param, 10);
			
			point_schedule[i] = ((min_tmp << 8) + (tariff << 5) + (hour_tmp << 0));
		}
	
		param += strlen(" ");    						// Skip space	
		destination = strtol(param, &param, 10);		
		
		date = (uint8_t)((month << 4) + (day << 0));
		
		uint8_t num_char;
		num_char = snprintf(out, bufsize, "%02x%02x%02x%02x", M230_CMD_SET_SCHEDULE_M_FULL, destination, tmp, date);
		for(i = 0; i < 8; i++) {
			num_char += snprintf(out + num_char, bufsize - num_char, "%04x", point_schedule[i]);
		}
		
	}
	else if (strstr(param, "iface ") == param) { 
		param += strlen("iface ");    // Skip command	
		uint8_t interface = 0;
		if (strstr(param, "can") == param) { 	
			interface = 2;
		}
		else if (strstr(param, "485") == param) { 	
			interface = 1;		
		}
		
		snprintf(out, bufsize, "%02x%02x", M230_CMD_SET_IFACE, interface);
	}
	else {
		snprintf(out, bufsize, "%02x", M230_ERROR_CMD);
		return;
	}
}

bool umdk_m230_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	char buf[100];
    char strbuf[20];
	char buf_addr[30];
		
	// uint8_t ii;
    // printf("[m230] RX data:  ");
    // for(ii = 0; ii < moddatalen; ii++) {
        // printf(" %02X ", moddata[ii]);
    // }
   // puts("\n");
	
	
   if (moddatalen == 1) {
		if (moddata[0] == M230_OK_REPLY) {
			add_value_pair(mqtt_msg, "msg", "ok");
		} 
		else if(moddata[0] == M230_ERROR_REPLY){
			add_value_pair(mqtt_msg, "msg", "error");
		}
		else if(moddata[0] == M230_ERROR_CMD){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		}
		return true;
	}	
	
	m230_cmd_t cmd = moddata[0];
	
	// if(cmd < M230_CMD_PROPRIETARY_COMMAND) {
		
		uint8_t address = moddata[2];

		snprintf(buf_addr, sizeof(buf_addr), "%u", address);	
		add_value_pair(mqtt_msg, "Address", buf_addr);
							
		if (moddatalen == 3) {
			if (moddata[1] == M230_OK_REPLY) {
				add_value_pair(mqtt_msg, "msg", "ok");
			} else if(moddata[1] == M230_ERROR_REPLY){
				add_value_pair(mqtt_msg, "msg", "error");
			} else if(moddata[1] == M230_NO_RESPONSE_REPLY){
				add_value_pair(mqtt_msg, "msg", "no response");					
			} else if(moddata[1] == M230_ERROR_NOT_FOUND){
				add_value_pair(mqtt_msg, "msg", "device not found");					
			} else if(moddata[1] == M230_ERROR_OFFLINE){
				add_value_pair(mqtt_msg, "msg", "offline");					
			} else if(moddata[1] == M230_WRONG_CMD){
				add_value_pair(mqtt_msg, "msg", "invalid parameter");							
			} else if(moddata[1] == M230_INTERNAL_ERROR){
				add_value_pair(mqtt_msg, "msg", "internal error");							
			} else if(moddata[1] == M230_ACCESS_ERROR){
				add_value_pair(mqtt_msg, "msg", "access error");					
			} else if(moddata[1] == M230_TIME_CORRECTED){
				add_value_pair(mqtt_msg, "msg", "time already corrected");
			} else if(moddata[1] == M230_OFFLINE){
				add_value_pair(mqtt_msg, "msg", "offline");					
			} else if(moddata[1] == M230_OK){
				add_value_pair(mqtt_msg, "msg", "ok");					
			} else if(moddata[1] == M230_WAIT_REPLY){
				add_value_pair(mqtt_msg, "msg", "please wait");
			}
			return true;
		// }
		
	}
	
    int i = 0;
	
	switch(cmd) {
		
		case M230_CMD_GET_VALUE: {
			uint32_t value[4] = { 0 };
						
			memset(value, 0xFF, sizeof(value));	
						
            for (i = 0; i < 4; i++) {
				value[i] = (moddata[4*i + 3] << 24) + (moddata[4*i + 2] << 16) + (moddata[4*i + 5] << 8)  + (moddata[4*i + 4] << 0);
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
				add_value_pair(mqtt_msg, "A-", "N/A");									
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
				add_value_pair(mqtt_msg, "R-", "N/A");					
			}
			
			return true;
			break;
		}		
		
		case M230_CMD_GET_POWER_LIMIT: {			
			uint32_t power_limit = ((moddata[2] << 16) + (moddata[4] << 8) + moddata[3]) &  0x00FFFFFF; 

			int_to_float_str(strbuf, power_limit, 2);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, "Power limit", buf);
			
			return true;
			break;			
		}
		
		case M230_CMD_GET_ENERGY_LIMIT: {			
			uint32_t energy_limit = (moddata[3] << 24) + (moddata[2] << 16) + (moddata[5] << 8) + moddata[4]; 
			
			int_to_float_str(strbuf, energy_limit, 3);
			snprintf(buf, sizeof(buf), "%s", strbuf);
			add_value_pair(mqtt_msg, "Energy limit", buf);
			
			return true;
			break;			
		}
		
		case M230_CMD_GET_TIMEDATE: {			
			char time_buf[10] = { };
			uint8_t time[8] = { 0 };

            for(i = 0; i < (moddatalen - 2); i++) {
                time[i] = (moddata[i + 2] >> 4) * 10;
                time[i] += (moddata[i + 2] & 0x0F) * 1;
            }

			time[3]--;
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
				add_value_pair(mqtt_msg, "Energy limit control", "Allowed");
			}
			else if(mode_limit_energy == 0) {
				add_value_pair(mqtt_msg, "Energy limit control", "Not allowed");				
			}		

			if(mode_limit_power == 1) {
				add_value_pair(mqtt_msg, "Power limit control", "Allowed");
			}
			else if(mode_limit_power == 0) {
				add_value_pair(mqtt_msg, "Power limit control", "Not allowed");				
			}						
			
			if(mode_load == 1) {
				add_value_pair(mqtt_msg, "Pulse output mode", "Load");
			}
			else if(mode_load == 0) {
				add_value_pair(mqtt_msg, "Pulse output mode", "Telemetry");				
			}			
			
			if(powerload_on_off == 1) {
				add_value_pair(mqtt_msg, "Load control", "Off");
			}
			else if(powerload_on_off == 0) {
				add_value_pair(mqtt_msg, "Load control", "On");				
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
			uint8_t flag_error = 0;
			
			for(i = 0; i < 6; i++) {
				status = moddata[i + 2];
				for(j = 0; j < 8; j++) {
					error = (status >> j) & 1;
					if(error == 1) {
						flag_error = 1;
						snprintf(buf_addr, sizeof(buf_addr), "E-%02d", 8*i + j + 1);	
						add_value_pair(mqtt_msg, "Error", buf_addr);												
					}
				}
			}
			
			if(flag_error == 0) {
				add_value_pair(mqtt_msg, "Error", "No");	
			}
			
			return true;
			break;
		}
		
		case M230_CMD_GET_VERSION: {
			
			return true;
			break;			
		}
		
		case M230_CMD_GET_SOFTWARE: {
						
			uint8_t soft[3] = { 0 };
		
			soft[0] = ( moddata[2]  >> 4 ) * 10;
			soft[0] += ( moddata[2] & 0x0F ) * 1;
			
			soft[1] = ( moddata[3] >> 4 ) * 10;
			soft[1] += ( moddata[3] & 0x0F ) * 1;
			
			soft[2] = ( moddata[4] >> 4 ) * 10;
			soft[2] += ( moddata[4] & 0x0F ) * 1;
			
			snprintf(buf, sizeof(buf), "%02d.%02d.%02d", soft[0], soft[1],soft[2]);	
			add_value_pair(mqtt_msg, "Software version", buf);
			
			return true;
			break;			
		}
		
		case M230_CMD_GET_SCHEDULE_M: {
			uint8_t tariff = 0;
			uint8_t hour = 0;
			uint8_t min = 0;
			uint8_t i = 0;
			char tariff_str[5] = { };
			uint16_t point_schedule = 0;
			for(i = 0; i < 8; i++) {
				point_schedule = (moddata[2*i + 2] << 8) + moddata[2*i + 3];
				if(point_schedule != 0x0038) {
					min = moddata[2*i + 2];
					tariff = (moddata[2*i + 3] >> 5) & 0x07;
					hour = moddata[2*i + 3] & 0x1F;
					
					snprintf(tariff_str, sizeof(tariff_str), "T%02d", tariff);
					snprintf(buf, sizeof(buf), "%02d:%02d",  hour, min);
					add_value_pair(mqtt_msg, tariff_str, buf);
				}
			}
			
			break;									
			
		}
		
		case M230_CMD_GET_HOLIDAYS_M: {
			uint8_t i, j = 0;
			uint8_t day = 0;
			uint8_t week = 0;
			uint8_t flag_holiday = 0;

			uint8_t num_char = 0;

			num_char = snprintf(buf, sizeof(buf), "[ ");
			for(i = 0; i < 4; i++) {
				week = moddata[i + 2];
				for(j = 0; j < 8; j++) {
					day = (week >> j) & 1;
					if(day == 1) {
						flag_holiday = 1;
						num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "%02d, ", 8*i + j + 1);						
					}
				}
			}
			num_char += snprintf(buf + num_char, sizeof(buf) - num_char, "]");
			
			if(flag_holiday == 1) {
				add_value_pair(mqtt_msg, "Holidays", buf);
			}			
			else if(flag_holiday == 0) {
				add_value_pair(mqtt_msg, "Holidays", "None");	
			}			
			
			break;					
		}
		
		case M230_CMD_GET_INFO: {
			
			break;						
		}
		
		case M230_CMD_GET_HOLIDAYS: {
			
			break;						
		}
		
		case M230_CMD_GET_STATUS_LONG_CMD: {
			
			break;					
		}
				
		default:
			break;
	}
	return true;
}
