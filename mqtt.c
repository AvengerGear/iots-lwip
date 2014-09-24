/*
 * mqtt.c
 *
 * (C) 2013,2014 Martin Hubáček
 * (C) 2014 AvengerGear
 *
 * Basic MQTT protocol implemented on lwIP library.
 */

#include <stdint.h>
#include "mqtt.h"
#include "mqtt_msg.h"

extern volatile uint32_t msTime;

void mqtt_init (struct mqtt *this, struct ip_addr addr, uint16_t port,
		mqtt_receive_callback fn, char *device_id) {
	this->callback = fn;
	this->addr = addr;
	this->port = port;
	this->connected = 0;
	this->device_id = device_id;
	this->auto_connect = 0;
}

/* TODO: write an MQTT message parser for this function */
err_t recv_callback (void *arg, struct tcp_pcb *pcb, struct pbuf *p,
		err_t err) {
	uint8_t *mqttData;
	MQTT *this = arg;

	/* Check if status is ok and data is arrived. */
	if (err == ERR_OK && p != NULL) {
		/* Inform TCP that we have taken the data. */
		tcp_recved (pcb, p->tot_len);
		mqttData = (uint8_t *) (p->payload);
		uint8_t *topic = mqttData + 2 + 2;
		uint16_t topicLen = (mqttData[2] << 8) | mqttData[3];
		uint8_t *data = &mqttData[2 + 2 + topicLen];
		uint32_t dataLen = p->tot_len - (2 + 2 + topicLen);
		switch (mqttData[0] & 0xF0) {
			case MQTT_MSGT_PUBLISH:
				this->msgReceivedCallback (this, topic, topicLen, data, dataLen);
				break;
			case MQTT_MSGT_CONACK:
				this->connected = 1;
				break;
		}
	} else {
		/* No data arrived.  That means the client closes the
		 * connection and sent us a packet with FIN flag set to 1.
		 * We have to cleanup and destroy out TCPConnection. */
		tcp_close (pcb);
	}
	pbuf_free (p);
	return ERR_OK;
}


/* Accept an incoming call on the registered port */
err_t accept_callback (void *arg, struct tcp_pcb * npcb, err_t err)
{
	struct mqtt *this = arg;

	/* Subscribe a receive callback function */
	tcp_recv (npcb, recv_callback);

	return ERR_OK;
}

void mqtt_disconnect_forced (MQTT * this) {
	if (!this->connected)
		return;
	tcp_abort (this->pcb);
	this->connected = 0;
}

static void conn_err (void *arg, err_t err)
{
	LWIP_UNUSED_ARG(err);
}

static err_t mqtt_poll (void *arg, struct tcp_pcb *pcb)
{
	struct mqtt *this = arg;
	if (!this->connected && this->poll_abort_counter == 4) {
		mqtt_disconnect_forced(this);
		return ERR_ABRT;
	}
	this->poll_abort_counter++;
	return ERR_OK;
}

uint8_t mqtt_connect (struct mqtt * this)
{
	struct mqtt_msg msg;
	if (this->connected) {
		return 1;
	}
	this->pollAbortCounter = 0;
	this->pcb = tcp_new ();
	err_t err =
		tcp_connect (this->pcb, &(this->server), this->port, accept_callback);
	if (err != ERR_OK) {
		mqtt_disconnect_forced (this);
		return 1;
	}
	tcp_arg (this->pcb, this);
	tcp_err (this->pcb, conn_err);
	tcp_poll (this->pcb, mqtt_poll, 4);
	tcp_accept (this->pcb, &accept_callback);

	mqtt_msg_init (&msg);
	mqtt_msg_add_fixed_header (&msg, MQTT_FH_TYPE_CONNECT);

	/* variable header: */
	/* protocol name */
	mqtt_msg_add_string (&msg, "MQIsdp", 6);
	/* protocol version */
	mqtt_msg_add_byte (&msg, 3);
	/* connect flag */
	mqtt_msg_add_byte (&msg, MQTT_CONNECT_CLEAN_SESSION);
	/* keep alive timer */
	mqtt_msg_add_short (&msg, KEEPALIVE / 1000);

	/* payload: client identifier */
	mqtt_msg_add_string (&msg, this->device_id, -1);

	err = mqtt_msg_send(&msg, this->pcb);
	if (err == ERR_OK) {
		tcp_output (this->pcb);
		this->last_activity = msTime;
		return 0;
	}
	else
	{
		mqtt_disconnect_forced (this);
		return 1;
	}
	return 0;
}

