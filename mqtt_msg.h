#include "mqtt.h"
#include <stdint.h>
#ifndef MQTT_MSG_H_
#define MQTT_MSG_H_

#ifndef MQTT_MSG_MAX_LENGTH
#define MQTT_MSG_MAX_LENGTH 32
#endif

#define MQTT_MSG_TYPE_BYTE	0
#define MQTT_MSG_TYPE_SHORT	1
#define MQTT_MSG_TYPE_DATA	2
#define MQTT_MSG_TYPE_STRING	3

struct mqtt_msg_byte {
	uint8_t data;
};

struct mqtt_msg_short {
	uint16_t data;
};

struct mqtt_msg_data {
	uint16_t len;
	uint8_t *data;
};

struct mqtt_msg_item {
	uint16_t type; /* alignment */
	union {
		struct mqtt_msg_byte byte;
		struct mqtt_msg_short _short;
		struct mqtt_msg_data data;
	};
};

struct mqtt_msg {
  uint8_t msg_len;
  uint8_t fixed_header;
  uint32_t rem_len;
  struct mqtt_msg_item items[MQTT_MSG_MAX_LENGTH];
};

void mqtt_msg_init (struct mqtt_msg *msg);
void mqtt_msg_add_fixed_header (struct mqtt_msg *msg, uint8_t data);
int mqtt_msg_add_byte (struct mqtt_msg *msg, uint8_t data);
int mqtt_msg_add_short (struct mqtt_msg *msg, uint16_t data);
int mqtt_msg_add_string (struct mqtt_msg *msg, uint8_t * str, int32_t len);

#ifdef MQTT_MSG_DEBUG
void mqtt_msg_print (struct mqtt_msg *msg);
#endif

#endif
