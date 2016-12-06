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

#include <sys/queue.h>

#include "mqtt.h"
#include "unwds-mqtt.h"
#include "utils.h"

#define VERSION "1.5.1"

#define MQTT_SUBSCRIBE_TO "devices/lora/#"
#define MQTT_PUBLISH_TO "devices/lora/"

#define UART_POLLING_INTERVAL 100	// milliseconds
#define QUEUE_POLLING_INTERVAL 10 	// milliseconds

static struct mosquitto *mosq = NULL;
static int uart = 0;

static pthread_t publisher_thread;
static pthread_t reader_thread;
static pthread_t pending_thread;

static pthread_mutex_t mutex_uart;
static pthread_mutex_t mutex_queue;
static pthread_mutex_t mutex_pending;
static pthread_mutex_t mutex_pong;

#define REPLY_LEN 1024

typedef struct entry {
	TAILQ_ENTRY(entry) entries;   /* Circular queue. */	
	char buf[REPLY_LEN];
} cq_entry_t;

TAILQ_HEAD(TAILQ, entry) inputq;
typedef struct TAILQ fifo_t;

// static fifo_t *inputqp;              /* UART input requests queue */

static bool m_enqueue(fifo_t *l, char *v);
static bool m_dequeue(fifo_t *l, char *v);
static bool is_fifo_empty(fifo_t *l);

#define MAX_NODES 128
#define RETRY_TIMEOUT_S 10
#define NUM_RETRIES 5

/* Pending messages queue pool */
static bool pending_free[MAX_NODES];
typedef struct {
	uint64_t nodeid;
	unsigned short nodeclass;

	bool has_been_invited;

	fifo_t pending_fifo;

	bool can_send;
	time_t last_msg;
	unsigned short num_retries;
	unsigned short num_pending;
} pending_item_t;

static pending_item_t pending[MAX_NODES];

/* The devices list is requested for gate needs, so don't post in MQTT it's results */
static bool list_for_gate = false;
static bool devlist_needed = false;
static void devices_list(bool internal);

/* If too many pings was skipped by gate, the connection might be faulty */
static int pings_skipped = 0;
static const int MIN_PINGS_SKIPPED = 10;

static char get_node_class(unsigned short nodeclass) {
	switch (nodeclass) {
		case 0:
			return 'A';

		case 1:
			return 'B';

		case 2:
			return 'C';

		default:
			return '?';
	}
}

static void init_pending(void) {
	int i;	
	for (i = 0; i < MAX_NODES; i++) {
		pending_free[i] = true;
		pending[i].nodeid = 0;
		pending[i].nodeclass = 0;
		pending[i].last_msg = 0;
		pending[i].num_retries = 0;
		pending[i].can_send = false;
		pending[i].num_pending = 0;
	}
}

