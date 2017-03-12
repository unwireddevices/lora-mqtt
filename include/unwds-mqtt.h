#ifndef UNWDS_MQTT_H
#define UNWDS_MQTT_H

#include <stdbool.h>
#include <stdint.h>
#include <mosquitto.h>

#define MQTT_MSG_MAX_NUM 20
#define MQTT_SUBSCRIBE_TO "devices/lora/#"
#define MQTT_PUBLISH_TO "devices/lora/"
#define MQTT_MAX_MSG_SIZE 1024

typedef enum {
	UNWDS_GPIO_GET = 0,
	UNWDS_GPIO_SET_0 = 1,
	UNWDS_GPIO_SET_1 = 2,
	UNWDS_GPIO_TOGGLE = 3,
} unwds_gpio_action_t;

typedef enum {
    UNWDS_MQTT_REGULAR = 0,
    UNWDS_MQTT_ESCAPED = 1,
} mqtt_format_t;

typedef struct {
    char name[40];
    char value[40];
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
void publish_mqtt_message(mosquitto *mosq, const char *addr, const char *topic, char *msg, const mqtt_format_t format);
void build_mqtt_message(char *msg, const mqtt_msg_t *mqtt_msg, const mqtt_status_t status, const char *addr);
void add_value_pair(mqtt_msg_t *msg, char const *name, char const *value);

#endif
