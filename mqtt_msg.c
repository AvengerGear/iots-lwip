/*
 * mqtt_msg.c
 *
 * (C) 2014 AvengerGear
 *
 * MQTT message builder.
 */

#include "mqtt.h"
#include "mqtt_msg.h"
#include <stdint.h>
#include <string.h>

void mqtt_msg_init (struct mqtt_msg *msg) {
	msg->msg_len = 0;
	msg->rem_len = 0;
	msg->fixed_header = 0x00;
}

void mqtt_msg_add_fixed_header (struct mqtt_msg *msg, uint8_t data) {
	msg->fixed_header = data;
}

err_t mqtt_msg_add_byte (struct mqtt_msg *msg, uint8_t data) {
	if (msg->msg_len >= MQTT_MSG_MAX_LENGTH) {
		return ERR_MEM;
	}

	struct mqtt_msg_item *item = &msg->items[msg->msg_len++];
	item->type = MQTT_MSG_TYPE_BYTE;
	item->byte.data = data;
	msg->rem_len += 1;
	return 0;
}

err_t mqtt_msg_add_short (struct mqtt_msg *msg, uint16_t data) {
	if (msg->msg_len >= MQTT_MSG_MAX_LENGTH) {
		return ERR_MEM;
	}

	struct mqtt_msg_item *item = &msg->items[msg->msg_len++];
	item->type = MQTT_MSG_TYPE_SHORT;
	item->_short.data = data;
	msg->rem_len += 2;
	return 0;
}

err_t mqtt_msg_add_string (struct mqtt_msg *msg, uint8_t * str, int32_t len) {
	if (msg->msg_len >= MQTT_MSG_MAX_LENGTH) {
		return ERR_MEM;
	}

	if (len < 0) {
		len = strlen(str);
	}

	struct mqtt_msg_item *item = &msg->items[msg->msg_len++];

	item->type = MQTT_MSG_TYPE_STRING;
	item->data.len = len;
	item->data.data = str;
	msg->rem_len += len + sizeof(uint16_t);
}

err_t mqtt_msg_add_data (struct mqtt_msg *msg, uint8_t * data, int32_t len) {
	if (msg->msg_len >= MQTT_MSG_MAX_LENGTH) {
		return ERR_MEM;
	}

	struct mqtt_msg_item *item = &msg->items[msg->msg_len++];

	item->type = MQTT_MSG_TYPE_STRING;
	item->data.len = len;
	item->data.data = data;
	msg->rem_len += len;
}

err_t mqtt_msg_send (mqtt_msg *msg, struct tcp_pcb *pcb) {
	uint32_t rem_len = msg->rem_len & 0x0fffffff;
	uint8_t rem_len_byte;
	int count = 0;
	int i;
	err_t err;

	/* fixed header */
	err |= tcp_write(pcb, &msg->fixed_header, sizeof(msg->fixed_header),
			TCP_WRITE_FLAG_COPY);

	/* remaining length */
	do {
		rem_len_byte = (uint8_t) (rem_len & 0x0000007f)
			| ((0xffffff80 & rem_len) ? 0x80 : 0x00);
		err |= tcp_write(pcb, &rem_len_byte, sizeof(rem_len_byte),
				TCP_WRITE_FLAG_COPY);
		rem_len = rem_len >> 7;
	} while (rem_len > 0);

	/* variable header and payload */
	for (i=0; i<msg->msg_len; i++){
		struct mqtt_msg_item *item = &msg->items[i];
		uint16_t a;

		switch(item->type) {
			case MQTT_MSG_TYPE_BYTE:
			err |= tcp_write(pcb, &item->byte.data,
					sizeof(item->byte.data),
					TCP_WRITE_FLAG_COPY);
			break;

			case MQTT_MSG_TYPE_SHORT:
			a = htons(item->_short.data);
			err |= tcp_write(pcb, &a, sizeof(a),
					TCP_WRITE_FLAG_COPY);
			break;

			case MQTT_MSG_TYPE_STRING:
			a = htons(item->data.len);
			err |= tcp_write(pcb, &a, sizeof(a),
					TCP_WRITE_FLAG_COPY);
			err |= tcp_write(pcb, item->data.data, item->data.len,
					0);
			break;

			case MQTT_MSG_TYPE_DATA:
			err |= tcp_write(pcb, item->data.data, item->data.len,
					0);
			break;
		}
	}

	return err;
}