static pending_item_t *pending_to_nodeid(uint64_t nodeid) {
	int i;	
	for (i = 0; i < MAX_NODES; i++) {
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
			puts("[+] Device successfully invited");
			e->has_been_invited = false;
		}

		e->nodeclass = nodeclass;
		pthread_mutex_unlock(&mutex_pending);

		/* Reset number of retransmission/invite attempts */
		e->num_retries = 0;

		return true;
	}

	int i;
	for (i = 0; i < MAX_NODES; i++) {
		/* Free cell found, occupy */
		if (pending_free[i]) {
			pending_free[i] = false;

			/* Initialize cell */
			pending[i].nodeid = nodeid;
			pending[i].nodeclass = nodeclass;
			pending[i].has_been_invited = !was_joined; /* Node added without actual join via invitation */

			pending[i].last_msg = 0;
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
	for (i = 0; i < MAX_NODES; i++) {
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

static int mid = 42;

static void serve_reply(char *str) {
	if (strlen(str) > REPLY_LEN * 2) {
		puts("[error] Received too long reply from the gate");
		return;
	}

	gate_reply_type_t reply = (gate_reply_type_t)str[0];
	str += 1;

	switch (reply) {
		case REPLY_LIST: {
			/* Read EUI64 */
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				printf("[error] Unable to parse list reply: %s\n", str - 16);
				return;
			}

			/* Read APPID64 */
			char appid[17] = {};
			memcpy(appid, str, 16);
			str += 16;

			uint64_t appid64;
			if (!hex_to_bytes(appid, (uint8_t *) &appid64, !is_big_endian())) {
				printf("[error] Unable to parse list reply: %s\n", str - 32);
				return;
			}

			/* Read ability mask */
			char ability[17] = {};
			memcpy(ability, str, 16);
			str += 16;

			uint64_t abil64;
			if (!hex_to_bytes(ability, (uint8_t *) &abil64, !is_big_endian())) {
				printf("[error] Unable to parse list reply: %s\n", str - 48);
				return;
			}

			/* Read last seen time */
			char lastseen[5] = {};
			memcpy(lastseen, str, 4);
			str += 4;

			uint16_t lseen;
			if (!hex_to_bytes(lastseen, (uint8_t *) &lseen, !is_big_endian())) {
				printf("[error] Unable to parse list reply: %s\n", str - 52);
				return;
			}

			/* Read nodeclass */
			char nodeclass[5] = {};
			memcpy(nodeclass, str, 4);
			str += 4;

			uint16_t cl;
			if (!hex_to_bytes(nodeclass, (uint8_t *) &cl, !is_big_endian())) {
				printf("[error] Unable to parse list reply: %s\n", str - 52);
				return;
			}

			/* Refresh our internal device list info for that node */
			if (!add_device(nodeid, cl, true)) {
				printf("[error] Was unable to add device 0x%s with nodeclass %s to our device list!\n", addr, nodeclass);
				return;
			}


			/* The device list was requested by gate, don't post results in MQTT then */
			if (list_for_gate) 
				return;

			char topic[22] = {};
			strcpy(topic, "list/");
			strcat(topic, addr);

			char msg[128] = {};
			sprintf(msg, "{ appid64: 0x%s, ability: 0x%s, last_seen: %d, nodeclass: %d }", 
					appid, ability, (unsigned) lseen, (unsigned) cl);

			/* Publish message */
			char *mqtt_topic = (char *)malloc(strlen(MQTT_PUBLISH_TO) + strlen(topic) + 1);
			strcpy(mqtt_topic, MQTT_PUBLISH_TO);
			strcat(mqtt_topic, topic);

			printf("[mqtt] Publishing to the topic %s the message \"%s\"\n", mqtt_topic, msg);

			mosquitto_publish(mosq, 0, mqtt_topic, strlen(msg), msg, 1, false);

			free(mqtt_topic);
		}
		break;

		case REPLY_IND: {
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				printf("[error] Unable to parse device app. data: %s", str - 16);
				return;
			}
	
			uint16_t rssi_buf;
			if (!hex_to_bytesn(str, 4, (uint8_t *) &rssi_buf, !is_big_endian())) {
				printf("[error] Unable to parse RSSI from gate reply: %s\n", str);
				return;
			}
			int16_t rssi = -rssi_buf;

			/* Skip RSSI hex */
			str += 4;

			uint8_t bytes[REPLY_LEN] = {};
			if (!hex_to_bytes(str,  (uint8_t *) &bytes, false)) {
				printf("[error] Unable to parse payload bytes gate reply: \"%s\" | len: %d\n", str, strlen(str));
				return;
			}

			int moddatalen = strlen(str + 1) / 2;

			uint8_t modid = bytes[0];
			uint8_t *moddata = bytes + 1;

			char topic[64] = {};
			char msg[128] = {};

			if (!convert_to(modid, moddata, moddatalen, (char *) &topic, (char *) &msg)) {
				printf("[error] Unable to convert gate reply \"%s\" for module %d\n", str, modid);
				return;
			}

			// Append an MQTT topic path to the topic from the reply
			char *mqtt_topic = (char *)malloc(strlen(MQTT_PUBLISH_TO) + strlen(addr) + 1 + strlen(topic));

			strcpy(mqtt_topic, MQTT_PUBLISH_TO);
			strcat(mqtt_topic, addr);
			strcat(mqtt_topic, "/");
			strcat(mqtt_topic, topic);

			printf("[mqtt] Publishing to the topic %s the message \"%s\" | RSSI: %d\n", mqtt_topic, msg, rssi);

			mosquitto_publish(mosq, &mid, mqtt_topic, strlen(msg), msg, 1, false);

			free(mqtt_topic);	

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e != NULL) {
				/* Send pending frame on class A device app. data */
				if (e->nodeclass != LS_ED_CLASS_A || e->num_pending == 0)
					break;

				/*
				 *	Allow to send app. data
				 */
				pthread_mutex_lock(&mutex_pending);
				e->can_send = true;
				pthread_mutex_unlock(&mutex_pending);		
			}
		}
		break;

		case REPLY_JOIN: {
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				printf("[error] Unable to parse join reply: %s", str - 16);
				return;
			}

			unsigned short nodeclass = atoi(str);

			printf("[join] Joined device with id = 0x%08X%08X and class = %c\n", 
				(unsigned int) (nodeid >> 32), 
				(unsigned int) (nodeid & 0xFFFFFFFF), get_node_class(nodeclass));

			add_device(nodeid, nodeclass, true);

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e != NULL) {
				/* If device is rejoined, check the pending messages */
				if (e->num_pending) {
					/* Notify gate about pending messages */
					pthread_mutex_lock(&mutex_uart);
					dprintf(uart, "%c%08X%08X%02x\r", CMD_HAS_PENDING, 
						(unsigned int) (nodeid >> 32), (unsigned int) (nodeid & 0xFFFFFFFF), e->num_pending);
					pthread_mutex_unlock(&mutex_uart);	
				}				
			}
		}
		break;

		case REPLY_KICK: {
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				printf("[error] Unable to parse kick packet: %s", str - 16);
				return;
			}

			pending_item_t *e = pending_to_nodeid(nodeid);
			if (e == NULL)
				break;

			if (kick_device(nodeid)) {
				printf("[kick] Device with id = 0x%08X%08X kicked due to long silence\n", 
					(unsigned int) (nodeid >> 32), 
					(unsigned int) (nodeid & 0xFFFFFFFF));
			}
		}
		break;

		case REPLY_ACK: {
			char addr[17] = {};
			memcpy(addr, str, 16);
			str += 16;

			uint64_t nodeid;
			if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
				printf("[error] Unable to parse ack: %s", str - 16);
				return;
			}

			printf("[ack] ACK received from %08X%08X\n", 
				(unsigned int) (nodeid >> 32), 
				(unsigned int) (nodeid & 0xFFFFFFFF));

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

		case REPLY_PONG: {
			pthread_mutex_lock(&mutex_pong);
			pings_skipped = 0;
			pthread_mutex_unlock(&mutex_pong);
		}

		break;
	}
}

