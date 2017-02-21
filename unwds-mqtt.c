#define _BSD_SOURCE

#include "unwds-mqtt.h"
#include "mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "utils.h"

bool mqtt_retain = false;
bool mqtt_sepio = false;
int mqtt_qos = 1;

static int mqtt_mid = 0;
 
void add_value_pair(mqtt_msg_t *mqtt_msg, const char *name, const char *value)
{
    uint8_t i = 0;
    
    for (i = 0; i < MQTT_MSG_MAX_NUM; i++) {
        if (mqtt_msg[i].name[0] == 0) {
            strcat(mqtt_msg[i].name, name);
            strcat(mqtt_msg[i].value, value);
            return;
        }
    }
}

static void mqtt_escape_quotes(char *msg) {
    char *buf = (char *)malloc(MQTT_MAX_MSG_SIZE);
    memset(buf, 0, MQTT_MAX_MSG_SIZE);
    
    char *ptr;
    ptr = strtok(msg, "\"");
    
    do {
        strcat(buf, ptr);
        strcat(buf, "\\\"");
        ptr = strtok(NULL, "\"");
    } while (ptr);
    
    strcpy(msg, buf);
    free(buf);
}

void publish_mqtt_message(mosquitto *mosq, const char *addr, const char *topic, char *msg, const mqtt_format_t format) {
    if (!mosq) {
        return;
    }
       
	// Append an MQTT topic path to the topic from the reply
	char *mqtt_topic = (char *)malloc(strlen(MQTT_PUBLISH_TO) + strlen(addr) + strlen(topic) + strlen("/miso") + 1);

	strcpy(mqtt_topic, MQTT_PUBLISH_TO);
	strcat(mqtt_topic, addr);
	strcat(mqtt_topic, "/");
    if (mqtt_sepio) {
        strcat(mqtt_topic, "miso/");
    }
    strcat(mqtt_topic, topic);
    
    if (format == UNWDS_MQTT_ESCAPED) {
        mqtt_escape_quotes(msg);
    }
    
    char *logbuf = (char *) malloc(MQTT_MAX_MSG_SIZE + 50);
	snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Publishing to the topic %s the message \"%s\"\n", mqtt_topic, msg);
	logprint(logbuf);

	int res = mosquitto_publish(mosq, &mqtt_mid, mqtt_topic, strlen(msg), msg, mqtt_qos, mqtt_retain);
    
    switch (res) {
        case MOSQ_ERR_SUCCESS:
            snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Message published successfully\n");
            break;
        case MOSQ_ERR_INVAL:
            snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Error: invalid input\n");
            break;
        case MOSQ_ERR_NOMEM:
            snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Error: out of memory\n");
            break;
        case MOSQ_ERR_NO_CONN:
            snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Error: not connected\n");
            break;
        case MOSQ_ERR_PROTOCOL:
            snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Error: protocol error\n");
            break;
        case MOSQ_ERR_PAYLOAD_SIZE:
            snprintf(logbuf, MQTT_MAX_MSG_SIZE + 50, "[mqtt] Error: payload too large\n");
            break;
    }
    logprint(logbuf);
    free(logbuf);
	free(mqtt_topic);
}

