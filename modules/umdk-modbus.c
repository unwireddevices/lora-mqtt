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
 * @file	umdk-modbus.c
 * @brief   umdk-modbus message parser
 * @author  Mikhail Perkov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

#define MODBUS_DEBUG 1

typedef enum {
	MODBUS_SET_PARAMS 	= 0xFF,
	MODBUS_SET_DEVICE	= 0xFE,
	MODBUS_MAX_CMD		= 0x7F,

} umdk_modbus_cmd_t;

typedef enum {
	ILLEGAL_FUNCTION 	= 0x01,
	ILLEGAL_ADDRESS 	= 0x02,
	ILLEGAL_VALUE 		= 0x03,
	DEVICE_FAILURE 		= 0x04,
	ACK 				= 0x05,
	DEVICE_BUSY 		= 0x06,	
	NAK 				= 0x07,	
	MEMORY_ERROR 		= 0x08,
	GATEWAY_UNAVAILABLE = 0x0A,
	GATEWAY_FAILED 		= 0x0B,
} modbus_exceptions_t;

typedef enum {
	MODBUS_OK_REPLY 			= 0x00,
	MODBUS_ERROR_REPLY			= 0x01,
	MODBUS_NO_RESPONSE_REPLY   	= 0x02,
	MODBUS_OVERFLOW_REPLY   	= 0x03,
    MODBUS_INVALID_CMD_REPLY	= 0xFF,
} modbus_reply_t;

void umdk_modbus_command(char *param, char *out, int bufsize) {
    if (strstr(param, "send ") == param) {
        uint8_t bytes[100] = {};
        char *hex = param + strlen("send "); // Skip command

        if (!hex_to_bytes(hex, bytes, true)) {
            return;
        }

        snprintf(out, bufsize, "%s", hex);
    }
    else if (strstr(param, "set ") == param) {
        param += strlen("set "); // Skip commands
		if (strstr(param, "device ") == param) {
			param += strlen("device "); // Skip commands
			
			uint8_t device = strtol(param, &param, 10);
			snprintf(out, bufsize, "%02x%02x", MODBUS_SET_DEVICE, device);
		}
		else if (strstr(param, "mode ") == param) {
			param += strlen("mode "); // Skip commands
			if (strlen(param) > strlen("115200-8N1")) {
				return;
			}

			snprintf(out, bufsize, "%02x", MODBUS_SET_PARAMS);
			
			/* convert string to hex */
			uint8_t k;
			char tmpbuf[5];
			for (k = 0; k < strlen(param); k++) {
				snprintf(tmpbuf, sizeof(tmpbuf), "%02x", param[k]);
				strcat(out, tmpbuf);
				//snprintf(out + 4 + 2*k, 3, "%02x", param[k]);
			}
			
			printf("MODBUS mode: %s\n", param);
		}
		else {
			snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
			return;
		}
    }
    else {
		snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
		return;
	}
}

bool umdk_modbus_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{	
#if MODBUS_DEBUG	
	uint8_t ii;
    printf("[modbus] RX data:  ");
    for(ii = 0; ii < moddatalen; ii++) {
        printf(" %02X ", moddata[ii]);
    }
   puts("\n");
#endif
	
   if (moddatalen == 1) {
        if (moddata[0] == MODBUS_OK_REPLY) {
            add_value_pair(mqtt_msg, "msg", "ok");
        } else if(moddata[0] == MODBUS_ERROR_REPLY){
            add_value_pair(mqtt_msg, "msg", "error");
		} else if(moddata[0] == MODBUS_INVALID_CMD_REPLY){
			add_value_pair(mqtt_msg, "msg", "invalid command");
		}
        return true;
    }
	
	char buf[100] = { 0 };
    char buf_cmd[5] = { 0 };
	char buf_addr[5] = { 0 };
	char buf_exc[5] = { 0 };
	char hex[3] = { 0 };
	
	uint8_t addr = moddata[0];
    uint8_t cmd = moddata[1];

	snprintf(buf_addr, sizeof(buf_addr), "%d", addr);
	add_value_pair(mqtt_msg, "address", buf_addr);
										
	if (moddatalen == 2) {
		if (moddata[0] == MODBUS_OK_REPLY) {
			add_value_pair(mqtt_msg, "msg", "ok");
		} else if(moddata[0] == MODBUS_ERROR_REPLY){
			add_value_pair(mqtt_msg, "msg", "error");
		} else if(moddata[0] == MODBUS_NO_RESPONSE_REPLY){
			add_value_pair(mqtt_msg, "msg", "no response");
		} else if(moddata[0] == MODBUS_OVERFLOW_REPLY){
			add_value_pair(mqtt_msg, "msg", "rx buffer overflow");
		}
		return true;
	}
	
	if(cmd >= 0x80) {
		cmd = cmd - 0x80;
		snprintf(buf_cmd, sizeof(buf_cmd), "%02d", cmd);
		add_value_pair(mqtt_msg, "cmd", buf_cmd);	
		
		snprintf(buf_exc, sizeof(buf_exc), "%02d", moddata[2]);
		add_value_pair(mqtt_msg, "exception", buf_exc);
		return true;
	}
	
	snprintf(buf_cmd, sizeof(buf_cmd), "%02d", cmd);
	add_value_pair(mqtt_msg, "cmd", buf_cmd);	
	
	uint8_t i = 0;	
	for(i = 2; i < moddatalen; i++) {
		snprintf(hex, 3, "%02x", moddata[i]);
        strcat(buf, hex);
	}
	
	add_value_pair(mqtt_msg, "data", buf);

	return true;
    // return false;
}