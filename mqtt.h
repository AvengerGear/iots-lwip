/*
 * mqtt.h
 *
 * (C) 2013,2014 Martin Hubáček
 * (C) 2014 AvengerGear
 *
 * Basic MQTT protocol implemented on lwIP library.
 */

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>

#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "netif/loopif.h"


#define MAX_PACKET_SIZE 128

#define KEEPALIVE 5000

typedef void (*mqtt_receive_callback) (mqtt * this, uint8_t * topic,
		uint8_t topicLen, uint8_t * data, uint32_t dataLen);

struct mqtt {
  struct ip_addr addr;
  uint16_t port;
  uint8_t connected;
  uint8_t auto_connect;
  struct tcp_pcb *pcb;
  uint32_t last_activity;
  mqtt_receive_callback callback;
  char *device_id;
  uint8_t poll_abort_counter;
  struct mqtt_msg
};

#define MQTT_FH_TYPE_CONNECT		(1 << 4)
#define MQTT_FH_TYPE_CONACK		(2 << 4)
#define MQTT_FH_TYPE_PUBLISH		(3 << 4)
#define MQTT_FH_TYPE_PUBACK		(4 << 4)
#define MQTT_FH_TYPE_PUBREC		(5 << 4)
#define MQTT_FH_TYPE_PUBREL		(6 << 4)
#define MQTT_FH_TYPE_PUBCOMP		(7 << 4)
#define MQTT_FH_TYPE_SUBSCRIBE		(8 << 4)
#define MQTT_FH_TYPE_SUBACK		(9 << 4)
#define MQTT_FH_TYPE_UNSUBSCRIBE	(10 << 4)
#define MQTT_FH_TYPE_UNSUBACK		(11 << 4)
#define MQTT_FH_TYPE_PINGREQ		(12 << 4)
#define MQTT_FH_TYPE_PINGRESP		(13 << 4)
#define MQTT_FH_TYPE_DISCONNECT		(14 << 4)
#define MQTT_FH_DUP			(1 << 3)
#define MQTT_FH_QOS_0			(0 << 1)
#define MQTT_FH_QOS_1			(1 << 1)
#define MQTT_FH_QOS_2			(2 << 1)
#define MQTT_FH_RETAIN			(1 << 0)

/* connect flag */
#define MQTT_CONNECT_USERNAME		(1 << 7)
#define MQTT_CONNECT_PASSWORD		(1 << 6)
#define MQTT_CONNECT_WILL_RETAIN	(1 << 5)
#define MQTT_CONNECT_WILL_QOS_0		(0 << 3)
#define MQTT_CONNECT_WILL_QOS_1		(1 << 3)
#define MQTT_CONNECT_WILL_QOS_2		(2 << 3)
#define MQTT_CONNECT_WILL		(1 << 2)
#define MQTT_CONNECT_CLEAN_SESSION	(1 << 1)

/* subscribe flag */
#define MQTT_SUBSCRIBE_QOS_0		(0 << 3)
#define MQTT_SUBSCRIBE_QOS_1		(1 << 3)
#define MQTT_SUBSCRIBE_QOS_2		(2 << 3)

void mqtt_init (struct mqtt * this, struct ip_addr serverIp, int port,
		mqtt_receive_callback fn, char *devId);
uint8_t mqtt_connect (struct mqtt * this);
uint8_t mqttPublish (Mqtt * this, char *pub_topic, char *msg);
uint8_t mqttDisconnect (Mqtt * this);
uint8_t mqttSubscribe (Mqtt * this, char *topic);
uint8_t mqttLive (Mqtt * this);
void mqttDisconnectForced (Mqtt * this);

#endif /* MQTT_H_ */