static void invite_mote(uint64_t addr) 
{
	printf("[inv] Sending invitation to node with address 0x%08X%08X\n", (unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF));

	pthread_mutex_lock(&mutex_uart);
	dprintf(uart, "%c%08X%08X\r", CMD_INVITE, (unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF));
	pthread_mutex_unlock(&mutex_uart);
}

static void* pending_worker(void *arg) {
	(void) arg;

	while (1) {
		pthread_mutex_lock(&mutex_pending);

		int i;	
		for (i = 0; i < MAX_NODES; i++) {
			if (pending_free[i])
				continue;

			pending_item_t *e = &pending[i];
			time_t current = time(NULL);

			if (is_fifo_empty(&e->pending_fifo))
				continue;

			/* Messages for "nodeclass A" devices will be sended only on demand */
			if (e->nodeclass == LS_ED_CLASS_A && !e->can_send)
				continue;

			if (current - e->last_msg > RETRY_TIMEOUT_S) {
				if (e->num_retries >= NUM_RETRIES) {
					printf("[fail] Unable to send message to 0x%08X%08X after %u attempts, giving up\n", 
						(unsigned int) (e->nodeid >> 32), (unsigned int) (e->nodeid & 0xFFFFFFFF), NUM_RETRIES);
					e->num_retries = 0;
					m_dequeue(&e->pending_fifo, NULL);

					continue;
				}

				/* Must wait for device to join before sending messages */
				if (e->nodeclass == LS_ED_CLASS_C && e->has_been_invited) {
					/* Retry invitation */
					invite_mote(e->nodeid);
					
					e->num_retries++;
					e->last_msg = current;		
					continue;
				}

				char buf[REPLY_LEN] = {};
				if (!m_peek(&e->pending_fifo, buf)) /* Peek message from queue but don't remove. Will be removed on acknowledge */
					continue;

				printf("[pending] Sending message to 0x%08X%08X: %s\n", (unsigned int) (e->nodeid >> 32), (unsigned int) (e->nodeid & 0xFFFFFFFF), buf);

				e->num_retries++;

				/* Send */
				pthread_mutex_lock(&mutex_uart);
				dprintf(uart, "%s\r", buf);
				pthread_mutex_unlock(&mutex_uart);

				/* Reset invitaion flag. If device doesn't respond on message sended, a new invitation attempt will occur */
				if (e->nodeclass == LS_ED_CLASS_C)
					e->has_been_invited = true;

				e->can_send = false;
				e->last_msg = current;			
			}
		}

		pthread_mutex_unlock(&mutex_pending);

		//usleep(1e3);
	}

	return 0;
}

