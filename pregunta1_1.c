/*
 * Copyright (c) 2012, Thingsquare, www.thingsquare.com.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"

#include "simple-udp.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define UDP_PORT 1234

#define SEND_INTERVAL  (60 * CLOCK_SECOND)  // Intervalo de envío
#define SEND_TIME      (random_rand() % (SEND_INTERVAL))  // Tiempo de envío aleatorio

static struct simple_udp_connection unicast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(sender_node_process, "Sender node process");
AUTOSTART_PROCESSES(&sender_node_process);
/*---------------------------------------------------------------------------*/
struct Sensor {
  char name[15];
  int value;  // Cambiado de float a int
};

int random_value(int min, int max) {
  return min + (rand() % (max - min + 1));
}

struct Sensor read_temperature() {
  struct Sensor temp;
  strncpy(temp.name, "Temperature", sizeof(temp.name) - 1);
  temp.value = random_value(0, 35);
  return temp;
}

struct Sensor read_humidity() {
  struct Sensor humidity;
  strncpy(humidity.name, "Humidity", sizeof(humidity.name) - 1);
  humidity.value = random_value(40, 80);
  return humidity;
}

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Sender received data from ");
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d with length %d: '%s'\n",
         receiver_port, sender_port, datalen, data);
}

/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;
  const uip_ipaddr_t *default_prefix = uip_ds6_default_prefix();

  uip_ip6addr_copy(&ipaddr, default_prefix);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sender_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;
  const uip_ipaddr_t *default_prefix;

  PROCESS_BEGIN();

  set_global_address();

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));

    default_prefix = uip_ds6_default_prefix();
    uip_ip6addr_copy(&addr, default_prefix);

    addr.u16[4] = UIP_HTONS(0x0201);
    addr.u16[5] = UIP_HTONS(0x0001);
    addr.u16[6] = UIP_HTONS(0x0001);
    addr.u16[7] = UIP_HTONS(0x0001);

    {
      struct Sensor temp = read_temperature();
      struct Sensor humidity = read_humidity();
      char buf[50];

      snprintf(buf, sizeof(buf), "Temperatura: %d, Humedad: %d", temp.value, humidity.value);
      printf("Sending unicast to ");
      uip_debug_ipaddr_print(&addr);
      printf(" with message: '%s'\n", buf);
      simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, &addr);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

