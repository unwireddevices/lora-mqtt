#include "unwds-mqtt.h"
#include "mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

/*
	=========
		TODO: split and move this routines into RIOT/unwired-modules/ and build against .c files there to have one undivided code space for modules and drivers for both MQTT and ARM devices
	=========
*/

/**
 * Converts module data into MQTT topic and message
 */
bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, char *msg) {
	switch (modid) {
	case 01:	/* GPIO */
		break;

	case 02:	/* 4BTN */
		strcpy(topic, "4btn");
		uint8_t btn = moddata[0];

		if (moddatalen != 1 || btn < 1 || btn > 4)
			return false;
		
		sprintf(msg, "{ btn: %d }", btn);
		return true;

		break;

	case 06: /* LMT01 */ {
		if (moddatalen < 2)
			return false;

		/*puts("moddata: ");
		int j;
		for (j = 0; j < moddatalen; j++) {
			printf("%02x", moddata[j]);
		}
		puts("");*/

		strcpy(topic, "lmt01");
		
		if (strcmp(moddata, "ok") == 0) {
			strcpy(msg, "ok");
			return true;
		}

		char reply[128];
		strcpy(msg, "{ ");

		int i;
		for (i = 0; i < 8; i += 2) {
			uint32_t sensor = 0;
			if (is_big_endian())
				sensor = (moddata[i + 1] << 8) | moddata[i]; /* We're in big endian there, swap bytes */
			else
				sensor = (moddata[i] << 8) | moddata[i + 1];

			char buf[16] = {};

			if (sensor == 0xFFFF)
				sprintf(buf, "s%d: null", (i / 2) + 1);
			else {
				sprintf(buf, "s%d: %.3f", (i / 2) + 1, (float) (sensor / 16.0) - 100.0);
			}
			
			strcat(msg, buf);

			if (i != 6)
				strcat(msg, ", ");
		}

		strcat(msg, " }");

		break;
	}

	case 07: {/* UART */
		uint8_t reply_type = moddata[0];

		strcat(topic, "uart");

		switch (reply_type) {
			case 0: /* UMDK_UART_REPLY_SENT */
				strcat(msg, "{ type: 0, msg: \"sent ok\" }");
				return true;			

			case 1: { /* UMDK_UART_REPLY_RECEIVED */
				char hexbuf[255] = {};
				bytes_to_hex(moddata + 1, moddatalen - 1, hexbuf, false);

				sprintf(msg, "{ type: 1, msg: \"%s\" }", hexbuf);
				return true;
			}

			case 2:
				strcat(msg, "{ type: 2, msg: \"baud rate set\" }");
				return true;

			case 253: /* UMDK_UART_REPLY_ERR_OVF */
				strcat(msg, "{ type: 253, msg: \"rx buffer overrun\" }");
				return true;

			case 254: /* UMDK_UART_REPLY_ERR_FMT */
				strcat(msg, "{ type: 254, msg: \"invalid format\" }");
				return true;

			case 255: /* UMDK_UART_REPLY_ERR */
				strcat(msg, "{ type: 255, msg: \"UART interface error\" }");
				return true;
		}

		return false;
	}

	case 9:	/* PIR */
		strcpy(topic, "pir");
		uint8_t pir = moddata[0];

		if (moddatalen != 1 || pir < 1 || pir > 4)
			return false;
		
		sprintf(msg, "{ pir: rising, num: %d }", pir);
		return true;

		break;

	case 10: {	/* 6ADC */
		strcpy(topic, "6adc");

		printf("[6adc] moddatalen: %d\n", moddatalen);
		printf("[6adc] moddata: ");

		int j;
		for (j = 0; j < moddatalen; j++) {
			printf("%02x", moddata[j]);
		}
		puts("");
		
		if (strcmp(moddata, "ok") == 0) {
			strcpy(msg, "ok");
			return true;
		}

		if (strcmp(moddata, "fail") == 0) {
			strcpy(msg, "fail");
			return true;
		}

		char reply[128];
		strcpy(msg, "{ ");
	
		int i;
		for (i = 0; i < 12; i += 2) {
			uint16_t sensor = 0;
			if (is_big_endian())
				sensor = (moddata[i + 1] << 8) | moddata[i]; /* We're in big endian there, swap bytes */
			else
				sensor = (moddata[i] << 8) | moddata[i + 1];

			char buf[16] = {};

			if (sensor == 0xFFFF)
				sprintf(buf, "adc%d: null", (i / 2) + 1);
			else {
				sprintf(buf, "adc%d: %d", (i / 2) + 1, sensor);
			}
			
			strcat(msg, buf);

			if (i != 10)
				strcat(msg, ", ");
		}

		strcat(msg, " }");		
		break;
  }

  case 11: /* LPS331 */ {
    if (moddatalen < 4)
      return false;
  
    strcpy(topic, "lps331");
  
    if (strcmp(moddata, "ok") == 0) {
      strcpy(msg, "ok");
      return true;
    }   
  
    strcpy(msg, "{ ");
  
    /* Extract temperature */
    int16_t temperature = 0;
  
    if (is_big_endian()) {
      temperature = moddata[0];
      temperature += (moddata[1] << 8); 
    }   
    else {
      temperature = moddata[1];
      temperature += (moddata[0] << 8); 
    }   
  
    /* Extract pressure */
    uint16_t pressure = 0;
  
    if (is_big_endian()) {
      pressure = moddata[2];
      pressure += (moddata[3] << 8); 
    }   
    else {
      pressure = moddata[3];
      pressure += (moddata[2] << 8); 
    }
  
    char buf[40] = {}; 
    snprintf(buf, 40, "temperature: %.1f, pressure: %d", temperature / 1000.0, pressure);
  
    strcat(msg, buf);
    strcat(msg, " }");
  
    break;
  }
  default:
                        return false;
  }

  return true;
}

