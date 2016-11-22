#ifndef UNWDS_MQTT_H
#define UNWDS_MQTT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	UNWDS_GPIO_GET = 0,
	UNWDS_GPIO_SET_0 = 1,
	UNWDS_GPIO_SET_1 = 2,
	UNWDS_GPIO_TOGGLE = 3,
} unwds_gpio_action_t;

bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, char *msg);
bool convert_from(char *type, char *param, char *out);

#endif
