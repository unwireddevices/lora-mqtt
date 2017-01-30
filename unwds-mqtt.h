#ifndef UNWDS_MQTT_H
#define UNWDS_MQTT_H

#include <stdbool.h>
#include <stdint.h>

#define MQTT_MSG_MAX_NUM 20

typedef enum {
	UNWDS_GPIO_GET = 0,
	UNWDS_GPIO_SET_0 = 1,
	UNWDS_GPIO_SET_1 = 2,
	UNWDS_GPIO_TOGGLE = 3,
} unwds_gpio_action_t;


typedef struct {
    char name[40];
    char value[40];
} mqtt_msg_t;

bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, mqtt_msg_t *msg);
bool convert_from(char *type, char *param, char *out);

void build_mqtt_message(char *msg, mqtt_msg_t *mqtt_msg);
void add_value_pair(mqtt_msg_t *msg, char const *name, char const *value);

#endif
