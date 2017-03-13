/*
 * Copyright (C) 2016 Unwired Devices [info@unwds.com]
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    
 * @ingroup     
 * @brief       
 * @{
 * @file		unwds-modules.h
 * @brief       functions to access unwds modules
 * @author      Oleg Artamonov
 */
 
#ifndef UNWDS_MODULES_H
#define UNWDS_MODULES_H

#include <inttypes.h>

#include "unwds-ids.h"
#include "unwds-mqtt.h"

bool umdk_4btn_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_4counter_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_6adc_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_bme280_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_gpio_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_gps_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_lmt01_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_lps331_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_opt3001_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_pir_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_rssiecho_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_sht21_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_uart_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);

void umdk_4counter_command(char *param, char *out, int bufsize);
void umdk_6adc_command(char *param, char *out, int bufsize);
void umdk_bme280_command(char *param, char *out, int bufsize);
void umdk_gpio_command(char *param, char *out, int bufsize);
void umdk_gps_command(char *param, char *out, int bufsize);
void umdk_lmt01_command(char *param, char *out, int bufsize);
void umdk_lps331_command(char *param, char *out, int bufsize);
void umdk_opt3001_command(char *param, char *out, int bufsize);
void umdk_rssiecho_command(char *param, char *out, int bufsize);
void umdk_sht21_command(char *param, char *out, int bufsize);
void umdk_uart_command(char *param, char *out, int bufsize);

static const unwds_module_desc_t unwds_modules_list[] = {
    { .id = UNWDS_GPIO_MODULE_ID,    .name = "gpio",    .cmd = &umdk_gpio_command,       .reply = &umdk_gpio_reply     },
    { .id = UNWDS_4BTN_MODULE_ID,    .name = "4btn",    .cmd = NULL,                     .reply = &umdk_4btn_reply     },
    { .id = UNWDS_GPS_MODULE_ID,     .name = "gps",     .cmd = &umdk_gps_command,        .reply = &umdk_gps_reply      },
    { .id = UNWDS_LSM6DS3_MODULE_ID, .name = "lsm6ds3", .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_LM75_MODULE_ID,    .name = "lm75",    .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_LMT01_MODULE_ID,   .name = "lmt01",   .cmd = &umdk_lmt01_command,      .reply = &umdk_lmt01_reply    },
    { .id = UNWDS_UART_MODULE_ID,    .name = "uart",    .cmd = &umdk_uart_command,       .reply = &umdk_uart_reply     },
    { .id = UNWDS_SHT21_MODULE_ID,   .name = "sht21",   .cmd = &umdk_sht21_command,      .reply = &umdk_sht21_reply    },
    { .id = UNWDS_PIR_MODULE_ID,     .name = "pir",     .cmd = NULL,                     .reply = &umdk_pir_reply      },
    { .id = UNWDS_6ADC_MODULE_ID,    .name = "6adc",    .cmd = &umdk_6adc_command,       .reply = &umdk_6adc_reply     },
    { .id = UNWDS_LPS331_MODULE_ID,  .name = "lps331",  .cmd = &umdk_lps331_command,     .reply = &umdk_lps331_reply   },
    { .id = UNWDS_RSSIECHO_MODULE_ID,.name = "echo",    .cmd = &umdk_rssiecho_command,   .reply = &umdk_rssiecho_reply },
    { .id = UNWDS_PWM_MODULE_ID,     .name = "pwm",     .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_OPT3001_MODULE_ID, .name = "opt3001", .cmd = &umdk_opt3001_command,    .reply = &umdk_opt3001_reply  },
    { .id = UNWDS_DALI_MODULE_ID,    .name = "dali",    .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_BME280_MODULE_ID,  .name = "bme280",  .cmd = &umdk_bme280_command,     .reply = &umdk_bme280_reply   },
};

bool (*umdk_reply_ptr)(uint8_t*, int, mqtt_msg_t*);
void (*umdk_command_ptr)(char*, char*, int);

#endif