/**
 * Converts from MQTT topic and message into module data to send to the nodes
 */
bool convert_from(char *type, char *param, char *out) {
	if (strcmp(type, "gpio") == 0) {
		if (strstr(param, "set ") == param) {
			param += 4; // skip command

			uint8_t pin = strtol(param, &param, 10);
			uint8_t value = strtol(param, NULL, 10);

			uint8_t gpio_cmd = 0;
			if (value == 1)
				gpio_cmd = 2 << 6;	// 10 in upper two bits of cmd byte is SET TO ONE command
			else if (value == 0)
				gpio_cmd = 1 << 7;	// 01 in upper two bits of cmd byte is SET TO ZERO command

			// Append pin number bits and mask exceeding bits just in case
			gpio_cmd |= pin & 0x3F;

			printf("[mqtt-gpio] Set command | Pin: %d, value: %d, cmd: 0x%02x\n", pin, value, gpio_cmd);

			sprintf(out, "01%02x", gpio_cmd);
		} else if (strstr(param, "get ") == param) {
		} else if (strstr(param, "toggle ") == param) {
		}
	} else if (strcmp(type, "lmt01") == 0) {
		if (strstr(param, "set_period ") == param) {
			param += 11;	// Skip command

			uint8_t period = atoi(param);
			sprintf(out, "0600%02x", period);
		} else if (strstr(param, "get") == param) {
			sprintf(out, "0601");
		} else if (strstr(param, "set_gpios ") == param) {/*
			param += 10;	// Skip command			
	
			uint8_t gpio = 0;
			while ((gpio = strtol(param, param, 10))

			sprintf(out, "0602");*/
		}
	} else if (strcmp(type, "6adc") == 0) {
		if (strstr(param, "set_period ") == param) {
			param += 11;	// Skip command

			uint8_t period = atoi(param);
			sprintf(out, "0a00%02x", period);
		} else if (strstr(param, "get") == param) {
			sprintf(out, "0a01");
		} else if (strstr(param, "set_gpio ") == param) {
			param += 9;	// Skip command			
	
			uint8_t gpio = atoi(param);

			sprintf(out, "0a02%02x", gpio);
		} else if (strstr(param, "set_lines ") == param) {
			param += 10;	// Skip command			
	
			uint8_t lines_en = 0;
			uint8_t line = 0;
			while (line = (uint8_t) strtol(param, &param, 10)) {
				if (line > 0 && line <= 6)
					lines_en |= 1 << (line - 1);
			}

			sprintf(out, "0a03%02x", lines_en);
		}
	} else if (strcmp(type, "uart") == 0) {
		if (strstr(param, "send ") == param) {
			uint8_t bytes[200] = {};
			char *hex = param + 5; // Skip command
			
			if (!hex_to_bytes(hex, bytes, true)) {
				return false;
			}

			sprintf(out, "0700%s", hex);
		} else if (strstr(param, "set baudrate ") == param) {
			param += 13; // Skip commands

			uint8_t baudrate = atoi(param);
			printf("baudrate: %d\n", baudrate);
			if (baudrate > 10) {
				return false;
			}

			sprintf(out, "0701%02x", baudrate);
		}
	} else
		return false;

	return true;
}
