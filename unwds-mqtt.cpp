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
#include "unwds-modules.h"

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
        
        char *endptr = NULL;
        strtof(mqtt_msg[i].value, &endptr);
        
        if ( &mqtt_msg[i].value[strlen(mqtt_msg[i].value)] == endptr  ) {
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
    snprintf(buf, sizeof(buf), "\"%s\"", addr);
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

/**
 * Convert received data into MQTT topic and message
 */
bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *mqtt_msg)
{
    switch (modid) {
        case UNWDS_GPIO_MODULE_ID: { /* GPIO */
            if (!umdk_gpio_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_4BTN_MODULE_ID: { /* 4BTN */
			if (!umdk_4btn_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_GPS_MODULE_ID: { /* GPS */
            if (!umdk_gps_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_LMT01_MODULE_ID: /* LMT01 */ {
            if (!umdk_lmt01_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_UART_MODULE_ID: { /* UART */
            if (!umdk_uart_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
		case UNWDS_SHT21_MODULE_ID: /* SHT21 */
		{
            if (!umdk_sht21_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_PIR_MODULE_ID: /* PIR */
		{
            if (!umdk_pir_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_6ADC_MODULE_ID: { /* 6ADC */
            if (!umdk_6adc_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
        }
        case UNWDS_LPS331_MODULE_ID: { /* LPS331 */
            if (!umdk_lps331_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
        }
        case UNWDS_4COUNTER_MODULE_ID: { /* 4counter */
            if (!umdk_4counter_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
        }
		case UNWDS_RSSIECHO_MODULE_ID: { /* RSSI echo */
            if (!umdk_rssiecho_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
		case UNWDS_PWM_MODULE_ID: { /* OPT3001 */
            if (!umdk_opt3001_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}
        case UNWDS_BME280_MODULE_ID: { /* BME280 */
            if (!umdk_opt3001_reply(moddata, moddatalen, topic, mqtt_msg)) {
                return false;
            }
            break;
		}

        default:
            return false;
    }

    return true;
}

/**
 * Convert MQTT message into module data to send to the motes
 */
bool convert_from(char *type, char *param, char *out, int bufsize)
{
/*
 * GPIO
 */ 
    if (strcmp(type, "gpio") == 0) {
        umdk_gpio_command(param, out, bufsize);
    }
/*
 * GPS
 */ 
    else if (strcmp(type, "gps") == 0) {
        umdk_gps_command(param, out, bufsize);
    }
/*
 * LMT01
 */ 
    else if (strcmp(type, "lmt01") == 0) {
        umdk_lmt01_command(param, out, bufsize);
    }
/*
 * 6ADC
 */ 
    else if (strcmp(type, "6adc") == 0) {
        umdk_6adc_command(param, out, bufsize);
    }
/*
 * UART
 */ 
    else if (strcmp(type, "uart") == 0) {
        umdk_uart_command(param, out, bufsize);
    }
/*
 * SHT21
 */ 
    else if (strcmp(type, "sht21") == 0) {
        umdk_sht21_command(param, out, bufsize);
    }
/*
 * LPS331
 */ 
	else if (strcmp(type, "lps331") == 0) {
        umdk_lps331_command(param, out, bufsize);
    }
/*
 * ECHO
 */ 
	else if (strcmp(type, "echo") == 0) {
        umdk_rssiecho_command(param, out, bufsize);
    }
/*
 * OPT3001
 */ 
    else if (strcmp(type, "opt3001") == 0) {
        umdk_opt3001_command(param, out, bufsize);
    }
/*
 * BME280
 */ 
    else if (strcmp(type, "bme280") == 0) {
        umdk_bme280_command(param, out, bufsize);
    }
    else {
        return false;
    }

    return true;
}

