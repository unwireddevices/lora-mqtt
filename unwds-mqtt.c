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
