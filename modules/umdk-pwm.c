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
 * @file	umdk-mercury.c
 * @brief   umdk-mercury message parser
 * @author   Mikhail Perkov
 * @author  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

void umdk_pwm_command(char *param, char *out, int bufsize) {
	if (strstr(param, "set ") == param) { 
		param += strlen("set ");    // Skip command
		uint8_t cmd = 0;
		
		if(strstr(param, "freq ") == param) {
			param += strlen("freq ");				// Skip command
			uint32_t freq = strtol(param, &param, 10);
			param += strlen(" ");    						// Skip space
	
			if(strstr(param, "dev ") == param) {
				param += strlen("dev ");				// Skip command
				uint8_t dev = strtol(param, &param, 10);
				dev--;
				param += strlen(" ");    						// Skip space				
				
				uint8_t mask;
				if(strstr(param, "on ") == param) {
					param += strlen("on ");				// Skip command
					mask = 1;
				}
				else if(strstr(param, "off ") == param) {
					param += strlen("off ");				// Skip command
					mask = 0;							
				}
				
				if(strstr(param, "ch ") == param) {	
					param += strlen("ch ");				// Skip command					
					uint8_t channel = strtol(param, &param, 10); 
					channel--;
					param += strlen(" ");    						// Skip space
					
					if(strstr(param, "duty ") == param) {							
						param += strlen("duty ");				// Skip command		
						uint8_t duty = strtol(param, &param, 10); 				
						
						freq = freq & 0x000FFFFF;	
						uint32_t cmd_freq_dev = (cmd << 28) + (freq << 8) + (dev << 0);
						uint8_t mask_ch = (mask << 4) + (channel << 0);
						
						snprintf(out, bufsize, "%08x%02x%02x", cmd_freq_dev, mask_ch, duty);								
					}
				}	
			}
		}
	}

}

bool umdk_pwm_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
	if (moddatalen == 1) {
		if (moddata[0] == 0) {
			add_value_pair(mqtt_msg, "Msg", "Ok");
		} else if(moddata[0] == 1){
			add_value_pair(mqtt_msg, "Msg", "Error");
		}
		return true;
	}
		
	return true;
}
