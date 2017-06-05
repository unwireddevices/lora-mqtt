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
 * @file	umdk-pulse.c
 * @brief   umdk-pulse message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_PULSE_CMD_SET_PERIOD,
    UMDK_PULSE_CMD_POLL,
    UMDK_PULSE_CMD_RESET,
    UMDK_PULSE_CMD_SET_INITIAL_VALUES,
    UMDK_PULSE_CMD_SET_P2L_COEFF,
    UMDK_PULSE_CMD_RESET_TAMPER,
} umdk_pulse_cmd_t;

typedef enum {
    UMDK_PULSE_REPLY_OK,
    UMDK_PULSE_REPLY_UNKNOWN_COMMAND,
    UMDK_PULSE_REPLY_INV_PARAMETER,
    UMDK_PULSE_REPLY_DATA,
    UMDK_PULSE_REPLY_LEAK,
    UMDK_PULSE_REPLY_TAMPER,
} umdk_pulse_reply_t;

#define UMDK_PULSE_NUM_SENS  2

void umdk_pulse_command(char *param, char *out, int bufsize)
{
    if (strstr(param, "period ") == param) {
        param += strlen("period "); // skip command

        uint8_t period = strtol(param, &param, 10);
        printf("[mqtt-counter] Set period: %" PRIu8 " hrs\n", period);

        snprintf(out, bufsize, "%02x%02x", UMDK_PULSE_CMD_SET_PERIOD, period);
    }
    
    if (strstr(param, "coeff ") == param) {
        param += strlen("coeff "); // skip command

        uint8_t coeff = strtol(param, &param, 10);
        printf("[mqtt-counter] Set coefficient: %" PRIu8 " l/p\n", coeff);

        snprintf(out, bufsize, "%02x%02x", UMDK_PULSE_CMD_SET_P2L_COEFF, coeff);
    }
    
    if (strstr(param, "reset") == param) {
        snprintf(out, bufsize, "%02x", UMDK_PULSE_CMD_RESET);
    }
    
    if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "%02x", UMDK_PULSE_CMD_POLL);
    }
    
    if (strstr(param, "tamper") == param) {
        snprintf(out, bufsize, "%02x", UMDK_PULSE_CMD_RESET_TAMPER);
    }
    
    if (strstr(param, "values ") == param) {
        param += strlen("values "); // skip command
        
        uint8_t coeff = strtol(param, &param, 10);
        
        uint32_t values[UMDK_PULSE_NUM_SENS];
        int i = 0;
        for (i = 0; i < UMDK_PULSE_NUM_SENS; i++) {
            values[i] = strtol(param, &param, 10);
            uint32_to_le(&values[i]);
        }
        /*
        uint32_t v_compressed[3];
        
        v_compressed[0]  = values[0] << 8;
        v_compressed[0] |= (values[1] >> 16) & 0xFF;
        
        v_compressed[1] = values[1] << 16;
        v_compressed[1] |= (values[2] >> 8) & 0xFFFF;
        
        v_compressed[2] = (values[2] << 24);
        v_compressed[2] |= values[3] & 0xFFFFFF;
        
        for (i = 0; i < 3; i++ ) {
            uint32_to_le(&v_compressed[i]);
        }
        
        snprintf(out, bufsize, "%02x%02x%08x%08x%08x",
                    UMDK_PULSE_CMD_SET_INITIAL_VALUES,
                    coeff,
                    v_compressed[0], v_compressed[1], v_compressed[2]);
        */
        snprintf(out, bufsize, "%02x%02x%08x%08x",
                                UMDK_PULSE_CMD_SET_INITIAL_VALUES,
                                coeff,
                                values[0], values[1]);
    }
    
    return;
}


bool umdk_pulse_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    int i = 0;

    switch (moddata[0]) {
        case UMDK_PULSE_REPLY_OK:
            add_value_pair(mqtt_msg, "msg", "ok");
            break;
        case UMDK_PULSE_REPLY_UNKNOWN_COMMAND:
            add_value_pair(mqtt_msg, "msg", "invalid command");
            break;
        case UMDK_PULSE_REPLY_INV_PARAMETER:
            add_value_pair(mqtt_msg, "msg", "invalid parameter");
            break;
        case UMDK_PULSE_REPLY_LEAK:
            add_value_pair(mqtt_msg, "msg", "leak");
            break;
        case UMDK_PULSE_REPLY_TAMPER:
            add_value_pair(mqtt_msg, "msg", "tamper");
            break;
        case UMDK_PULSE_REPLY_DATA: {
            uint8_t tamper = moddata[1] & (1<<7);
            uint8_t hours = (moddata[1] >> 2) & 0b11111;
            uint8_t channels = (moddata[1] & 0b11) + 1;
            
            /* absolute data compressed to 3 bytes per counter, has to be decoded to regular UINT32 */
            uint32_t values[channels];
            
            int k = 0;

            for (i = 0; i < channels; i++) {
                values[i] = moddata[2 + i*3] | (moddata[2 + i*3 + 1] << 8) | (moddata[2 + i*3 + 2] << 16);
                /* uint32_to_le(&values[i]); */
            }
            /* most recent absolute data are in values[i].num now */
                        
            /* let's decode hourly data */
            uint32_t history[channels][hours];
            for (i = 0; i < channels; i++) {
                for (k = 0; k < hours - 1; k++) {
                    uint16_t tmp16 = moddata[2 + channels*3 + i*2*(hours - 1) + k*2] | (moddata[2 + channels*3 + i*2*(hours - 1) + k*2] << 8);
                    /* uint16_to_le(&tmp16); */
                    history[i][k + 1] = tmp16;
                }
                history[i][0] = values[i];
            }
            
            /* convert delta values to absolute values */
            for (i = 0; i < channels; i++) {
                for (k = 1; k < hours; k++) {
                    history[i][k] = history[i][k-1] - history[i][k];
                }
            }
            
            /* now history[i] holds absolute values for counter `i` for `hours` hours */
            
            char ch[10];
            char strtmp[10];
            
            for (i = 0; i < channels; i++) {
                snprintf(buf, 3, "[ ");
                for (k = 0; k < hours; k++) {
                    snprintf(strtmp, 10, "%" PRIu32, history[i][k]);
                    strcat(buf, strtmp);
                    if (k < hours - 1 ) {
                        strcat(buf, ",");
                    }
                    strcat(buf, " ");
                }
                strcat(buf, "]");
                snprintf(ch, 10, "P%d", i+1);
                add_value_pair(mqtt_msg, ch, buf);
            }
            
            add_value_pair(mqtt_msg, "tamper", tamper? "broken" : "ok");
            
            break;
        }
    }
    
    return true;
}
