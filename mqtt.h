#ifndef MQTT_H
#define MQTT_H

typedef enum {
	CMD_PING = 'P',			/* Command to ping/pong with client */
	CMD_DEVLIST = 'L',		/* Command to get devices list from a gate */
	CMD_IND = 'I',			/* Individual command to the mote by address */
	CMD_HAS_PENDING = '?',	/* Individual device has N pending packets */
	CMD_INVITE = 'V',		/* Individual invite to join network for class C devices */

	CMD_FLUSH = 'F',		/* Command to get all pending info */
} gate_cmd_type_t;

typedef enum {
	REPLY_PONG = '!',	/* Reply for ping command from client */
	REPLY_LIST = 'L',	/* Reply for the device list command */
	REPLY_IND = 'I',	/* Reply from the individual mote */

	REPLY_JOIN = 'J',	/* Node is joined to the network */
	REPLY_KICK = 'K',	/* Node is kicked from the network */

	REPLY_ACK = 'A',	/* Application data acknowledged by the node */
} gate_reply_type_t;

typedef enum {
	LS_ED_CLASS_A = 0,
	LS_ED_CLASS_B = 1,
	LS_ED_CLASS_C = 2,
} ls_node_class_t;

#endif