uint8_t mqtt_publish (struct mqtt * this, char *pub_topic, char *msg)
{
	struct mqtt_msg msg;
	if (!this->connected) {
		return 1;
	}

	mqtt_msg_init(&msg);
	mqtt_msg_add_fixed_header(&msg, MQTT_FH_TYPE_PUBLISH);

	/* variable header: topic name */
	mqtt_msg_add_string(&msg, pub_topic, -1);

	/* payload: message */
	mqtt_msg_add_data(&msg, pub_topic, -1);

	err = mqtt_msg_send(&msg, this->pcb);
	if (err == ERR_OK) {
		tcp_output (this->pcb);
	} else {
		mqtt_disconnect_forced (this);
		return 1;
	}
	return 0;
}

static void close_conn (struct tcp_pcb *pcb) {
	err_t err;
	tcp_arg (pcb, NULL);
	tcp_sent (pcb, NULL);
	tcp_recv (pcb, NULL);
	err = tcp_close (pcb);
}

uint8_t mqtt_disconnect (struct mqtt *this) {
	struct mqtt_msg msg;
	if (!this->connected) {
		return 1;
	}
	mqtt_msg_init(&msg);
	mqtt_msg_add_fixed_header(&msg, MQTT_FH_TYPE_DISCONNECT);

	err = mqtt_msg_send(&msg, this->pcb);
	if (err == ERR_OK) {
		tcp_output (this->pcb);
		tcp_close (this->pcb);
		this->connected = 0;
	} else {
		mqtt_disconnect_forced (this);
		return 1;
	}
	return 0;
}

uint8_t mqtt_subscribe (struct mqtt * this, char *topic) {
	struct mqtt_msg msg;
	if (!this->connected) {
		return -1;
	}

	mqtt_msg_init(&msg);
	mqtt_msg_add_fixed_header(&msg, MQTT_FH_TYPE_SUBSCRIBE);

	/* variable header: message id */
	mqtt_msg_add_short(&msg, 10);

	/* TODO: we can subscirbe multiple topics at once */

	/* payload: */
	/* topic */
	mqtt_msg_add_string(&msg, topic, -1);
	/* topic QoS */
	mqtt_msg_add_byte(&msg, MQTT_SUBSCRIBE_QOS_0);

	err = mqtt_msg_send(&msg, this->pcb);
	if (err == ERR_OK) {
		tcp_output (this->pcb);
	} else {
		mqtt_disconnect_forced (this);
		return 1;
	}
	return 0;
}

int mqtt_ping (struct mqtt *this) {
	struct mqtt_msg msg;
	if (!this->connected) {
		return -1;
	}

	mqtt_msg_init(&msg);
	mqtt_msg_add_fixed_header(&msg, MQTT_FH_TYPE_PINGREQ);

	err = mqtt_msg_send(&msg, this->pcb);
	if (err == ERR_OK) {
		tcp_output (this->pcb);
	} else {
		mqtt_disconnect_forced (this);
		return 1;
	}
	return 0;
}

uint8_t mqtt_keepalive (struct mqtt *this) {
	uint32_t t = msTime;
	if (t - this->last_activity > (KEEPALIVE - 2000)) {
		if (this->connected)
		{
			mqtt_ping (this);
		}
		else if (this->auto_connect)
		{
			mqtt_connect (this);
		}
		this->last_activity = t;
	}
	return 0;
}
