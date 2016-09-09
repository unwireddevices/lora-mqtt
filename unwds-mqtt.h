#ifndef UNWDS_MQTT_H
#define UNWDS_MQTT_H

#include <stdbool.h>
#include <stdint.h>

bool convert_to(uint8_t modid, uint8_t *moddata, int moddatalen, char *topic, char *msg);
bool convert_from(char *type, char *param, char *out);

#endif
