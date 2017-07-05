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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <mosquitto.h>
#include <pthread.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/msg.h>
#include <sys/queue.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "mqtt.h"
#include "unwds-mqtt.h"
#include "utils.h"

#define VERSION "2.1.0"

#define MAX_PENDING_NODES 1000

#define RETRY_TIMEOUT_S 35
#define INVITE_TIMEOUT_S 45

#define NUM_RETRIES 5
#define NUM_RETRIES_INV 5
#define NUM_RETRIES_BEFORE_INVITE 2

#define UART_POLLING_INTERVAL 100	// milliseconds
#define QUEUE_POLLING_INTERVAL 1 	// milliseconds
#define REPLY_LEN 1024

int msgqid;
extern int errno;

struct msg_buf {
  long mtype;
  char mtext[REPLY_LEN];
} msg_rx;

static struct mosquitto *mosq = NULL;
static int uart = 0;

static pthread_t publisher_thread;
static pthread_t reader_thread;
static pthread_t pending_thread;

static pthread_mutex_t mutex_uart;
static pthread_mutex_t mutex_pending;

static uint8_t mqtt_format;

char logbuf[1024];

typedef struct entry {
	TAILQ_ENTRY(entry) entries;   /* Circular queue. */	
	char buf[REPLY_LEN];
} cq_entry_t;

TAILQ_HEAD(TAILQ, entry) inputq;
typedef struct TAILQ fifo_t;

static bool m_enqueue(fifo_t *l, char *v);
static bool m_dequeue(fifo_t *l, char *v);
static bool is_fifo_empty(fifo_t *l);

/* Pending messages queue pool */
static bool pending_free[MAX_PENDING_NODES];
typedef struct {
	uint64_t nodeid;
    fifo_t pending_fifo;
    
	time_t last_msg;
	time_t last_inv;
    
	unsigned short nodeclass;
	bool has_been_invited;
	bool can_send;
	unsigned short num_retries;
	unsigned short num_pending;
} pending_item_t;

static pending_item_t pending[MAX_PENDING_NODES];

/* The devices list is requested for gate needs, so don't post in MQTT it's results */
static bool list_for_gate = false;
static bool devlist_needed = false;
static void devices_list(bool internal);

/* If too many pings was skipped by gate, the connection might be faulty */
/*
static int pings_skipped = 0;
static const int MIN_PINGS_SKIPPED = 10;
*/

static bool static_devices_list_sent = false;

static char *get_node_class(unsigned short nodeclass) {
	switch (nodeclass) {
		case 0:
			return "A";

		case 1:
			return "B";

		case 2:
			return "C";

		default:
			return "?";
	}
}

static void init_pending(void) {
	int i;	
	for (i = 0; i < MAX_PENDING_NODES; i++) {
		pending_free[i] = true;
		pending[i].nodeid = 0;
		pending[i].nodeclass = 0;
		pending[i].last_msg = 0;
		pending[i].last_inv = 0;
		pending[i].num_retries = 0;
		pending[i].can_send = false;
		pending[i].num_pending = 0;
	}
}

static pending_item_t *pending_to_nodeid(uint64_t nodeid) {
	int i;	
	for (i = 0; i < MAX_PENDING_NODES; i++) {
		if (pending_free[i])
			continue;

		if (pending[i].nodeid == nodeid) {
			return &pending[i];
		}
	}	

	return NULL;
}

static bool add_device(uint64_t nodeid, unsigned short nodeclass, bool was_joined) {
	pthread_mutex_lock(&mutex_pending);

	pending_item_t *e = pending_to_nodeid(nodeid);

	/* Update device info for existing record */
	if (e != NULL) {
		/* Clear invitation flag */
		if (nodeclass == LS_ED_CLASS_C && e->has_been_invited && was_joined) {
			snprintf(logbuf, sizeof(logbuf), "[+] Device successfully invited");
			logprint(logbuf);

			e->has_been_invited = false;
		}

		e->nodeclass = nodeclass;
		pthread_mutex_unlock(&mutex_pending);

		/* Reset number of retransmission/invite attempts */
		e->num_retries = 0;

		return true;
	}

	int i;
	for (i = 0; i < MAX_PENDING_NODES; i++) {
		/* Free cell found, occupy */
		if (pending_free[i]) {
			pending_free[i] = false;

			/* Initialize cell */
			pending[i].nodeid = nodeid;
			pending[i].nodeclass = nodeclass;
			pending[i].has_been_invited = !was_joined; /* Node added without actual join via invitation */

			pending[i].last_msg = 0;
			pending[i].last_inv = 0;
			pending[i].num_retries = 0;
			pending[i].can_send = false;
			pending[i].num_pending = 0;

			/* Initialize queue in cell */
			TAILQ_INIT(&pending[i].pending_fifo);

			pthread_mutex_unlock(&mutex_pending);
			return true;
		}
	}

	pthread_mutex_unlock(&mutex_pending);
	return false;
}

