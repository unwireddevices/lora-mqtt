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
 * @file	umdk-idcard.c
 * @brief   umdk-idcard message parser
 * @author  Oleg Artamonov [oleg@unwds.com]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum {
	UMDK_IDCARD_CMD_SET_PERIOD = 0,
	UMDK_IDCARD_CMD_COLLECT,
	UMDK_IDCARD_CMD_AUTH,
    UMDK_IDCARD_CMD_ALARM,
    UMDK_IDCARD_CMD_LOCATION,
    UMDK_IDCARD_CMD_SWITCH_GPS,
} umdk_idcard_cmd_t;

typedef enum {
    UMDK_IDCARD_REPLY_FINGERPRINT_SET = 0,
    UMDK_IDCARD_REPLY_PACKET,
    UMDK_IDCARD_REPLY_ACK_OK,
    UMDK_IDCARD_REPLY_ACK_ERR,
} umdk_reply_action_t;

typedef enum {
    UMDK_IDCARD_PACKET_ID = 0,
    UMDK_IDCARD_BIOMETRY = 1,
    UMDK_IDCARD_GPS = 2,
    UMDK_IDCARD_RESERVED = 9,
    UMDK_IDCARD_ALARM = 10,
    UMDK_IDCARD_MESSAGE_ID = 11,
    UMDK_IDCARD_MESSAGE_ID2 = 12,
    UMDK_IDCARD_MESSAGE_LENGTH = 13,
} umdk_idcard_packet_fields_t;

#include "unwds-modules.h"
#include "utils.h"

void umdk_idcard_command(char *param, char *out, int bufsize) {
    if (strstr(param, "fingerprint ") == param) {
        param += strlen("fingerprint ");    // Skip command
        if (strstr(param, "set ") == param) {
            param += strlen("set ");
            uint8_t cell = strtol(param, &param, 10);
            uint16_t cmdid = strtol(param, &param, 10);
            snprintf(out, bufsize, "%02x%02x%04x", UMDK_IDCARD_CMD_COLLECT, cell, cmdid);
        } 
    } else if (strstr(param, "get ") == param) {
        param += strlen("get ");    // Skip command
        if (strstr(param, "fingerprint ") == param) {
            param += strlen("fingerprint ");
            uint16_t cmdid = strtol(param, &param, 10);
            snprintf(out, bufsize, "%02x%04x", UMDK_IDCARD_CMD_AUTH, cmdid);
        } else if (strstr(param, "location ") == param) {
            param += strlen("location ");
            uint16_t cmdid = strtol(param, &param, 10);
            snprintf(out, bufsize, "%02x%04x", UMDK_IDCARD_CMD_LOCATION, cmdid);
        }
    } else if (strstr(param, "alarm ") == param) {
        param += strlen("alarm ");
        uint16_t cmdid = strtol(param, &param, 10);
        snprintf(out, bufsize, "%02x%04x", UMDK_IDCARD_CMD_ALARM, cmdid);
    } else if (strstr(param, "gps ") == param) {
        param += strlen("gps ");
        uint8_t gps_state = 1;
        if (strstr(param, "on ") == param) {
            gps_state = 1;
            param += strlen("on ");
        } else {
            gps_state = 0;
            param += strlen("off ");
        }
        uint16_t cmdid = strtol(param, &param, 10);
        snprintf(out, bufsize, "%02x%02x%04x", UMDK_IDCARD_CMD_SWITCH_GPS, gps_state, cmdid);
    }
}

bool umdk_idcard_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];
    uint16_t message_id;

    switch (moddata[UMDK_IDCARD_PACKET_ID]) {
        case UMDK_IDCARD_REPLY_ACK_OK:
            add_value_pair(mqtt_msg, "msg", "ok");
            break;
        case UMDK_IDCARD_REPLY_ACK_ERR:
            add_value_pair(mqtt_msg, "msg", "error");
            break;
        case UMDK_IDCARD_REPLY_FINGERPRINT_SET:
            add_value_pair(mqtt_msg, "fingerprint", "ok");
            break;
        case UMDK_IDCARD_REPLY_PACKET:
            if (moddata[UMDK_IDCARD_BIOMETRY] == 0) {
                add_value_pair(mqtt_msg, "biometry", "passed");
            } else if (moddata[UMDK_IDCARD_BIOMETRY] == 1) {
                add_value_pair(mqtt_msg, "biometry", "failed");
            } else if (moddata[UMDK_IDCARD_BIOMETRY] == 2) {
                add_value_pair(mqtt_msg, "biometry", "skipped");
            }
            
            add_value_pair(mqtt_msg, "fingerprints", "1");
            
            /* GPS data parser */
            uint8_t *bytes = &moddata[UMDK_IDCARD_GPS];
            gps_data_t gps;
            parse_gps_data(&gps, bytes, false);
            
            if (!gps.ready) {
                snprintf(buf, sizeof(buf), "absent");
            } else if (!gps.valid) {
                snprintf(buf, sizeof(buf), "invalid");
            } else {
                int lat_d, lon_d;
                double lat, lon;
                lat_d = 100*modf(gps.latitude, &lat);
                lon_d = 100*modf(gps.longitude, &lon);
                
                snprintf(buf, sizeof(buf), "%04d.%02d%s, %05d.%02d%s",
                        abs((int)lat), abs(lat_d), (gps.latitude)>0?"N":"S",
                        abs((int)lon), abs(lon_d), (gps.longitude)>0?"E":"W");
            }
            
            add_value_pair(mqtt_msg, "gps", buf);
            
            snprintf(buf, sizeof(buf), "%d", moddata[UMDK_IDCARD_ALARM]);
            add_value_pair(mqtt_msg, "alarm", buf);

            break;
    }
    
    message_id = moddata[UMDK_IDCARD_MESSAGE_ID] | moddata[UMDK_IDCARD_MESSAGE_ID + 1] << 8;
    uint16_to_le(&message_id);
    sprintf(buf, "%" PRIu16, message_id);
    add_value_pair(mqtt_msg, "mid", buf);
    return true;
}
