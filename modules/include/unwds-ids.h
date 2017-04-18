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
    UNWDS_4BTN_MODULE_ID,
    UNWDS_GPS_MODULE_ID,
    UNWDS_LSM6DS3_MODULE_ID,
    UNWDS_LM75_MODULE_ID,
    UNWDS_LMT01_MODULE_ID,
    UNWDS_UART_MODULE_ID,
    UNWDS_SHT21_MODULE_ID,
    UNWDS_PIR_MODULE_ID,
    UNWDS_ADC_MODULE_ID,
    UNWDS_LPS331_MODULE_ID,
    UNWDS_COUNTER_MODULE_ID,
    UNWDS_RSSIECHO_MODULE_ID,
    UNWDS_PWM_MODULE_ID,
    UNWDS_OPT3001_MODULE_ID,
    UNWDS_DALI_MODULE_ID,
    UNWDS_BME280_MODULE_ID,
    UNWDS_MHZ19_MODULE_ID,
    /* Proprietary 100 to 199 */
    UNWDS_MERCURY_MODULE_ID = 100,
    UNWDS_NAMUR_MODULE_ID = 101,
    UNWDS_PULSE_MODULE_ID = 102,
    /* Customer 200 to 250*/
    UNWDS_CUSTOMER_MODULE_ID = 200,
    /* System 251 to 254 */
    UNWDS_CONFIG_MODULE_ID = 251,
} UNWDS_MODULE_IDS_t;

typedef struct {
    uint8_t id;
    char    name[20];
    void*   cmd;
    void*   reply;
} unwds_module_desc_t;

#endif