static bool kick_device(uint64_t nodeid) {
	pthread_mutex_lock(&mutex_pending);

	int i;
	for (i = 0; i < MAX_PENDING_NODES; i++) {
		if (pending_free[i])
			continue;

		if (pending[i].nodeid != nodeid)
			continue;

		/* Device found, kick */
		pending_free[i] = true;

		/* Empty the pending queue */
		while (m_dequeue(&pending[i].pending_fifo, NULL)) {}

		pthread_mutex_unlock(&mutex_pending);

		return true;
	}	

	pthread_mutex_unlock(&mutex_pending);

	return false;
}

static bool m_enqueue(fifo_t *l, char *v)
{
	cq_entry_t *val;
	val = (cq_entry_t *)malloc(sizeof(cq_entry_t));
	if (val != NULL) {
		memcpy(val->buf, v, strlen(v)+1);
		TAILQ_INSERT_TAIL(l, val, entries);
		return true;
	}

	return false;
}
 
static bool m_dequeue(fifo_t *l, char *v)
{
	cq_entry_t *e = l->tqh_first;
	
	if (e != NULL) {
		if (v != NULL)
			memcpy(v, e->buf, strlen(e->buf)+1);

		TAILQ_REMOVE(l, e, entries);
		free(e);
		e = NULL;
		return true;
	}

	return false;
}

static bool m_peek(fifo_t *l, char *v)
{
	cq_entry_t *e = l->tqh_first;
	
	if (e != NULL) {
		if (v != NULL)
			memcpy(v, e->buf, strlen(e->buf)+1);
		else
			return false;	/* Makes no sense to peek into NULL buffer */

		return true;
	}

	return false;	
}

static bool is_fifo_empty(fifo_t *l)
{
	if (l->tqh_first == NULL) 
		return true;

	return false;
}

static int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                fprintf(stderr, "error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        // tty.c_iflag &= ~IGNBRK;         // disable break processing
		tty.c_iflag |= IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                fprintf(stderr, "error %d from tcsetattr\n", errno);
                return -1;
        }
        return 0;
}

static void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                fprintf(stderr, "error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                fprintf(stderr, "error %d setting term attributes", errno);
}

