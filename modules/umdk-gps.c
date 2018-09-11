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
 * @file	umdk-gps.c
 * @brief   umdk-gps message parser
 * @author  Eugeny Ponomarev [ep@unwds.com]
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwds-modules.h"
#include "utils.h"

typedef enum {
	UMDK_GPS_DATA = 0,
    UMDK_GPS_COMMAND = 1,
	UMDK_GPS_CMD_POLL = 2,
	UMDK_GPS_REPLY_ERROR = 0xFF,
} umdk_gps_cmd_t;

void umdk_gps_command(char *param, char *out, int bufsize) {
    if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "00");
    }
}

bool umdk_gps_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

    if (moddata[0] == UMDK_GPS_DATA) {
        add_value_pair(mqtt_msg, "valid", (moddata[1] > 0)?"true":"false");
        
        int32_t latitude;
        memcpy((void *)&latitude, &moddata[2], 4);
        convert_from_be_sam((void *)&latitude, sizeof(latitude));
        
        int32_t longitude;
        memcpy((void *)&longitude, &moddata[6], 4);
        convert_from_be_sam((void *)&longitude, sizeof(longitude));
        
        uint16_t velocity;
        memcpy((void *)&velocity, &moddata[10], 2);
        convert_from_be_sam((void *)&velocity, sizeof(velocity));
        
        uint16_t direction;
        memcpy((void *)&direction, &moddata[12], 2);
        convert_from_be_sam((void *)&direction, sizeof(direction));
        
        snprintf(buf, sizeof(buf), "%f", (double)latitude/1000000);
        add_value_pair(mqtt_msg, "lat", buf);
        snprintf(buf, sizeof(buf), "%f", (double)longitude/1000000);
        add_value_pair(mqtt_msg, "lon", buf);
        snprintf(buf, sizeof(buf), "%d", velocity);
        add_value_pair(mqtt_msg, "vel", buf);
        snprintf(buf, sizeof(buf), "%f", (double)direction/100);
        add_value_pair(mqtt_msg, "dir", buf);
    }
    
    if (moddata[0] == UMDK_GPS_COMMAND) {
        add_value_pair(mqtt_msg, "msg", "ok");
    }

    if (moddata[0] == UMDK_GPS_REPLY_ERROR) {
        add_value_pair(mqtt_msg, "msg", "error");
    }
    
    return true;
}
