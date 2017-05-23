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

void publish_mqtt_message(struct mosquitto *mosq, const char *addr, const char *topic, char *msg, const mqtt_format_t format) {
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
        
        int len = strlen(mqtt_msg[i].value);
        
        /* numbers in JSON do not need to be escaped in quotes */
        if ( &mqtt_msg[i].value[len] == endptr  ) {
            needs_quotes = 0;
        } else {
            needs_quotes = 1;
        }
        
        /* sublasses do not need to be escaped */
        if ((mqtt_msg[i].value[0] == '{') && (mqtt_msg[i].value[len - 1] == '}')) {
            needs_quotes = 0;
        }
        
        /* arrays do not need to be escaped */
        if ((mqtt_msg[i].value[0] == '[') && (mqtt_msg[i].value[len - 1] == ']')) {
            needs_quotes = 0;
        }
        
        /* elements INSIDE sublass or array MUST be escaped manually if needed */
        
        /* leading zeros are not allowed for regular numbers in JSON */
        if ((mqtt_msg[i].value[0] == '0') && (mqtt_msg[i].value[1] != '.') && (mqtt_msg[i].value[1] != 0)) {
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
    int num_modules = sizeof(unwds_modules_list)/sizeof(unwds_module_desc_t);
    
    int i = 0;
    for (i = 0; i<num_modules; i++) {
        if (unwds_modules_list[i].id == modid) {
            if (unwds_modules_list[i].reply) {
                umdk_reply_ptr = unwds_modules_list[i].reply;
                strcpy(topic, unwds_modules_list[i].name);
                return umdk_reply_ptr(moddata, moddatalen, mqtt_msg);
            }
        }
    }
    return false;
}

/**
 * Convert MQTT message into module data to send to the motes
 */
bool convert_from(char *type, char *param, char *out, int bufsize)
{
    int num_modules = sizeof(unwds_modules_list)/sizeof(unwds_module_desc_t);
    
    int i = 0;
    for (i = 0; i<num_modules; i++) {
        if (strcmp(type, unwds_modules_list[i].name) == 0) {
            if (unwds_modules_list[i].cmd) {
                umdk_command_ptr = unwds_modules_list[i].cmd;
                /* first byte - two characters with ASCII HEX - is a module ID */
                snprintf(out, 3, "%02x", unwds_modules_list[i].id);
                /* the rest is data */
                umdk_command_ptr(param, out + 2, bufsize);
                return true;
            }
        }
    }
    return false;
}

int unwds_modid_by_name(char *name) {
    int i = 0;
    for (i = 0; i < sizeof(unwds_modules_list)/sizeof(unwds_module_desc_t); i++) {
        if (strcmp(name, unwds_modules_list[i].name) == 0) {
            return unwds_modules_list[i].id;
        }
    }
    
    return -1;
}