static void serve_reply(char *str) {
    puts("[info] Gate reply received");
    
	if (strlen(str) > REPLY_LEN * 2) {
		puts("[error] Received too long reply from the gate");
		return;
	}

	gate_reply_type_t reply = (gate_reply_type_t)str[0];
	str += 1;
    char *str_orig = str;

	switch (reply) {
		case REPLY_LIST: {
            puts("[info] Reply type: REPLY_LIST");
			/* Read EUI64 */
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse list reply: %s\n", str_orig);
				logprint(logbuf);
				return;
			}

			/* Read APPID64 */
			char appid[17] = {};
			memcpy(appid, str, 16);
			str += 16;

			uint64_t appid64;
			if (!hex_to_bytes(appid, (uint8_t *) &appid64, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse list reply: %s\n", str_orig);
				logprint(logbuf);
				return;
			}

			/* Read last seen time */
			char lastseen[5] = {};
			memcpy(lastseen, str, 4);
			str += 4;

			uint16_t lseen;
			if (!hex_to_bytes(lastseen, (uint8_t *) &lseen, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse list reply: %s\n", str_orig);
				logprint(logbuf);
				return;
			}

			/* Read nodeclass */
			char nodeclass[5] = {};
			memcpy(nodeclass, str, 4);
			str += 4;

			uint16_t cl;
			if (!hex_to_bytes(nodeclass, (uint8_t *) &cl, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse list reply: %s\n", str_orig);
				logprint(logbuf);
				return;
			}

			/* Refresh our internal device list info for that node */
			if (!add_device(nodeid, cl, true)) {
				snprintf(logbuf, sizeof(logbuf), "[error] Was unable to add device 0x%s with nodeclass %s to our device list!\n", addr, nodeclass);
				logprint(logbuf);
				return;
			}


			/* The device list was requested by gate, don't post results in MQTT then */
			if (list_for_gate) 
				return;

			char *msg = (char *) malloc(MQTT_MAX_MSG_SIZE);
			snprintf(msg, MQTT_MAX_MSG_SIZE, "{ \"appid64\": \"0x%s\", \"last_seen\": %d, \"nodeclass\": %d }", 
					appid, (unsigned) lseen, (unsigned) cl);

            publish_mqtt_message(mosq, addr, "list/", msg, (mqtt_format_t) mqtt_format);
            free(msg);
		}
		break;

		case REPLY_IND: {
            puts("[info] Reply type: REPLY_IND");
            
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse device app. data: %s", str_orig);
				logprint(logbuf);
				return;
			}

			int16_t rssi;
			if (!hex_to_bytesn(str, 4, (uint8_t *) &rssi, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse RSSI from gate reply: %s\n", str);
				logprint(logbuf);
				return;
			}

			/* Skip RSSI hex */
			str += 4;
            
            uint8_t status;
            if (!hex_to_bytesn(str, 2, &status, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse status from gate reply: %s\n", str);
				logprint(logbuf);
				return;
			}
            
            /* Skip status hex */
            str += 2;

			uint8_t bytes[REPLY_LEN] = {};
			if (!hex_to_bytes(str,  (uint8_t *) &bytes, false)) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse payload bytes gate reply: \"%s\" | len: %zu\n", str, strlen(str));
				logprint(logbuf);
				return;
			}
            
			int moddatalen = strlen(str + 1) / 2;

			uint8_t modid = bytes[0];
			uint8_t *moddata = bytes + 1;

			char *topic = (char *)malloc(64);
			char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
            
            mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
            memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
            
            mqtt_status_t mqtt_status;           
            mqtt_status.rssi = rssi;
            mqtt_status.battery = 2000 + (50*(status & 0x1F));
            mqtt_status.temperature = 20*(status >> 5) - 30;

			if (!convert_to(modid, moddata, moddatalen, topic, mqtt_msg)) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to convert gate reply \"%s\" for module %d\n", str, modid);
				logprint(logbuf);
				return;
			}
            
            build_mqtt_message(msg, mqtt_msg, mqtt_status, addr);           
            publish_mqtt_message(mosq, addr, topic, msg, (mqtt_format_t) mqtt_format);
            free(topic);
            free(msg);
            free(mqtt_msg);
		}
		break;

		case REPLY_JOIN: {
            puts("[info] Reply type: REPLY_JOIN");

			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse join reply: %s", str_orig);
				logprint(logbuf);
				return;
			}

			unsigned short nodeclass = atoi(str);
            char *cl = get_node_class(nodeclass);
			snprintf(logbuf, sizeof(logbuf), "[join] Joined device with id = 0x%" PRIx64 " and class = %s\n", 
				     nodeid, cl);
			logprint(logbuf);
            
            mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
            memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
            
            add_value_pair(mqtt_msg, "joined", "1");            
            add_value_pair(mqtt_msg, "class", cl);

            mqtt_status_t status = { 0 };
            
            char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
            build_mqtt_message(msg, mqtt_msg, status, addr);           
            publish_mqtt_message(mosq, addr, "device", msg, (mqtt_format_t) mqtt_format);
            free(msg);
            free(mqtt_msg);

			add_device(nodeid, nodeclass, true);

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e != NULL) {
				/* If device is rejoined, check the pending messages */
				if (e->num_pending) {
					/* Notify gate about pending messages */
					pthread_mutex_lock(&mutex_uart);
					dprintf(uart, "%c%" PRIx64 "%02x\r", CMD_HAS_PENDING, 
						    nodeid, e->num_pending);
					pthread_mutex_unlock(&mutex_uart);	
				}				
			}
		}
		break;

		case REPLY_KICK: {
            puts("[info] Reply type: REPLY_KICK");
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse kick packet: %s", str_orig);
				logprint(logbuf);
				return;
			}

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e == NULL)
				break;

			if (kick_device(nodeid)) {
				snprintf(logbuf, sizeof(logbuf), "[kick] Device with id = 0x%" PRIx64 " kicked due to long silence\n", nodeid);
				logprint(logbuf);
                
                mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
                memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
                add_value_pair(mqtt_msg, "joined", "0");            
                mqtt_status_t status = { 0 };
                
                char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
                build_mqtt_message(msg, mqtt_msg, status, addr);           
                publish_mqtt_message(mosq, addr, "device", msg, (mqtt_format_t) mqtt_format);
                free(msg);
                free(mqtt_msg);
			}
		}
		break;

		case REPLY_ACK: {
            puts("[info] Reply type: REPLY_ACK");

			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse ack: %s", str_orig);
				logprint(logbuf);
				return;
			}

			snprintf(logbuf, sizeof(logbuf), "[ack] ACK received from %" PRIx64 "\n", nodeid);
			logprint(logbuf);

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e == NULL)
				break;

			pthread_mutex_lock(&mutex_pending);
			/* No need to invite device */
			e->has_been_invited = false;

			/* Dequeue pending message */
			if (!is_fifo_empty(&e->pending_fifo))
				m_dequeue(&e->pending_fifo, NULL);

			if (e->num_pending > 0)
				e->num_pending--;

			e->num_retries = 0;
			pthread_mutex_unlock(&mutex_pending);			
		}
		break;

		case REPLY_PENDING_REQ: {
            puts("[info] Reply type: REPLY_PENDING_REQ");

			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Unable to parse pending frames request data: %s", str_orig);
				logprint(logbuf);
				return;
			}

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e != NULL) {
				/* Check if there's pending frames for this class A device */
				if (e->nodeclass == LS_ED_CLASS_A && e->num_pending > 0) {
					snprintf(logbuf, sizeof(logbuf), "[pending] Gate requested next pending frame for 0x%" PRIx64 "\n", nodeid);
					logprint(logbuf);					

					pthread_mutex_lock(&mutex_pending);
					e->can_send = true;
					e->last_msg = 0; /* Force immediate sending */
					pthread_mutex_unlock(&mutex_pending);		
				}
			}			
		}
		break;
        default:
            puts("[error] Reply type: unknown reply type");
            break;
	}
}

