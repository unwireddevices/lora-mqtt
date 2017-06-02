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

void umdk_gps_command(char *param, char *out, int bufsize) {
    if (strstr(param, "get") == param) {
        snprintf(out, bufsize, "00");
    }
}

bool umdk_gps_reply(uint8_t *moddata, int moddatalen, mqtt_msg_t *mqtt_msg)
{
    char buf[100];

    if (moddatalen < 1) {
        return false;
    }

    uint8_t reply = moddata[0] & 3; /* Last 4 bits is reply type */
    switch (reply) {
        case 0: { /* GPS data */
            if (moddatalen != 1 + 6) { /* There must be 6 bytes of GPS data + 1 byte of reply type */
                return false;
            }

            uint8_t *bytes = (uint8_t *) (moddata + 1);

            int lat, lat_d, lon, lon_d;

            /* This code is endian-safe */
            lat = bytes[0] + (bytes[1] << 8);
            lat_d = bytes[2];
            lon = bytes[3] + (bytes[4] << 8);
            lon_d = bytes[5];

            /* Apply sign bits from reply */
            if ((moddata[0] >> 5) & 1) {
                lat = -lat;
            }

            if ((moddata[0] >> 6) & 1) {
                lon = -lon;
            }

            add_value_pair(mqtt_msg, "valid", "true");
            snprintf(buf, sizeof(buf), "%03d.%d", lat, lat_d);
            add_value_pair(mqtt_msg, "lat", buf);
            snprintf(buf, sizeof(buf), "%04d.%d", lon, lon_d);
            add_value_pair(mqtt_msg, "lon", buf);
            break;
        }
        case 1: { /* No data yet */
            add_value_pair(mqtt_msg, "valid", "false");
            add_value_pair(mqtt_msg, "lat", "null");
            add_value_pair(mqtt_msg, "lon", "null");
            break;
        }
        case 3: { /* Error occured */
            add_value_pair(mqtt_msg, "valid", "false");
            add_value_pair(mqtt_msg, "msg", "error");
            break;
        }
        default:
            return false;
    }
    return true;
}
