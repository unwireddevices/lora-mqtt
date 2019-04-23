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
 * @file	umdk-pwm.c
 * @brief   umdk-pwm message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
    UMDK_PWM_OK = 0,
	UMDK_PWM_COMMAND = 1,
	UMDK_PWM_POLL = 2,
    UMDK_PWM_FAIL = 0xFF,
} umdk_pwm_cmd_t;

void umdk_pwm_command(char *param, char *out, int bufsize) {
    uint8_t pin = 0;
    uint16_t freq = 0;
    uint8_t duty = 0;
    uint16_t pulses = 0;
    uint8_t soft = 0;
    
    char *substring = NULL;
    
    substring = strstr(param, "pin");
    if (substring) {
        substring += strlen("pin");
        do {
            substring++;
        } while (*substring == ' ');
        pin = strtol(substring, NULL, 10);
    }
    
    substring = strstr(param, "freq");
    if (substring) {
        substring += strlen("freq");
        do {
            substring++;
        } while (*substring == ' ');
        freq = strtol(substring, NULL, 10);
    }
    
    substring = strstr(param, "duty");
    if (substring) {
        substring += strlen("duty");
        do {
            substring++;
        } while (*substring == ' ');
        duty = strtol(substring, NULL, 10);
    }
    
    substring = strstr(param, "pulses");
    if (substring) {
        substring += strlen("pulses");
        do {
            substring++;
        } while (*substring == ' ');
        pulses = strtol(substring, NULL, 10);
    }
    
    substring = strstr(param, "soft");
    if (substring) {
        substring += strlen("soft");
        do {
            substring++;
        } while (*substring == ' ');
        soft = strtol(substring, NULL, 10);
    }

    snprintf(out, bufsize, "%02x%02x%04x%02x%04x%02x", UMDK_PWM_COMMAND, pin, freq, duty, pulses, soft);
}

bool umdk_pwm_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    if (moddata[0] == UMDK_PWM_COMMAND) {
        switch (moddata[1]) {
            case UMDK_PWM_OK:
                add_value_pair(mqtt_msg, "msg", "ok");
                break;
            case UMDK_PWM_FAIL:
                add_value_pair(mqtt_msg, "msg", "error");
                break;
            default:
                break;
        }
    }
		
	return true;
}