static void invite_mote(uint64_t addr) 
{
	snprintf(logbuf, sizeof(logbuf), "[inv] Sending invitation to node with address 0x%" PRIx64 "\n", addr);
	logprint(logbuf);
    
    mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
    memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
    add_value_pair(mqtt_msg, "invited", "1");
    add_value_pair(mqtt_msg, "message", "sending invitation to the node");
    mqtt_status_t status = { 0 };
    
    char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
    
    char hexbuf[40];
    snprintf(hexbuf, sizeof(hexbuf), "%" PRIx64, addr);
    build_mqtt_message(msg, mqtt_msg, status, hexbuf);
    publish_mqtt_message(mosq, hexbuf, "device", msg, (mqtt_format_t) mqtt_format);

    free(msg);
    free(mqtt_msg);

	pthread_mutex_lock(&mutex_uart);
	dprintf(uart, "%c%" PRIx64 "\r", CMD_INVITE, addr);
	pthread_mutex_unlock(&mutex_uart);
}

static void* pending_worker(void *arg) {
	(void) arg;

	while (1) {
		pthread_mutex_lock(&mutex_pending);

		int i;	
		for (i = 0; i < MAX_PENDING_NODES; i++) {
			if (pending_free[i]) {
                usleep(1e3 * QUEUE_POLLING_INTERVAL);
				continue;
            }

			pending_item_t *e = &pending[i];
			time_t current = time(NULL);

			if (is_fifo_empty(&e->pending_fifo)) {
                usleep(1e3 * QUEUE_POLLING_INTERVAL);
				continue;
            }

			/* Messages for Class A devices will be sent only on demand */
			if (e->nodeclass == LS_ED_CLASS_A && !e->can_send) {
                usleep(1e3 * QUEUE_POLLING_INTERVAL);
				continue;
            }

			/* Must wait for device to join before sending messages */
			if (e->nodeclass == LS_ED_CLASS_C && e->has_been_invited) {
				if (e->num_retries > NUM_RETRIES_INV) {
					snprintf(logbuf, sizeof(logbuf), "[fail] Unable to invite node 0x%" PRIx64 " to network after %u attempts, giving up\n", e->nodeid, NUM_RETRIES_INV);
					logprint(logbuf);

                    mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
                    memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
                    add_value_pair(mqtt_msg, "invited", "0");
                    add_value_pair(mqtt_msg, "message", "failed to invite node");
                    mqtt_status_t status = { 0 };
                    
                    char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
                    
                    char hexbuf[40];
                    snprintf(hexbuf, 40, "%" PRIx64, e->nodeid);
                    build_mqtt_message(msg, mqtt_msg, status, hexbuf);
                    publish_mqtt_message(mosq, hexbuf, "device", msg, (mqtt_format_t) mqtt_format);

                    free(msg);
                    free(mqtt_msg);

					e->num_retries = 0;
					m_dequeue(&e->pending_fifo, NULL);						
				} else
				if (current - e->last_inv > e->num_retries * INVITE_TIMEOUT_S) {
					/* Retry invitation */
					invite_mote(e->nodeid);

					e->num_retries++;
					e->last_inv = current;

					if (e->num_retries <= NUM_RETRIES_INV) {
						snprintf(logbuf, sizeof(logbuf), "[inv] [%d/%d] Next invitation retry after %d seconds\n", 
										e->num_retries, NUM_RETRIES_INV, e->num_retries * INVITE_TIMEOUT_S);
						logprint(logbuf);
					}
				}

				continue;
			}

			if (current - e->last_msg > RETRY_TIMEOUT_S) {
				if (e->num_retries > NUM_RETRIES) {
					snprintf(logbuf, sizeof(logbuf), "[fail] Unable to send message to 0x%" PRIx64 " after %u attempts, giving up\n", 
						      e->nodeid, NUM_RETRIES);
					logprint(logbuf);
                    
                    mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
                    memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
                    add_value_pair(mqtt_msg, "sent", "0");
                    add_value_pair(mqtt_msg, "message", "failed to send message to the node");
                    mqtt_status_t status = { 0 };
                    
                    char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
                    char hexbuf[40];
                    snprintf(hexbuf, 40, "%" PRIx64, e->nodeid);
                    build_mqtt_message(msg, mqtt_msg, status, hexbuf);
                    publish_mqtt_message(mosq, hexbuf, "device", msg, (mqtt_format_t) mqtt_format);

                    free(msg);
                    free(mqtt_msg);
                    
					e->num_retries = 0;
					m_dequeue(&e->pending_fifo, NULL);

					continue;
				}

				char buf[REPLY_LEN] = {};
				if (!m_peek(&e->pending_fifo, buf)) /* Peek message from queue but don't remove. Will be removed on acknowledge */
					continue;

				snprintf(logbuf, sizeof(logbuf), "[pending] [%d/%d] Sending message to 0x%" PRIx64 ": %s\n", 
					e->num_retries + 1, (e->num_retries < NUM_RETRIES_BEFORE_INVITE) ? NUM_RETRIES_BEFORE_INVITE : NUM_RETRIES,
					e->nodeid, buf);
				logprint(logbuf);

				e->num_retries++;

				/* Send */
				pthread_mutex_lock(&mutex_uart);
				dprintf(uart, "%s\r", buf);
				pthread_mutex_unlock(&mutex_uart);

				/* Send invitation after NUM_RETRIES_BEFORE_INVITE retransmissions */
				if (e->nodeclass == LS_ED_CLASS_C && e->num_retries == NUM_RETRIES_BEFORE_INVITE) {
					e->num_retries = 1;
					e->last_inv = current;
					e->has_been_invited = true;
				}

				e->can_send = false;
				e->last_msg = current;			
			}
		}

		pthread_mutex_unlock(&mutex_pending);
	}

	return 0;
}

