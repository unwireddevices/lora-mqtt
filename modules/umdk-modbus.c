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

#define MODBUS_LENGTH_DATA_MAX 64
#define MODBUS_PACK_MAX 80

typedef enum {
	MODBUS_SET_PARAMS 	= 0xFF,
	MODBUS_SET_DEVICE	= 0xFE,
	MODBUS_MAX_CMD		= 0x7F,
	
	MODBUS_READ_DESCR_INP	= 0x02,
	MODBUS_READ_COILS		= 0x01,
	MODBUS_READ_HOLD_REG	= 0x03,
	MODBUS_READ_INP_REG		= 0x04,
	MODBUS_WRITE_SINGLE_COIL	= 0x05,
	MODBUS_WRITE_SINGLE_REG	= 0x06,
	MODBUS_READ_EXC_STATUS	= 0x07,
	MODBUS_READ_DIAGNOSTIC	= 0x08,	
	MODBUS_GET_COM_EVENT_CNT	= 0x0B,
	MODBUS_GET_COM_EVENT_LOG	= 0x0C,	
	MODBUS_WRITE_MULT_COILS	= 0x0F,
	MODBUS_WRITE_MULT_REG	= 0x10,
	MODBUS_READ_SERVER_ID	= 0x11,
	MODBUS_READ_FILE_REC	= 0x14,
	MODBUS_WRITE_FILE_REC	= 0x15,
	MODBUS_MASK_WRITE_REG	= 0x16,
	MODBUS_RW_MULT_REG		= 0x17,		/* Not realized */
	MODBUS_READ_FIFO_QUEUE	= 0x18,
	MODBUS_READ_DEV_INFO	= 0x2B,

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

void umdk_modbus_command(char *param, char *out, int bufsize) 
{
	uint8_t cmd = 0;
	uint8_t addr = 0;
	uint8_t tmp_8 = 0;
	uint16_t tmp_16 = 0;
	uint8_t data[MODBUS_LENGTH_DATA_MAX] = { 0 };
	uint8_t length = 0;
	uint16_t num_char = 0;
    uint8_t i = 0;
	uint8_t k = 0;
	
	if (strstr(param, "send ") == param) {
        uint8_t bytes[100] = {};
        char *hex = param + strlen("send "); // Skip command

        if (!hex_to_bytes(hex, bytes, true)) {
			snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
            return;
        }
		if((strlen(hex) / 2) >= MODBUS_PACK_MAX) {
			snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
            return;
		}

        snprintf(out, bufsize, "%s", hex);
		return;
    }
	else if (strstr(param, "pack ") == param) {
		param += strlen("pack "); // Skip commands
		
        uint8_t bytes_space[MODBUS_PACK_MAX] = {};		
		char *parametr_ptr = param;
		uint8_t length_total = (uint8_t)strlen(parametr_ptr);
		
		length_total++;
		length_total = length_total / 3;
		
		if(length_total >= MODBUS_PACK_MAX) {
			snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
			return;
		}
		
		for(i = 0; i < length_total; i++) {
			bytes_space[i] = strtol(parametr_ptr, &parametr_ptr, 16);
			parametr_ptr += strlen(" ");    						// Skip space
			num_char += snprintf(out + num_char, bufsize - num_char, "%02x", bytes_space[i]);
		}
		
		return;
    }
    else if (strstr(param, "set ") == param) {
        param += strlen("set "); // Skip commands
		if (strstr(param, "device ") == param) {
			param += strlen("device "); // Skip commands
			
			uint8_t device = strtol(param, &param, 16);
			snprintf(out, bufsize, "%02x%02x", MODBUS_SET_DEVICE, device);
			return;	
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
			}
			
			printf("MODBUS mode: %s\n", param);
			return;
		}
		else {
			snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
			return;
		}
    }
	else if(strstr(param, "read_di ") == param) {
		param += strlen("read_di "); // Skip commands
		
		cmd = MODBUS_READ_DESCR_INP;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "read_ao ") == param) {
		param += strlen("read_ao "); // Skip commands
		
		cmd = MODBUS_READ_COILS;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "read_hold_reg ") == param) {
		param += strlen("read_hold_reg "); // Skip commands
		
		cmd = MODBUS_READ_HOLD_REG;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "read_inp_reg ") == param) {
		param += strlen("read_inp_reg "); // Skip commands
		
		cmd = MODBUS_READ_INP_REG;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "write_coil ") == param) {
		param += strlen("write_coil "); // Skip commands
		
		cmd = MODBUS_WRITE_SINGLE_COIL;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "write_reg ") == param) {
		param += strlen("write_reg "); // Skip commands
		
		cmd = MODBUS_WRITE_SINGLE_REG;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "status ") == param) {
		param += strlen("status "); // Skip commands
		
		cmd = MODBUS_READ_EXC_STATUS;
		addr = strtol(param, &param, 16);
		
		length = 0;
	}
	else if(strstr(param, "diagnostic ") == param) {
		param += strlen("diagnostic "); // Skip commands
		
		cmd = MODBUS_READ_DIAGNOSTIC;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		length = 4;
	}
	else if(strstr(param, "event_cnt ") == param) {
		param += strlen("event_cnt "); // Skip commands
		
		cmd = MODBUS_GET_COM_EVENT_CNT;
		addr = strtol(param, &param, 16);
		
		length = 0;
	}
	else if(strstr(param, "event_log ") == param) {
		param += strlen("event_log "); // Skip commands
		
		cmd = MODBUS_GET_COM_EVENT_LOG;
		addr = strtol(param, &param, 16);
		
		length = 0;
	}
	else if(strstr(param, "write_mul_coils ") == param) {
		param += strlen("write_mul_coils "); // Skip commands
		
		cmd = MODBUS_WRITE_MULT_COILS;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[4] = tmp_8;
		length = 5;
		for(k = 0; k < data[4]; k++) {
			tmp_8 = strtol(param, &param, 16);
			param += strlen(" ");    						// Skip space
			data[5 + k] = tmp_8;
		}
		length += data[4];
	}
	else if(strstr(param, "write_mul_reg ") == param) {
		param += strlen("write_mul_reg "); // Skip commands
		
		cmd = MODBUS_WRITE_MULT_REG;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[4] = tmp_8;
		length = 5;
		
		for(k = 0; k < (data[4] / 2); k++) {
			tmp_16 = strtol(param, &param, 16);
			param += strlen(" ");    						// Skip space
			data[5 + 2*k] = (uint8_t)(tmp_16 >> 8); 
			data[6 + 2*k] = (uint8_t)(tmp_16 & 0xFF);
		}
		length += data[4];
	}
	else if(strstr(param, "server_id ") == param) {
		param += strlen("server_id "); // Skip commands
		
		cmd = MODBUS_READ_SERVER_ID;
		addr = strtol(param, &param, 16);
		
		length = 0;
	}
	else if(strstr(param, "read_file ") == param) {
		param += strlen("read_file "); // Skip commands
			/* at a time you can read one record from one file */
		cmd = MODBUS_READ_FILE_REC;	
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = tmp_8;
		length = 1;

		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[1] = tmp_8; 
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[4] = (uint8_t)(tmp_16 >> 8); 
		data[5] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[6] = (uint8_t)(tmp_16 >> 8); 
		data[7] = (uint8_t)(tmp_16 & 0xFF);			

		length += data[0];
	}
	else if(strstr(param, "write_file ") == param) {
		param += strlen("write_file "); // Skip commands
		/* at a time you can write one record to one file */
		cmd = MODBUS_WRITE_FILE_REC;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = tmp_8;
		length = 1;
		
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[1] = tmp_8; 
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[4] = (uint8_t)(tmp_16 >> 8); 
		data[5] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[6] = (uint8_t)(tmp_16 >> 8); 
		data[7] = (uint8_t)(tmp_16 & 0xFF);
		
		uint16_t length_tmp = tmp_16;
		
		for(k = 0; k < length_tmp; k++) {
			tmp_16 = strtol(param, &param, 16);
			param += strlen(" ");    						// Skip space
			data[8 + 2*k] = (uint8_t)(tmp_16 >> 8); 
			data[9 + 2*k] = (uint8_t)(tmp_16 & 0xFF);
		}

		length += data[0];
	}
	else if(strstr(param, "write_mask ") == param) {
		param += strlen("write_mask "); // Skip commands
		
		cmd = MODBUS_MASK_WRITE_REG;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = (uint8_t)(tmp_16 >> 8); 
		data[3] = (uint8_t)(tmp_16 & 0xFF);
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[4] = (uint8_t)(tmp_16 >> 8); 
		data[5] = (uint8_t)(tmp_16 & 0xFF);
		
		length = 6;
	}
	else if(strstr(param, "read_fifo ") == param) {
		param += strlen("read_fifo "); // Skip commands
		
		cmd = MODBUS_READ_FIFO_QUEUE;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_16 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = (uint8_t)(tmp_16 >> 8); 
		data[1] = (uint8_t)(tmp_16 & 0xFF);
		
		length = 2;
	}
	else if(strstr(param, "read_info ") == param) {
		param += strlen("read_info "); // Skip commands
		
		cmd = MODBUS_READ_DEV_INFO;
		addr = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[0] = tmp_8; 
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[1] = tmp_8; 
		tmp_8 = strtol(param, &param, 16);
		param += strlen(" ");    						// Skip space
		data[2] = tmp_8; 
		
		length = 3;
	}
    else {
		snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
		return;
	}
		
	if(length >= MODBUS_LENGTH_DATA_MAX) {
		snprintf(out, bufsize, "%02x", MODBUS_INVALID_CMD_REPLY);
		return;
	}
	
	num_char = snprintf(out, bufsize, "%02x%02x", addr, cmd);
	
	for(i = 0; i < length; i++) {
		num_char += snprintf(out + num_char, bufsize - num_char, "%02x", data[i]);
	}
	
	return;
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
	char buf_addr[5] = { 0 };
	char buf_exc[5] = { 0 };
	
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
		uint8_t exception = moddata[2];
		snprintf(buf_exc, sizeof(buf_exc), "%02d", exception);
		add_value_pair(mqtt_msg, "exception", buf_exc);

		if(exception == ILLEGAL_FUNCTION) {
			add_value_pair(mqtt_msg, "msg", "illegal function");
		}
		else if(exception == ILLEGAL_ADDRESS) {
			add_value_pair(mqtt_msg, "msg", "illegal address");
		}
		else if(exception == ILLEGAL_VALUE) {
			add_value_pair(mqtt_msg, "msg", "illegal value");
		}
		else if(exception == DEVICE_FAILURE) {
			add_value_pair(mqtt_msg, "msg", "device failure");
		}
		else if(exception == ACK) {
			add_value_pair(mqtt_msg, "msg", "ack");
		}
		else if(exception == DEVICE_BUSY) {
			add_value_pair(mqtt_msg, "msg", "device busy");
		}
		else if(exception == NAK) {
			add_value_pair(mqtt_msg, "msg", "nak");
		}
		else if(exception == MEMORY_ERROR) {
			add_value_pair(mqtt_msg, "msg", "memory error");
		}
		else if(exception == GATEWAY_UNAVAILABLE) {
			add_value_pair(mqtt_msg, "msg", "gateway unavailable");
		}
		else if(exception == GATEWAY_FAILED) {
			add_value_pair(mqtt_msg, "msg", "gateway no response");
		}
		
		return true;
	}
	
	uint8_t i = 0;		
	char byte[10];
	for(i = 2; i < moddatalen; i++) {	
		 snprintf(byte, sizeof(byte), "%02x", moddata[i]);
         strcat(buf, byte);
	}
	add_value_pair(mqtt_msg, "data", buf);

	return true;
}