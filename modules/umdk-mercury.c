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
    MERCURY_CMD_INIT_ADDR = 0xFF,		/* Set address at initialization */

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
} mercury_cmd_t;

void umdk_mercury_command(char *param, char *out, int bufsize) {
	
	if (strstr(param, "set address ") == param) {
		param += strlen("set address ");    // Skip command
		uint32_t address = strtol(param, &param, 10);
		
		snprintf(out, bufsize, "02%08x", address);
	}
	else if (strstr(param, "get serial") == param) {
		snprintf(out, bufsize, "01");
	}
	else if (strstr(param, "get total") == param) { 
		snprintf(out, bufsize, "0A");
	}
	else if (strstr(param, "get value ") == param) { 
	  param += strlen("get value "); // skip command
		uint8_t month = strtol(param, &param, 10);
		
		snprintf(out, bufsize, "0F%02x", month);
	}
	else if (strstr(param, "get value curr") == param) { 
		snprintf(out, bufsize, "0F0F");
	}
	else if (strstr(param, "get schedule ") == param) { 
		param += strlen("get schedule "); // skip command
		uint8_t month = strtol(param, &param, 10);
		uint8_t dow = strtol(param, NULL, 10);

		snprintf(out, bufsize, "0E%01x%01x", month, dow);
	}
}

bool umdk_mercury_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

    if (moddatalen == 1) {
        if (moddata[0] == 0) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else {
            add_value_pair(mqtt_msg, "msg", "error");
        }
        return true;
    }

	mercury_cmd_t cmd = moddata[0];
	
	switch(cmd) {
		case : MERCURY_CMD_GET_SERIAL {
			uint32_t serial = (moddata[1] << 24) + (moddata[2] << 16) + (moddata[3] << 8) + (moddata[4] << 0); 
			snprintf(buf, sizeof(buf), "%d", serial);
			add_value_pair(mqtt_msg, "Serial number: ", buf);		
			return true;
			break;
		}
		
		case : MERCURY_CMD_GET_TOTAL_VALUE {
			uint32_t value[5] = { 0 };
			for(uint8_t i = 0; i < 5; i++) {
				value[i] = (moddata[4*i + 1] << 24) + (moddata[4*i + 2] << 16) + (moddata[4*i + 3] << 8) + (moddata[4*i + 4] << 0);
			}
			 
			char tariff[5] = { };
			for(uint8_t i = 0; i < 4; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d:", i);
				snprintf(buf, sizeof(buf), "%u", values[i]);
				add_value_pair(mqtt_msg, tariff, buf);				
			}
			snprintf(buf, sizeof(buf), "%u", value[4]);
			add_value_pair(mqtt_msg, "Summary: ", buf);		
			
			return true;
			break;
		}

		case : MERCURY_CMD_GET_VALUE {
			uint32_t value[5] = { 0 };
			for(uint8_t i = 0; i < 5; i++) {
				value[i] = (moddata[4*i + 1] << 24) + (moddata[4*i + 2] << 16) + (moddata[4*i + 3] << 8) + (moddata[4*i + 4] << 0);
			}
			 
			char tariff[5] = { };
			for(uint8_t i = 0; i < 4; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d:", i);
				snprintf(buf, sizeof(buf), "%u", values[i]);
				add_value_pair(mqtt_msg, tariff, buf);				
			}
			snprintf(buf, sizeof(buf), "%u", value[4]);
			add_value_pair(mqtt_msg, "Summary: ", buf);		
	
			return true;
			break;
		}		
		
		case : MERCURY_CMD_GET_SCHEDULE {
			
			char tariff[5] = { };
			for(uint8_t i = 0; i < moddatalen; i++) {
				snprintf(tariff, sizeof(tariff), "T%02d ", moddata[3*i + 1] + 1);		
				snprintf(buf, sizeof(buf), "%02d:%02d ",  moddata[3*i + 2],  moddata[3*i + 3]);
				add_value_pair(mqtt_msg, tariff, buf);			
			}
			
			return true;
			break;
		}		
		
		default:
			break;
	}
	return true;
}