/* Publishes messages into MQTT */
static void *publisher(void *arg)
{ 
	while(1) {
        /* Wait for a message to arrive */
        if (msgrcv(msgqid, &msg_rx, sizeof(msg_rx.mtext), 0, 0) < 0) {
            puts("[error] Failed to receive internal message");
            continue;
        }
		serve_reply(msg_rx.mtext);
	}	
	
	return NULL;
}

#define STATIC_DEVS_LIST_FILE "/etc/lora-mqtt/static-devs.conf"

/* 
 * Devices list format:
 *
 * # comment
 * <eui64> <appid64> <network address> <device nonce> <channel>
 *
 * All numbers are in hex with zero padding if required. 
 * Device nonce is a random secret that must be set on the end-device. 
 * Channel is usually zero (one channel gate).
 *
 * Example:
 * abababababababab 0000000000000001 00000000 abababab 00
 *
 * NB: each device line must be 54 characters long
 *
 */
static void send_static_devices_list(void) {
	/* Clear list */
	pthread_mutex_lock(&mutex_uart);
	dprintf(uart, "%c\r", CMD_KICK_ALL_STATIC);
	pthread_mutex_unlock(&mutex_uart);

	/* Send list of statically personalized devices */
	FILE *list = fopen(STATIC_DEVS_LIST_FILE, "r");
	int num = 0;
	if (list)
	{
		char line[255];

		while(fgets(line, 254, list) != NULL)
		{
		    if (line[0] != '#' && strlen(line) >= 54) /* 54 characters long line + '\n' character */
			{
		    	uint8_t eui64[32];
				uint8_t appid64[32];
				uint8_t addr[32];
				uint8_t devnonce[32];
				uint8_t nochannel[32];

				sscanf(line, "%s %s %s %s %s", eui64, appid64, addr, devnonce, nochannel);

				/* Send item to the gate */
				pthread_mutex_lock(&mutex_uart);
				dprintf(uart, "%c%s%s%s%s%s\r", CMD_ADD_STATIC_DEV, eui64, appid64, addr, devnonce, nochannel);
				pthread_mutex_unlock(&mutex_uart);

				num++;
		    }
		}

		fclose(list);
	} else {
		snprintf(logbuf, sizeof(logbuf), "[gate] No statically personalized devices list found (%s)", STATIC_DEVS_LIST_FILE);	
		logprint(logbuf);
		return;
	}

	snprintf(logbuf, sizeof(logbuf), "[gate] List of statically personalized devices (%i pcs) sent", num);	
	logprint(logbuf);
}

/* Periodic read data from UART */
static void *uart_reader(void *arg)
{
	puts("[gate] UART reading thread created");

	while(1) {
		char buf[REPLY_LEN] = { '\0', };
		char c;
		int r = 0, i = 0;

		pthread_mutex_lock(&mutex_uart);

		dprintf(uart, "%c\r", CMD_FLUSH);

		while ((r = read(uart, &c, 1)) != 0) {
			buf[i++] = c;
		}

		pthread_mutex_unlock(&mutex_uart);

		buf[i] = '\0';

		if (strlen(buf) > 0) {
            
            char *running = strdup(buf), *token;
            const char *delims = "\n";
            
            while (strlen(token = strsep(&running, delims))) {
                if((strlen(token) + 1 ) > sizeof(msg_rx.mtext)) {
                    puts("[error] Oversized message, unable to send");
                    continue;
                }
                
                msg_rx.mtype = 1;
                memcpy(msg_rx.mtext, token, strlen(token));
                msg_rx.mtext[strlen(token)] = 0;
                
                if (msgsnd(msgqid, &msg_rx, sizeof(msg_rx.mtext), 0) < 0) {
                    perror( strerror(errno) );
                    printf("[error] Failed to send internal message");
                    continue;
                }
            }
		}

		usleep(1e3 * UART_POLLING_INTERVAL);
		
		/* Request devices list on demand */
		if (devlist_needed) {
			puts("[!] Device list requested");
			devlist_needed = false; /* No more devices lists needed */
			devices_list(true);

			usleep(1e3 * 150);
		}

		if (!static_devices_list_sent) {
			send_static_devices_list();
			static_devices_list_sent = true;
		}
	}

	return NULL;
}

