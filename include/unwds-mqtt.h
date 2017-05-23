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

#ifndef UNWDS_MQTT_H
#define UNWDS_MQTT_H

#include <stdbool.h>
#include <stdint.h>
#include <mosquitto.h>

#define MQTT_MSG_MAX_NUM 50
#define MQTT_SUBSCRIBE_TO "devices/lora/#"
#define MQTT_PUBLISH_TO "devices/lora/"
#define MQTT_MAX_MSG_SIZE 2048

typedef enum {
    UNWDS_MQTT_REGULAR = 0,
    UNWDS_MQTT_ESCAPED = 1,
} mqtt_format_t;

typedef struct {
    char name[40];
    char value[100];
} mqtt_msg_t;

typedef struct {
    int16_t rssi;
    int16_t battery;
    int16_t temperature;
} mqtt_status_t;

extern bool mqtt_retain;
extern bool mqtt_sepio;
extern int mqtt_qos;

bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *msg);

bool convert_from(char *type, char *param, char *out, int bufsize);

void publish_mqtt_message(struct mosquitto *mosq, const char *addr, const char *topic, char *msg, const mqtt_format_t format);

void build_mqtt_message(char *msg, const mqtt_msg_t *mqtt_msg, const mqtt_status_t status, const char *addr);

void add_value_pair(mqtt_msg_t *msg, char const *name, char const *value);

int unwds_modid_by_name(char *name);

#endif