void build_mqtt_message(char *msg, const mqtt_msg_t *mqtt_msg, const mqtt_status_t status, const char *addr) {   
    bool needs_quotes = 0;
    
    strcpy(msg, "{ \"data\": { ");
    
    uint8_t i = 0;
    for (i = 0; i < MQTT_MSG_MAX_NUM; i++) {
        if (mqtt_msg[i].name[0] == 0) {
            strcat(msg, " }");
            break;
        }
        
        if (i != 0) {
            strcat(msg, ", ");
        }
        
        strcat(msg, "\"");
        strcat(msg, mqtt_msg[i].name);
        strcat(msg, "\": ");
        
        float f;
        
        if ( sscanf(mqtt_msg[i].value, "%f", &f) || \
             !strcmp(mqtt_msg[i].value, "true") || !strcmp(mqtt_msg[i].value, "false") || \
             !strcmp(mqtt_msg[i].value, "null")) {
            needs_quotes = 0;
        } else {
            needs_quotes = 1;
        }
        
        if (needs_quotes) {
            strcat(msg, "\"");
        }
        strcat(msg, mqtt_msg[i].value);
        if (needs_quotes) {
            strcat(msg, "\"");
        }
    }
    
    char buf[50];
    strcat(msg, ", \"status\": { \"devEUI\" : ");
    snprintf(buf, sizeof(buf), "%s", addr);
    strcat(msg, buf);
    
    strcat(msg, ", \"rssi\": ");
    snprintf(buf, sizeof(buf), "%d", status.rssi);
    strcat(msg, buf);
    
    strcat(msg, ", \"temperature\": ");
    snprintf(buf, sizeof(buf), "%d", status.temperature);
    strcat(msg, buf);
    
    strcat(msg, ", \"battery\": ");
    snprintf(buf, sizeof(buf), "%d", status.battery);
    strcat(msg, buf);
    
    char time[64];
    struct timeval tv;
    struct tm *tm;
    gettimeofday(&tv, NULL);
    tm = gmtime(&tv.tv_sec);
    
    strcat(msg, ", \"date\": ");
    strftime(time, sizeof(time), "\"%FT%T.%%uZ\"", tm);
    snprintf(buf, sizeof(buf), time, tv.tv_usec);
    strcat(msg, buf);

    strcat(msg, " }}");
}

/*
    =========
        TODO: split and move those routines into RIOT/unwired-modules/ and build against .c files there to have one undivided code space for modules and drivers for both MQTT and ARM devices
    =========
 */


/**
 * Converts module data into MQTT topic and message
 */
bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    
    switch (modid) {
        case 1: /* GPIO */
		{
			uint8_t reply_type = moddata[0];
            strcpy(topic, "gpio");
			
            switch (reply_type) {
				case 0: { /* UNWD_GPIO_REPLY_OK_0 */
                    add_value_pair(mqtt_msg, "type", "0");
                    add_value_pair(mqtt_msg, "msg", "0");
					return true;
				}
				case 1: { /* UNWD_GPIO_REPLY_OK_1 */
                    add_value_pair(mqtt_msg, "type", "1");
                    add_value_pair(mqtt_msg, "msg", "1");
					return true;
				}
				case 2: { /* UNWD_GPIO_REPLY_OK */
                    add_value_pair(mqtt_msg, "type", "0");
                    add_value_pair(mqtt_msg, "msg", "set ok");
					return true;
				}
				case 3: { /* UNWD_GPIO_REPLY_ERR_PIN */
                    add_value_pair(mqtt_msg, "type", "3");
                    add_value_pair(mqtt_msg, "msg", "invalid pin");
					return true;
				}
				case 4: { /* UNWD_GPIO_REPLY_ERR_FORMAT */
                    add_value_pair(mqtt_msg, "type", "4");
                    add_value_pair(mqtt_msg, "msg", "invalid format");
					return true;
				}
			}
            break;
		}
        case 2: /* 4BTN */
		{
			strcpy(topic, "4btn");
            uint8_t btn = moddata[0];
            uint8_t dir = moddata[1];

            if (moddatalen != 2) {
                return false;
            }

            snprintf(buf, sizeof(buf), "%d", btn);
            add_value_pair(mqtt_msg, "btn", buf);
            if (dir) {
                add_value_pair(mqtt_msg, "state", "released");
            } else {
                add_value_pair(mqtt_msg, "state", "pressed");
            }
            return true;

            break;
		}
        case 3: /* GPS */
		{
            strcpy(topic, "gps");

            if (moddatalen < 1) {
                return false;
            }

            uint8_t reply = moddata[0] & 3; /* Last 4 bits is reply type */
            switch (reply) {
                case 0: { /* GPS data */
                    if (moddatalen != 1 + 6) { /* There must be 6 bytes of GPS data + 1 byte of reply type */
                        return false;
                    }

                    uint8_t *bytes = (uint8_t *) (moddata + 1);

                    float lat, lon;

                    /* This code is endian-safe */
                    lat = (bytes[0] + (bytes[1] << 8) + (bytes[2] << 16)) / 1000.0f;
                    lon = (bytes[3] + (bytes[4] << 8) + (bytes[5] << 16)) / 1000.0f;

                    /* Apply sign bits from reply */
                    if ((moddata[0] >> 5) & 1) {
                        lat = -lat;
                    }

                    if ((moddata[0] >> 6) & 1) {
                        lon = -lon;
                    }

                    add_value_pair(mqtt_msg, "valid", "true");
                    snprintf(buf, sizeof(buf), "%03.3f", lat);
                    add_value_pair(mqtt_msg, "lat", buf);
                    snprintf(buf, sizeof(buf), "%04.3f", lon);
                    add_value_pair(mqtt_msg, "lon", buf);
                    break;
				}
                case 1: { /* No data yet */
                    add_value_pair(mqtt_msg, "valid", "false");
                    add_value_pair(mqtt_msg, "lat", "null");
                    add_value_pair(mqtt_msg, "lon", "null");
                    break;
				}
                case 3: { /* Error occured */
                    add_value_pair(mqtt_msg, "valid", "false");
                    add_value_pair(mqtt_msg, "msg", "error");
                    break;
				}
                default:
                    return false;
            }

            break;
		}

        case 6: /* LMT01 */ {
			if (moddatalen < 2) {
                return false;
            }

            /*puts("moddata: ");
               int j;
               for (j = 0; j < moddatalen; j++) {
                printf("%02x", moddata[j]);
               }
               puts("");*/

            strcpy(topic, "lmt01");

            if (strcmp((const char *)moddata, "ok") == 0) {
                add_value_pair(mqtt_msg, "msg", "ok");
                return true;
            }

            int i;
            for (i = 0; i < 8; i += 2) {
                uint32_t sensor = 0;
                if (is_big_endian()) {
                    sensor = (moddata[i + 1] << 8) | moddata[i]; /* We're in big endian there, swap bytes */
                }
                else {
                    sensor = (moddata[i] << 8) | moddata[i + 1];
                }

                char ch[3] = {};
                snprintf(ch, sizeof(ch), "s%d", (i / 2) + 1);

                if (sensor == 0xFFFF) {
                    add_value_pair(mqtt_msg, ch, "null");
                }
                else {
                    snprintf(buf, sizeof(buf), "%.3f", (float) (sensor / 16.0) - 100.0);
                    add_value_pair(mqtt_msg, ch, buf);
                }
            }
            break;
		}
        case 7: { /* UART */
            uint8_t reply_type = moddata[0];

            strcpy(topic, "uart");

            switch (reply_type) {
                case 0: /* UMDK_UART_REPLY_SENT */
                    add_value_pair(mqtt_msg, "type", "0");
                    add_value_pair(mqtt_msg, "msg", "sent ok");
                    return true;

                case 1: { /* UMDK_UART_REPLY_RECEIVED */
                    char hexbuf[255] = { 0 };
                    char hex[3] = { 0 };
                    int k;
                    for (k = 0; k < moddatalen - 1; k++) {
                        snprintf(hex, 3, "%02x", moddata[k+1]);
                        strcat(hexbuf, hex);
                    }
                    add_value_pair(mqtt_msg, "type", "1");
                    add_value_pair(mqtt_msg, "msg", hexbuf);
                    return true;
                }

                case 2:
                    add_value_pair(mqtt_msg, "type", "2");
                    add_value_pair(mqtt_msg, "msg", "baud rate set");
                    return true;

                case 253: /* UMDK_UART_REPLY_ERR_OVF */
                    add_value_pair(mqtt_msg, "type", "253");
                    add_value_pair(mqtt_msg, "msg", "rx buffer overrun");
                    return true;

                case 254: /* UMDK_UART_REPLY_ERR_FMT */
                    add_value_pair(mqtt_msg, "type", "254");
                    add_value_pair(mqtt_msg, "msg", "invalid format");
                    return true;

                case 255: /* UMDK_UART_REPLY_ERR */
                    add_value_pair(mqtt_msg, "type", "255");
                    add_value_pair(mqtt_msg, "msg", "UART interface error");
                    return true;
            }

            return false;
		}
		case 8: /* SHT21 */
		{
            if (moddatalen < 2) {
                return false;
            }

            /*puts("moddata: ");
               int j;
               for (j = 0; j < moddatalen; j++) {
                printf("%02x", moddata[j]);
               }
               puts("");*/

            strcpy(topic, "sht21");

            if (strcmp((const char*)moddata, "ok") == 0) {
                add_value_pair(mqtt_msg, "msg", "ok");
                return true;
            }

			uint16_t temp = 0;
			if (is_big_endian()) {
				temp = (moddata[1] << 8) | moddata[0]; /* We're in big endian there, swap bytes */
			}
			else {
				temp = (moddata[0] << 8) | moddata[1];
			}

			uint8_t humid = moddata[2];
            snprintf(buf, sizeof(buf), "%.02f", (float) (temp / 16.0 - 100));
            add_value_pair(mqtt_msg, "temp", buf);
            
            snprintf(buf, sizeof(buf), "%d", humid);
            add_value_pair(mqtt_msg, "humid", buf);
            
			return true;
		}
        case 9: /* PIR */
		{
            strcpy(topic, "pir");
            uint8_t pir = moddata[0];

            if (moddatalen != 1) {
                return false;
            }

            snprintf(buf, sizeof(buf), "%d", pir);
            add_value_pair(mqtt_msg, "pir", buf);
            return true;

            break;
		}
        case 10: { /* 6ADC */
            strcpy(topic, "6adc");

			if (moddatalen == 1) {
				uint8_t fail = moddata[0];

				if (fail) {
                    add_value_pair(mqtt_msg, "msg", "fail");
				} else {
                    add_value_pair(mqtt_msg, "msg", "ok");
				}

				return true;
			}

            int i;
            for (i = 0; i < 16; i += 2) {
                uint16_t sensor = 0;
                if (is_big_endian()) {
                    sensor = (moddata[i + 1] << 8) | moddata[i]; /* We're in big endian there, swap bytes */
                }
                else {
                    sensor = (moddata[i] << 8) | moddata[i + 1];
                }

                char ch[6] = {};
                snprintf(ch, sizeof(ch), "adc%d", (i / 2) + 1);

                if (sensor == 0xFFFF) {
                    add_value_pair(mqtt_msg, ch, "null");
                }
                else {
                    snprintf(buf, sizeof(buf), "%d", sensor);
                    add_value_pair(mqtt_msg, ch, buf);
                }
            }
            break;
        }
        case 11: /* LPS331 */
		{
            if (moddatalen < 2) {
                return false;
            }

            strcpy(topic, "lps331");

            if (strcmp((const char*)moddata, "ok") == 0) {
                add_value_pair(mqtt_msg, "msg", "ok");
                return true;
            }

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

            snprintf(buf, sizeof(buf), "%.1f", ((float)temperature / 16.0) - 100.0);
            add_value_pair(mqtt_msg, "temperature", buf);
            snprintf(buf, sizeof(buf), "%d", pressure);
            add_value_pair(mqtt_msg, "pressure", buf);
            
            break;
        }
        
        case 12: { /* 4counter */
            strcpy(topic, "4counter");

            if (strcmp((const char*)moddata, "ok") == 0) {
                add_value_pair(mqtt_msg, "msg", "ok");
                return true;
            }

            /* Extract counter values */
            uint8_t i = 0;
            uint32_t values[4] = { 0 };
            char ch[5] = {};
            
            /* data encoded in 3 UINT32 numbers on Little Endian system */
            /* we need to swap bytes inside UINT32s if we are on BE system (e.g. MIPS CPU) */
            /* then we can just use functions reverse to those used to encode */
            uint32_t *num = (uint32_t *)&moddata[0];
            if (is_big_endian()) {
                for (i = 0; i < 3; i++ ) {
                    *num = ((*num >> 24) & 0xff) | ((*num << 8) & 0xff0000) | ((*num >> 8) & 0xff00) | ((*num << 24) & 0xff000000);
                    num++;
                }
            }
            
            /* let's unpack 12 bytes back into 4 values */
            num = (uint32_t *)&moddata[0];
            values[0] = num[0] >> 8;
            values[1] = ((num[0] & 0xFF) << 16) | (num[1] >> 16);
            values[2] = ((num[1] & 0xFFFF) << 8) | (num[2] >> 24);
            values[3] = num[2] & 0xFFFFFF;
            
            for (i = 0; i < 4; i++) {           
                snprintf(ch, sizeof(ch), "v%d", i);
                snprintf(buf, sizeof(buf), "%u", values[i]);
                add_value_pair(mqtt_msg, ch, buf);
            }
            break;
        }

		case 13: { /* RSSI echo */
            if (moddatalen < 2) {
                return false;
            }

            strcpy(topic, "echo");

            /* Extract RSSI value */
            int16_t rssi = 0;

            if (is_big_endian()) {
                rssi = moddata[0];
				rssi += (moddata[1] << 8);
            }
            else {
                rssi = moddata[1];
				rssi += (moddata[0] << 8);
            }

			snprintf(buf, sizeof(buf), "%d", rssi);
            add_value_pair(mqtt_msg, "rssi", buf);

			break;
		}
		case 15: /* OPT3001 */
		{
            if (moddatalen < 2) {
                return false;
            }

            strcpy(topic, "opt3001");

            if (strcmp((const char*)moddata, "ok") == 0) {
                add_value_pair(mqtt_msg, "msg", "ok");
                return true;
            }

			uint16_t lum = 0;
			if (is_big_endian()) {
				lum = (moddata[1] << 8) | moddata[0]; /* We're in big endian there, swap bytes */
			}
			else {
				lum = (moddata[0] << 8) | moddata[1];
			}

            snprintf(buf, sizeof(buf), "%d", lum);
            add_value_pair(mqtt_msg, "luminocity", buf);
			
			return true;
		}

        default:
            return false;
    }

    return true;
}