static void devices_list(bool internal) 
{
	list_for_gate = internal;

	pthread_mutex_lock(&mutex_uart);
	dprintf(uart, "%c\r", CMD_DEVLIST);
	pthread_mutex_unlock(&mutex_uart);
}

static void message_to_mote(uint64_t addr, char *payload) 
{
	snprintf(logbuf, sizeof(logbuf), "[gate] Sending individual message to the mote with address \"%" PRIx64 "\": \"%s\"\n", 
					addr, payload);	
	logprint(logbuf);

	pending_item_t *e = pending_to_nodeid(addr);
	if (e == NULL) {
		snprintf(logbuf, sizeof(logbuf), "[error] Mote with id = %" PRIx64 " is not in network, an invite will be sent\n", addr);
		logprint(logbuf);
        mqtt_msg_t *mqtt_msg = (mqtt_msg_t *)malloc(MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
        memset((void *)mqtt_msg, 0, MQTT_MSG_MAX_NUM * sizeof(mqtt_msg_t));
        add_value_pair(mqtt_msg, "sent", "2");
        add_value_pair(mqtt_msg, "message", "node not in the network");
        mqtt_status_t status = { 0 };
                    
        char *msg = (char *)malloc(MQTT_MAX_MSG_SIZE);
        char hexbuf[40];
        snprintf(hexbuf, sizeof(hexbuf), "%" PRIx64, addr);
        build_mqtt_message(msg, mqtt_msg, status, hexbuf);
        publish_mqtt_message(mosq, hexbuf, "device", msg, (mqtt_format_t) mqtt_format);

        free(msg);
        free(mqtt_msg);
        
		add_device(addr, LS_ED_CLASS_C, false);
		e = pending_to_nodeid(addr);
	}

	if (e == NULL) {
		puts("[error] Unable to add new device. Is devices list overflowed?\n");	
		return;
	}

	/* Enqueue the frame as a gate command */
	char buf[REPLY_LEN] = {};
	snprintf(buf, sizeof(buf), "%c%" PRIx64 "%s", CMD_IND, addr, payload);

	pthread_mutex_lock(&mutex_pending);
	if (!m_enqueue(&e->pending_fifo, buf)) {
		snprintf(logbuf, sizeof(logbuf), "[error] Out of memory when adding message to downlink queue for mote with id %" PRIx64 "!\n", addr);
		logprint(logbuf);
		pthread_mutex_unlock(&mutex_pending);
		return;
	}

	if (e->nodeclass == LS_ED_CLASS_A) {
		puts("[pending] Message is delayed");

		e->num_pending++;
		
		/* Notify gate about pending messages */
		pthread_mutex_lock(&mutex_uart);
		dprintf(uart, "%c%" PRIx64 "%02x\r", CMD_HAS_PENDING, addr, e->num_pending);
		pthread_mutex_unlock(&mutex_uart);		
	}

	pthread_mutex_unlock(&mutex_pending);
}

static void message_broadcast(char *payload) {
	snprintf(logbuf, sizeof(logbuf), "[gate] Sending broadcast message: \"%s\"\n", payload);	
	logprint(logbuf);

	/* Send gate command */
	pthread_mutex_lock(&mutex_uart);
	dprintf(uart, "%c%s\r", CMD_BROADCAST, payload);
	pthread_mutex_unlock(&mutex_uart);	
}

static void my_message_callback(struct mosquitto *m, void *userdata, const struct mosquitto_message *message)
{
    /* Ignore messages published by gate itself */
    /* Doesn't work with QoS 0 */
	if (message->mid != 0) {
		return;
    }

	char *running = strdup(message->topic), *token;
	const char *delims = "/";

	char topics[5][128] = {};
	int topic_count = 0;

	while (strlen(token = strsep(&running, delims))) {
		strcpy(topics[topic_count], token);
		topic_count++;

		if (running == NULL)
			break;
	}

	if (topic_count < 2) {
		return;
	}

	if (memcmp(topics[0], "devices", 7) != 0) {
		puts("[mqtt] Got message not from devices topic");	
		return;
	}

	if (memcmp(topics[1], "lora", 4) != 0) {
		puts("[mqtt] Got message not from devices/lora topic");	
		return;	
	}

	if (topic_count == 3 && memcmp(topics[2], "get", 3) == 0) {
		puts("[mqtt] Devices list requested");
		devices_list(false);
	}

	if (topic_count > 3) {
		/* Convert address */
		char *addr = topics[2];
		uint64_t nodeid = 0;
		bool is_broadcast = strcmp(addr, "*") == 0;

		if (!is_broadcast) {
			/* Not a broadcast address, parse it as hex EUI-64 address */
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				snprintf(logbuf, sizeof(logbuf), "[error] Invalid node address: %s\n", addr);
				logprint(logbuf);
				return;
			}
		}

        char *type;
        if (mqtt_sepio) {
            if (memcmp(topics[3], "mosi", 4) != 0) {
                snprintf(logbuf, sizeof(logbuf), "[warning] MQTT message ignored: direction is not MOSI");
				logprint(logbuf);
				return;
            }
            type = topics[4];
        } else {
            type = topics[3];
        }
		
		char buf[REPLY_LEN] = { 0 };
		if (!convert_from(type, (char *)message->payload, buf, REPLY_LEN)) {
			snprintf(logbuf, sizeof(logbuf), "[error] Convert failed. Unable to parse mqtt message: devices/lora/%s : %s, %s\n", addr, type, (char*) message->payload);
			logprint(logbuf);
			return;
		}

		if (!strlen(buf)) {
			snprintf(logbuf, sizeof(logbuf), "[error] Buffer is empty. Unable to parse mqtt message: devices/lora/%s : %s, %s\n", addr, type, (char*) message->payload);
			return;
		}

		if (!is_broadcast) {
			message_to_mote(nodeid, buf);
		} else {
			message_broadcast(buf);
		}
	}
}

