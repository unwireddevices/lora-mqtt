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
 * @file		unwds-ids.h
 * @brief       List of UMDK modules unique IDs
 * @author      Oleg Artamonov
 */
#ifndef UNWDS_IDS_H
#define UNWDS_IDS_H

#include "unwds-modules.h"

typedef enum {
    UNWDS_GPIO_MODULE_ID = 1,
    UNWDS_4BTN_MODULE_ID = 2, 
    UNWDS_GPS_MODULE_ID = 3,
    UNWDS_LSM6DS3_MODULE_ID = 4,
    UNWDS_LM75_MODULE_ID = 5,
    UNWDS_LMT01_MODULE_ID = 6,
    UNWDS_UART_MODULE_ID = 7,
    UNWDS_SHT21_MODULE_ID = 8,
    UNWDS_PIR_MODULE_ID = 9,
    UNWDS_ADC_MODULE_ID = 10,
    UNWDS_LPS331_MODULE_ID = 11,
    UNWDS_COUNTER_MODULE_ID = 12,
    UNWDS_RSSIECHO_MODULE_ID = 13,
    UNWDS_PWM_MODULE_ID = 14,
    UNWDS_OPT3001_MODULE_ID = 15,
    UNWDS_DALI_MODULE_ID = 16,
    UNWDS_BME280_MODULE_ID = 17,
    UNWDS_MHZ19_MODULE_ID = 18,
    /* Proprietary 50 to 99 */
    UNWDS_M200_MODULE_ID = 50,
    UNWDS_PULSE_MODULE_ID = 51,
    UNWDS_IBUTTON_MODULE_ID = 52,
    UNWDS_SWITCH_MODULE_ID = 53,
    UNWDS_M230_MODULE_ID = 54,	
    /* Customer 100 to 125*/
    UNWDS_CUSTOMER_MODULE_ID = 100,
    /* System module 126 */
    UNWDS_CONFIG_MODULE_ID = 126,
} UNWDS_MODULE_IDS_t;

typedef struct {
    uint8_t id;
    char    name[20];
    void*   cmd;
    void*   reply;
} unwds_module_desc_t;

#endif