/* Polls publish queue and publishes the messages into MQTT */
static void *publisher(void *arg)
{
	while(1) {
		pthread_mutex_lock(&mutex_queue);

		/* Checks wether queue is empty */
		if (is_fifo_empty(&inputq)) {
			pthread_mutex_unlock(&mutex_queue);		
			continue;
		}

		/* Pick an element from the head */
		char buf[REPLY_LEN];
		if (!m_dequeue(&inputq, buf)) {
			pthread_mutex_unlock(&mutex_queue);		
			continue;
		}

		pthread_mutex_unlock(&mutex_queue);
		serve_reply(buf);

		usleep(1e3 * QUEUE_POLLING_INTERVAL);
	}	
	
	return NULL;
}

/* Periodic read data from UART */
static void *uart_reader(void *arg)
{
	puts("[gate] UART reading thread created");

	while(1) {
		char buf[REPLY_LEN] = { '\0', };
		char c;
		int r = 0, i = 0;

		pthread_mutex_lock(&mutex_pong);
		if (pings_skipped++ >= MIN_PINGS_SKIPPED) {
			puts("[!!!] No response from LoRa gate! Check the connection!");
		}
		pthread_mutex_unlock(&mutex_pong);

		pthread_mutex_lock(&mutex_uart);

		dprintf(uart, "%c\r", CMD_PING);
		dprintf(uart, "%c\r", CMD_FLUSH);

		while ((r = read(uart, &c, 1)) != 0) {
			buf[i++] = c;
		}

		pthread_mutex_unlock(&mutex_uart);

		buf[i] = '\0';

		if (strlen(buf) > 0) {
			pthread_mutex_lock(&mutex_queue);
			
			char *running = strdup(buf), *token;
			const char *delims = "\n";

			while (strlen(token = strsep(&running, delims))) {
				char buf[REPLY_LEN] = {};
				memcpy(buf, token, strlen(token));

				/* Insert reply to the queue */
				if (!m_enqueue(&inputq, buf)) {
					if (!running) {
						free(running);
					}
					break;
				}

				if (running == NULL)
					break;
				
			}
			
			pthread_mutex_unlock(&mutex_queue);
		}

		usleep(1e3 * UART_POLLING_INTERVAL);
		
		/* Request devices list on demand */
		if (devlist_needed) {
			puts("[!] Device list requested");
			devlist_needed = false; /* No more devices lists needed */
			devices_list(true);

			usleep(1e3 * 150);
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
	printf("[gate] Sending individual message with to the mote with address \"%08X%08X\": \"%s\"\n", 
					(unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF), 
					payload);	

	pending_item_t *e = pending_to_nodeid(addr);
	if (e == NULL) {
		printf("[error] Mote with id = %08X%08X is not in network, an invite will be sended\n", (unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF));
		add_device(addr, LS_ED_CLASS_C, false);
		e = pending_to_nodeid(addr);
	}

	if (e == NULL) {
		puts("[error] Unable to add new device. Is devices list overflowed?\n");	
		return;
	}

	/* Enqueue the frame */
	char buf[REPLY_LEN] = {};
	sprintf(buf, "%c%08X%08X%s", CMD_IND, (unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF), payload);

	pthread_mutex_lock(&mutex_pending);
	if (!m_enqueue(&e->pending_fifo, buf)) {
		printf("[error] Out of memory when adding message to downlink queue for mote with id %08X%08X!\n", (unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF));
		pthread_mutex_unlock(&mutex_pending);
		return;
	}

	if (e->nodeclass == LS_ED_CLASS_A) {
		puts("[pending] Message is delayed");

		e->num_pending++;
		
		/* Notify gate about pending messages */
		pthread_mutex_lock(&mutex_uart);
		dprintf(uart, "%c%08X%08X%02x\r", CMD_HAS_PENDING, (unsigned int) (addr >> 32), (unsigned int) (addr & 0xFFFFFFFF), e->num_pending);
		pthread_mutex_unlock(&mutex_uart);		
	}

	pthread_mutex_unlock(&mutex_pending);
}

static void my_message_callback(struct mosquitto *m, void *userdata, const struct mosquitto_message *message)
{
	if (message->mid != 0) /* To not react on messages published by gate itself */
		return;

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

	printf("Topic count: %d\n", topic_count);

	if (topic_count < 2) {
		return;
	}

	if (memcmp(topics[0], "devices", 7) != 0) {
		puts("[mqtt] Got message not from devices topic");	
		return;
	}

	if (memcmp(topics[1], "lora", 4) != 0) {
		puts("[mqtt] Got message not from devices topic");	
		return;	
	}

	if (topic_count == 3 && memcmp(topics[2], "get", 3) == 0) {
		puts("[mqtt] Devices list requested");
		devices_list(false);
	}

	if (topic_count > 3) {
		// Convert address
		char *addr = topics[2];
		uint64_t nodeid = 0;
		if (!hex_to_bytes(addr, (uint8_t *) &nodeid, !is_big_endian())) {
			printf("[error] Invalid node address: %s\n", addr);
			return;
		}

		char *type = topics[3];
		
		char buf[REPLY_LEN] = {};
		if (!convert_from(type, (char *)message->payload, buf)) {
			printf("[error] Unable to parse mqtt message: devices/lora/%s : %s\n", addr, type);
			return;
		}

		message_to_mote(nodeid, buf);
	}
}

static void my_connect_callback(struct mosquitto *m, void *userdata, int result)
{
//	int i;
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		printf("Subscribing to %s\n", MQTT_SUBSCRIBE_TO);

		mosquitto_subscribe(mosq, NULL, MQTT_SUBSCRIBE_TO, 2);
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}

static void my_subscribe_callback(struct mosquitto *m, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void usage(void) {
	printf("Usage: mqtt <serial>\nExample: mqtt [-ihdp] -p /dev/ttyS0\n");
	printf("  -i\tIgnore /etc/lora-mqtt/mqtt.conf.\n");
	printf("  -h\tPrint this help.\n");
	printf("  -d\tFork to background.\n");
//	printf("  -r\tRetain last MQTT message.\n");
	printf("  -p <port>\tserial port device URI, e.g. /dev/ttyATH0.\n");
}

int main(int argc, char *argv[])
{
//	int i;
	const char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
//	bool clean_session = true;

	printf("=== MQTT-LoRa gate (version: %s) ===\n", VERSION);
	
	bool daemonize = 0;
//	bool retain = 0;
	char serialport[100];
	bool ignoreconfig = 0;
	
	int c;
	while ((c = getopt (argc, argv, "ihdrp:")) != -1)
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
//		case 'r':
//			retain = 1;
//			break;
		case 'p':
			if (strlen(optarg) > 100) {
				printf("Error: serial device URI is too long\n");
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
    int pidfile;
    if (daemonize)
    {
		printf("Attempting to run in the background\n");
		
        if (daemon(0, 0))
        {
            printf("Error forking to background\n");
            exit(EXIT_FAILURE);
        }
        
        char pidval[10];
        pidfile = open("/var/run/mqtt-lora.pid", O_CREAT | O_RDWR, 0666);
        if (lockf(pidfile, F_TLOCK, 0) == -1)
        {
            exit(EXIT_FAILURE);
        }
        sprintf(pidval, "%d\n", getpid());
        write(pidfile, pidval, strlen(pidval));
    }
	
	/* wait for 60 seconds */
	/*
	if (daemonize) {
		usleep(60*1e6);
	}
	*/
	
	FILE* config = NULL;
    char line[255] ;
    char* token;
	
	if (!ignoreconfig)
        {
            config = fopen( "/etc/lora-mqtt/mqtt.conf", "r" );
            if (config)
            {
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
                    }
                }
                fclose(config);
            }
			else
			{
				printf("Configuration file /etc/lora-mqtt/mqtt.conf not found\n");
				return -1;
			}
        }

	printf("Using serial port device: %s\n", serialport);
	
	pthread_mutex_init(&mutex_uart, NULL);
	pthread_mutex_init(&mutex_queue, NULL);
	pthread_mutex_init(&mutex_pending, NULL);
	pthread_mutex_init(&mutex_pong, NULL);

	/* Request a devices list on a first launch */
	devlist_needed = true;

	init_pending();

	TAILQ_INIT(&inputq);

	uart = open(serialport, O_RDWR | O_NOCTTY | O_SYNC);
	if (uart < 0)
	{
		fprintf(stderr, "error %d opening %s: %s\n", errno, serialport, strerror (errno));
		usage();
		return 1;
	}
	
	set_interface_attribs(uart, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking(uart, 0);                	 // set no blocking

	if(pthread_create(&reader_thread, NULL, uart_reader, NULL)) {
		fprintf(stderr, "Error creating reader thread\n");
		return 1;
	}

	if(pthread_create(&publisher_thread, NULL, publisher, NULL)) {
		fprintf(stderr, "Error creating publisher thread\n");
		return 1;
	}

	if (pthread_create(&pending_thread, NULL, pending_worker, NULL)) {
		fprintf(stderr, "Error creating pending queue worker thread");
		return 1;
	}

	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

	if(mosquitto_connect(mosq, host, port, keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}

	puts("[mqtt] Entering event loop");

	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	if (pidfile)
    {
        lockf(pidfile, F_ULOCK, 0);
        close(pidfile);
        remove("/var/run/mqtt-lora.pid");
    }
	
	return 0;
}