static void my_connect_callback(struct mosquitto *m, void *userdata, int result)
{
//	int i;
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		snprintf(logbuf, sizeof(logbuf), "Subscribing to %s\n", MQTT_SUBSCRIBE_TO);
		logprint(logbuf);

		mosquitto_subscribe(mosq, NULL, MQTT_SUBSCRIBE_TO, 2);
	}else{
		snprintf(logbuf, sizeof(logbuf), "Connect failed\n");
		logprint(logbuf);
	}
}

static void my_subscribe_callback(struct mosquitto *m, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	char tmpbuf[100];
	snprintf(logbuf, sizeof(logbuf), "Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		snprintf(tmpbuf, sizeof(tmpbuf), ", %d", granted_qos[i]);
		strcat(logbuf, tmpbuf);
	}
	strcat(logbuf, "\n");
	logprint(logbuf);
}

void usage(void) {
	printf("Usage: mqtt <serial>\nExample: mqtt [-ihdp] -p /dev/ttyS0\n");
	printf("  -i\tIgnore /etc/lora-mqtt/mqtt.conf.\n");
	printf("  -h\tPrint this help.\n");
	printf("  -d\tFork to background.\n");
//	printf("  -r\tRetain last MQTT message.\n");
	printf("  -p <port>\tserial port device URI, e.g. /dev/ttyATH0.\n");
    printf("  -t\tUse MQTT format compatible with Tibbo system.\n");
}

