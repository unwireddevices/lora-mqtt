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
 * @file		unwds-ids.h
 * @brief       List of UMDK modules unique IDs
 * @author      Oleg Artamonov
 */
#ifndef UNWDS_IDS_H
#define UNWDS_IDS_H

#include "unwds-modules.h"

typedef enum {
    UNWDS_GPIO_MODULE_ID = 1,
    UNWDS_4BTN_MODULE_ID,
    UNWDS_GPS_MODULE_ID,
    UNWDS_LSM6DS3_MODULE_ID,
    UNWDS_LM75_MODULE_ID,
    UNWDS_LMT01_MODULE_ID,
    UNWDS_UART_MODULE_ID,
    UNWDS_SHT21_MODULE_ID,
    UNWDS_PIR_MODULE_ID,
    UNWDS_6ADC_MODULE_ID,
    UNWDS_LPS331_MODULE_ID,
    UNWDS_4COUNTER_MODULE_ID,
    UNWDS_RSSIECHO_MODULE_ID,
    UNWDS_PWM_MODULE_ID,
    UNWDS_OPT3001_MODULE_ID,
    UNWDS_DALI_MODULE_ID,
    UNWDS_BME280_MODULE_ID,
} unwds_module_ids_t;

typedef struct {
    uint8_t id;
    char    name[20];
    void*   cmd;
    void*   reply;
} unwds_module_desc_t;

#endif