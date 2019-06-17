#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uip.h"
#include "net/mac/tsch/tsch.h"
#include "net/link-stats.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		  (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
uip_ipaddr_t dest_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
void
collect_common_send(void)
{
  static uint16_t seqno;

  struct collect_data_msg {
  uint16_t parent;
  uint16_t parent_etx;
  uint16_t current_rtmetric;
  uint16_t num_neighbors;
  uint16_t parent_rssi;
  uint16_t temp_value;
  uint16_t ext_tempature_value;  // =real value*100
  uint16_t int_tempature_value;  // =real value*1000
  uint16_t battery;
};

  struct {
    uint16_t seqno;
    struct collect_data_msg msg;
  } msg;
  /* struct collect_neighbor *n; */
  uint16_t parent_etx;
  uint16_t rtmetric;
  uint16_t num_neighbors;
  // int16_t battery;
  int16_t parent_rssi =0;

  rpl_parent_t *preferred_parent;
  linkaddr_t parent;
  rpl_dag_t *dag;
  // static uint16_t count=0;
  // char string[20];
  // int int_t;
  // int ext_t, ext_t_s7s_test;


  memset(&msg, 0, sizeof(msg));
  seqno++;
  if(seqno >=65535) {
    /* Wrap to 128 to identify restarts */
    seqno = 1;
  }
  msg.seqno = seqno;

  linkaddr_copy(&parent, &linkaddr_null);
  parent_etx = 0;

  /* Let's suppose we have only one instance */
  dag = rpl_get_any_dag();
  if(dag != NULL) {
    preferred_parent = dag->preferred_parent;
    if(preferred_parent != NULL) {
      uip_ds6_nbr_t *nbr;
      nbr = uip_ds6_nbr_lookup(rpl_parent_get_ipaddr(preferred_parent));
      if(nbr != NULL) {
         //Use parts of the IPv6 address as the parent address, in reversed byte order. 
        parent.u8[LINKADDR_SIZE - 1] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 2];
        parent.u8[LINKADDR_SIZE - 2] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1];
        parent_etx = rpl_neighbor_get_from_ipaddr(rpl_parent_get_ipaddr(preferred_parent))->rank;
        const struct link_stats *stats= rpl_neighbor_get_link_stats(preferred_parent);
        parent_rssi=stats->rssi;

        //parent_rssi = rpl_get_parent_link_stats(preferred_parent)->rssi;
      }
    }
    rtmetric = dag->rank;
    num_neighbors = uip_ds6_nbr_num();
  } else {
    rtmetric = 0;
    num_neighbors = 0;
  }
  memcpy(&msg.msg.parent, &parent.u8[LINKADDR_SIZE - 2], 2);

  msg.msg.parent_etx = parent_etx;
  msg.msg.current_rtmetric = rtmetric;
  msg.msg.num_neighbors = num_neighbors;
  msg.msg.parent_rssi = (uint16_t)parent_rssi;
  msg.msg.battery = 20;
  msg.msg.ext_tempature_value = 20;
  msg.msg.int_tempature_value = 20;


  LOG_INFO("parent'%x' \n", msg.msg.parent);
  LOG_INFO("parent_etx'%u' \n", msg.msg.parent_etx);
  LOG_INFO("current_rtmetric'%u' \n", msg.msg.current_rtmetric);
  LOG_INFO("num_neighbors'%u' \n", msg.msg.num_neighbors);
  // LOG_INFO("battery'%d' \n", battery);
  LOG_INFO("parent_rssi'%d' \n", parent_rssi);

  simple_udp_sendto(&udp_conn, &msg, sizeof(msg), &dest_ipaddr);
}
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  // static char str[32];
  // uip_ipaddr_t dest_ipaddr;
  int is_coordinator;

  PROCESS_BEGIN();

  is_coordinator = 0;
  if(is_coordinator) {
    NETSTACK_ROUTING.root_start();
  }

  NETSTACK_MAC.on();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

  uip_ip6addr(&dest_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO("packet timer timeout %u , %lu !\n", count, RTIMER_NOW());
    count++;

    if(count >=65534)
      count=1;


//    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
    if(NETSTACK_ROUTING.node_is_reachable()) {
      /* Send to DAG root */
      // LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      // snprintf(str, sizeof(str), "hello %d", count);
      // simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      collect_common_send();
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