int main(int argc, char *argv[])
{
	const char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
    
    mqtt_qos = 1;
    mqtt_retain = false;

	openlog("lora-mqtt", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);

	snprintf(logbuf, sizeof(logbuf), "=== MQTT-LoRa gate (version: %s) ===\n", VERSION);
	logprint(logbuf);
	
    mqtt_format = UNWDS_MQTT_REGULAR;
    
	bool daemonize = 0;
//	bool retain = 0;
	char serialport[100];
	bool ignoreconfig = 0;
	
	int c;
	while ((c = getopt (argc, argv, "ihdrpt:")) != -1)
    switch (c) {
		case 'd':
			daemonize = 1;
			break;
		case 'i':
			ignoreconfig = 1;
			break;
		case 'h':
			usage();
			return 0;
			break;
        case 't':
            mqtt_format = UNWDS_MQTT_ESCAPED;
//		case 'r':
//			retain = 1;
//			break;
		case 'p':
			if (strlen(optarg) > 100) {
				snprintf(logbuf, sizeof(logbuf), "Error: serial device URI is too long\n");
				logprint(logbuf);
			}
			else {
				strncpy(serialport, optarg, 100);
			}
			break;
		default:
			usage();
			return -1;
    }
	
	// fork to background if needed and create pid file
    int pidfile = 0;
    if (daemonize)
    {
		snprintf(logbuf, sizeof(logbuf), "Attempting to run in the background\n");
		logprint(logbuf);
		
        if (daemon(0, 0))
        {
            snprintf(logbuf, sizeof(logbuf), "Error forking to background\n");
			logprint(logbuf);
            exit(EXIT_FAILURE);
        }
        
        char pidval[10];
        pidfile = open("/var/run/mqtt-lora.pid", O_CREAT | O_RDWR, 0666);
        if (lockf(pidfile, F_TLOCK, 0) == -1)
        {
            exit(EXIT_FAILURE);
        }
        snprintf(pidval, sizeof(pidval), "%d\n", getpid());
        write(pidfile, pidval, strlen(pidval));
    }
	
    
    /* Create message queue */
    msgqid = msgget(IPC_PRIVATE, IPC_CREAT);
    if (msgqid < 0) {
        puts("Failed to create message queue");
        exit(EXIT_FAILURE);
    }
	
	FILE* config = NULL;
    char* token;
	
	if (!ignoreconfig)
        {
            config = fopen( "/etc/lora-mqtt/mqtt.conf", "r" );
            if (config)
            {
                char *line = (char *)malloc(255);
                while(fgets(line, 254, config) != NULL)
                {
                    token = strtok(line, "\t =\n\r");
                    if (token != NULL && token[0] != '#')
                    {
                        if (!strcmp(token, "port"))
                        {
                            strcpy(serialport, strtok(NULL, "\t\n\r"));
                            while( (*serialport == ' ') || (*serialport == '=') )
                            {
                                memmove(serialport, serialport+1, strlen(serialport));
                            }
                        }
                        if (!strcmp(token, "format"))
                        {
                            char *format;
                            format = strtok(NULL, "\t =\n\r");
                            if (!strcmp(format, "mqtt-escaped")) {
                                mqtt_format = UNWDS_MQTT_ESCAPED;
                                puts("MQTT format: quotes escaped");
                            } else {
                                if (!strcmp(format, "mqtt")) {
                                    mqtt_format = UNWDS_MQTT_REGULAR;
                                    puts("MQTT format: regular");
                                }
                            }
                        }
                        if (!strcmp(token, "mqtt_qos")) {
                            char *qos;
                            qos = strtok(NULL, "\t =\n\r");
                            sscanf(qos, "%d", &mqtt_qos);
                            printf("MQTT QoS: %d\n", mqtt_qos);
                        }
                        if (!strcmp(token, "mqtt_retain")) {
                            char *retain;
                            retain = strtok(NULL, "\t =\n\r");
                            if (!strcmp(retain, "true")) {
                                mqtt_retain = true;
                                puts("MQTT retain messages enabled");
                            } else {
                                mqtt_retain = false;
                                puts("MQTT retain messages disabled");
                            }
                        }
                        if (!strcmp(token, "mqtt_sepio")) {
                            char *retain;
                            retain = strtok(NULL, "\t =\n\r");
                            if (!strcmp(retain, "true")) {
                                mqtt_sepio = true;
                                puts("MQTT separate in/out topics enabled");
                            } else {
                                mqtt_sepio = false;
                                puts("MQTT separate in/out topics disabled");
                            }
                        }
                    }
                }
                free(line);
                fclose(config);
            }
			else
			{
				snprintf(logbuf, sizeof(logbuf), "Configuration file /etc/lora-mqtt/mqtt.conf not found\n");
				logprint(logbuf);
				return -1;
			}
        }

	printf("Using serial port device: %s\n", serialport);
	
	pthread_mutex_init(&mutex_uart, NULL);
	pthread_mutex_init(&mutex_pending, NULL);

	/* Request a devices list on a first launch */
	devlist_needed = true;

	init_pending();
    
	uart = open(serialport, O_RDWR | O_NOCTTY | O_SYNC);
	if (uart < 0)
	{
		snprintf(logbuf, sizeof(logbuf), "error %d opening %s: %s\n", errno, serialport, strerror (errno));
		logprint(logbuf);
		usage();
		return 1;
	}
	
	set_interface_attribs(uart, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking(uart, 0);                	 // set no blocking

	if(pthread_create(&reader_thread, NULL, uart_reader, NULL)) {
		snprintf(logbuf, sizeof(logbuf), "Error creating reader thread\n");
		logprint(logbuf);
		return 1;
	}

	if(pthread_create(&publisher_thread, NULL, publisher, NULL)) {
		snprintf(logbuf, sizeof(logbuf), "Error creating publisher thread\n");
		logprint(logbuf);
		return 1;
	}

	if (pthread_create(&pending_thread, NULL, pending_worker, NULL)) {
		snprintf(logbuf, sizeof(logbuf), "Error creating pending queue worker thread");
		logprint(logbuf);
		return 1;
	}

	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	if(!mosq){
		snprintf(logbuf, sizeof(logbuf), "Error: Out of memory.\n");
		logprint(logbuf);
		return 1;
	}
	
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

	if(mosquitto_connect(mosq, host, port, keepalive)){
		snprintf(logbuf, sizeof(logbuf), "Unable to connect.\n");
		logprint(logbuf);
		return 1;
	}

	snprintf(logbuf, sizeof(logbuf), "[mqtt] Entering event loop");
	logprint(logbuf);

	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	if (pidfile)
    {
        lockf(pidfile, F_ULOCK, 0);
        close(pidfile);
        remove("/var/run/mqtt-lora.pid");
    }
	
	syslog(LOG_INFO, "lora-mqtt service stopped");
    closelog();
	
	return 0;
}