/**
 * Converts from MQTT topic and message into module data to send to the nodes
 */
bool convert_from(char *type, char *param, char *out, int bufsize)
{
    if (strcmp(type, "gpio") == 0) {
        if (strstr(param, "set ") == param) {
            param += 4; // skip command

            uint8_t pin = strtol(param, &param, 10);
            uint8_t value = strtol(param, NULL, 10);

            uint8_t gpio_cmd = 0;
            if (value == 1) {
                gpio_cmd = UNWDS_GPIO_SET_1 << 6;  // 10 in upper two bits of cmd byte is SET TO ONE command
            }
            else if (value == 0) {
                gpio_cmd = UNWDS_GPIO_SET_0 << 6;  // 01 in upper two bits of cmd byte is SET TO ZERO command

            }
            // Append pin number bits and mask exceeding bits just in case
            gpio_cmd |= pin & 0x3F;

            printf("[mqtt-gpio] Set command | Pin: %d, value: %d, cmd: 0x%02x\n", pin, value, gpio_cmd);

            snprintf(out, bufsize, "01%02x", gpio_cmd);
        }
        else if (strstr(param, "get ") == param) {
			param += 4; // skip command

            uint8_t pin = strtol(param, &param, 10);

            uint8_t gpio_cmd = UNWDS_GPIO_GET << 6;
            // Append pin number bits and mask exceeding bits just in case
            gpio_cmd |= pin & 0x3F;

            printf("[mqtt-gpio] Get command | Pin: %d, cmd: 0x%02x\n", pin, gpio_cmd);

            snprintf(out, bufsize, "01%02x", gpio_cmd);
        }
        else if (strstr(param, "toggle ") == param) {
			param += 7; // skip command

            uint8_t pin = strtol(param, &param, 10);

            uint8_t gpio_cmd = UNWDS_GPIO_TOGGLE << 6;
            // Append pin number bits and mask exceeding bits just in case
            gpio_cmd |= pin & 0x3F;

            printf("[mqtt-gpio] Toggle command | Pin: %d, cmd: 0x%02x\n", pin, gpio_cmd);

            snprintf(out, bufsize, "01%02x", gpio_cmd);
        }
    }
    else if (strcmp(type, "gps") == 0) {
        if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0300");
        }
    }
    else if (strcmp(type, "lmt01") == 0) {
        if (strstr(param, "set_period ") == param) {
            param += 11;    // Skip command

            uint8_t period = atoi(param);
            snprintf(out, bufsize, "0600%02x", period);
        }
        else if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0601");
        }
        else if (strstr(param, "set_gpios ") == param) {
		/*	 param += 10;	// Skip command

		     uint8_t gpio = 0;
		     while ((gpio = strtol(param, param, 10))

		     snprintf(out, bufsize, "0602");*/
        }
    }
    else if (strcmp(type, "6adc") == 0) {
        if (strstr(param, "set_period ") == param) {
            param += 11;    // Skip command

            uint8_t period = atoi(param);
            snprintf(out, bufsize, "0a00%02x", period);
        }
        else if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0a01");
        }
        else if (strstr(param, "set_gpio ") == param) {
            param += 9; // Skip command

            uint8_t gpio = atoi(param);

            snprintf(out, bufsize, "0a02%02x", gpio);
        }
        else if (strstr(param, "set_lines ") == param) {
            param += 10;    // Skip command

            uint8_t lines_en = 0;
            uint8_t line = 0;
            while ( (line = (uint8_t)strtol(param, &param, 10)) ) {
                if (line > 0 && line <= 7) {
                    lines_en |= 1 << (line - 1);
                }
            }

            snprintf(out, bufsize, "0a03%02x", lines_en);
        }
    }
    else if (strcmp(type, "uart") == 0) {
        if (strstr(param, "send ") == param) {
            uint8_t bytes[200] = {};
            char *hex = param + 5; // Skip command

            if (!hex_to_bytes(hex, bytes, true)) {
                return false;
            }

            snprintf(out, bufsize, "0700%s", hex);
        }
        else if (strstr(param, "set_baudrate ") == param) {
            param += 13; // Skip commands

            uint8_t baudrate = atoi(param);
            printf("baudrate: %d\n", baudrate);
            if (baudrate > 10) {
                return false;
            }

            snprintf(out, bufsize, "0701%02x", baudrate);
        }
        else if (strstr(param, "set ") == param) {
            param += 4; // Skip commands

            if (strlen(param) > strlen("115200-8N1")) {
                return false;
            }

            snprintf(out, bufsize, "0702");
            
            /* convert string to hex */
            uint8_t k;
            for (k = 0; k < strlen(param); k++) {
                snprintf(out + 4 + 2*k, 3, "%02x", param[k]);
            }
            
            printf("UART mode: %s, Command: %s\n", param, out);
        }
        else {
            return false;
        }
    }
    else if (strcmp(type, "sht21") == 0) {
        if (strstr(param, "set_period ") == param) {
            param += 11;    // Skip command

            uint8_t period = atoi(param);
            snprintf(out, bufsize, "0800%02x", period);
        }
        else if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0801");
        }
        else if (strstr(param, "set_i2c ") == param) { 
             param += 8;	// Skip command

             uint8_t i2c = atoi(param);

             snprintf(out, bufsize, "0802%02x", i2c);
        }
    }
	else if (strcmp(type, "lps331") == 0) {
        if (strstr(param, "set_period ") == param) {
            param += 11;    // Skip command

            uint8_t period = atoi(param);
            snprintf(out, bufsize, "0b00%02x", period);
        }
        else if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0b01");
        }
        else if (strstr(param, "set_i2c ") == param) { 
             param += 8;	// Skip command

             uint8_t i2c = atoi(param);

             snprintf(out, bufsize, "0b02%02x", i2c);
        }
    }
	else if (strcmp(type, "echo") == 0) {
        if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0d00");
        }
    }
    else if (strcmp(type, "opt3001") == 0) {
        if (strstr(param, "set_period ") == param) {
            param += 11;    // Skip command

            uint8_t period = atoi(param);
            snprintf(out, bufsize, "0f00%02x", period);
        }
        else if (strstr(param, "get") == param) {
            snprintf(out, bufsize, "0f01");
        }
        else if (strstr(param, "set_i2c ") == param) { 
             param += 8;	// Skip command

             uint8_t i2c = atoi(param);

             snprintf(out, bufsize, "0f02%02x", i2c);
        }
    }
    else {
        return false;
    }

    return true;
}

