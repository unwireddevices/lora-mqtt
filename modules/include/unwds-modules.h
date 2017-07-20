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
bool umdk_counter_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_pulse_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_adc_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
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
bool umdk_m200_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_config_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_mhz19_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_ibutton_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_pwm_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_switch_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);
bool umdk_m230_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg);

void umdk_counter_command(char *param, char *out, int bufsize);
void umdk_pulse_command(char *param, char *out, int bufsize);
void umdk_adc_command(char *param, char *out, int bufsize);
void umdk_bme280_command(char *param, char *out, int bufsize);
void umdk_gpio_command(char *param, char *out, int bufsize);
void umdk_gps_command(char *param, char *out, int bufsize);
void umdk_lmt01_command(char *param, char *out, int bufsize);
void umdk_lps331_command(char *param, char *out, int bufsize);
void umdk_opt3001_command(char *param, char *out, int bufsize);
void umdk_rssiecho_command(char *param, char *out, int bufsize);
void umdk_sht21_command(char *param, char *out, int bufsize);
void umdk_uart_command(char *param, char *out, int bufsize);
void umdk_m200_command(char *param, char *out, int bufsize);
void umdk_config_command(char *param, char *out, int bufsize);
void umdk_mhz19_command(char *param, char *out, int bufsize);
void umdk_ibutton_command(char *param, char *out, int bufsize);
void umdk_pwm_command(char *param, char *out, int bufsize);
void umdk_switch_command(char *param, char *out, int bufsize);
void umdk_m230_command(char *param, char *out, int bufsize);

static const unwds_module_desc_t unwds_modules_list[] = {
    { .id = UNWDS_GPIO_MODULE_ID,    .name = "gpio",    .cmd = &umdk_gpio_command,       .reply = &umdk_gpio_reply     },
    { .id = UNWDS_4BTN_MODULE_ID,    .name = "4btn",    .cmd = NULL,                     .reply = &umdk_4btn_reply     },
    { .id = UNWDS_COUNTER_MODULE_ID, .name = "counter", .cmd = &umdk_counter_command,    .reply = &umdk_counter_reply  },
    { .id = UNWDS_GPS_MODULE_ID,     .name = "gps",     .cmd = &umdk_gps_command,        .reply = &umdk_gps_reply      },
    { .id = UNWDS_LSM6DS3_MODULE_ID, .name = "lsm6ds3", .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_LM75_MODULE_ID,    .name = "lm75",    .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_LMT01_MODULE_ID,   .name = "lmt01",   .cmd = &umdk_lmt01_command,      .reply = &umdk_lmt01_reply    },
    { .id = UNWDS_UART_MODULE_ID,    .name = "uart",    .cmd = &umdk_uart_command,       .reply = &umdk_uart_reply     },
    { .id = UNWDS_SHT21_MODULE_ID,   .name = "sht21",   .cmd = &umdk_sht21_command,      .reply = &umdk_sht21_reply    },
    { .id = UNWDS_PIR_MODULE_ID,     .name = "pir",     .cmd = NULL,                     .reply = &umdk_pir_reply      },
    { .id = UNWDS_ADC_MODULE_ID,     .name = "adc",     .cmd = &umdk_adc_command,        .reply = &umdk_adc_reply      },
    { .id = UNWDS_LPS331_MODULE_ID,  .name = "lps331",  .cmd = &umdk_lps331_command,     .reply = &umdk_lps331_reply   },
    { .id = UNWDS_RSSIECHO_MODULE_ID,.name = "echo",    .cmd = &umdk_rssiecho_command,   .reply = &umdk_rssiecho_reply },
    { .id = UNWDS_PWM_MODULE_ID,     .name = "pwm",     .cmd = &umdk_pwm_command,        .reply = &umdk_pwm_reply      },
    { .id = UNWDS_OPT3001_MODULE_ID, .name = "opt3001", .cmd = &umdk_opt3001_command,    .reply = &umdk_opt3001_reply  },
    { .id = UNWDS_DALI_MODULE_ID,    .name = "dali",    .cmd = NULL,                     .reply = NULL                 },
    { .id = UNWDS_BME280_MODULE_ID,  .name = "bme280",  .cmd = &umdk_bme280_command,     .reply = &umdk_bme280_reply   },
    { .id = UNWDS_M200_MODULE_ID,    .name = "m200",    .cmd = &umdk_m200_command,       .reply = &umdk_m200_reply     },
    { .id = UNWDS_CONFIG_MODULE_ID,  .name = "config",  .cmd = &umdk_config_command,     .reply = &umdk_config_reply   },
    { .id = UNWDS_PULSE_MODULE_ID,   .name = "pulse",   .cmd = &umdk_pulse_command,      .reply = &umdk_pulse_reply    },
    { .id = UNWDS_MHZ19_MODULE_ID,   .name = "mhz19",   .cmd = &umdk_mhz19_command,      .reply = &umdk_mhz19_reply    },
    { .id = UNWDS_IBUTTON_MODULE_ID, .name = "ibutton", .cmd = &umdk_ibutton_command,    .reply = &umdk_ibutton_reply  },
    { .id = UNWDS_SWITCH_MODULE_ID,  .name = "switch",  .cmd = &umdk_switch_command,     .reply = &umdk_switch_reply   },
    { .id = UNWDS_M230_MODULE_ID, 	 .name = "m230",  	.cmd = &umdk_m230_command,     	 .reply = &umdk_m230_reply   },
};

bool (*umdk_reply_ptr)(uint8_t*, int, mqtt_msg_t*);
void (*umdk_command_ptr)(char*, char*, int);

#endif