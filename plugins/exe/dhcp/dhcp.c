/** simple DHCP client implementation */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <kos/fs.h>
#include <kos/sem.h>
#include "dc/ta.h"
#include "syscalls.h"
#include "net.h"
#include "commands.h"
#include "packet.h"
#include "dhcp.h"

extern ether_header_t * ether;
extern ip_header_t * ip;
extern udp_header_t * udp;
extern dhcp_header_t * dhcp;

static int dhcp_xid;
static dhcp_conf_t dhcp_conf;

static char *host_name = "dcplaya";
#define lhost_name (sizeof(host_name) - 1)

static char parameter_request_list[] = { 54, 51, 1, 3, 6, 15 };

static int eth_txts(uint8 *pkt, int len)
{
  int res;
  for( ; ; ) {
    if (!irq_inside_int())
      tx_lock();

    res = net_tx(pkt, len, irq_inside_int());

    if (!irq_inside_int())
      tx_unlock();

    if (!res || irq_inside_int())
      break;
 
    thd_pass();
  }

  return res;
}

static void build_send_packet(int dport, int sport, int command_len)
{
    unsigned char * command = udp->data;

    make_ether(broadcast, eth_mac, ether);
    make_ip(0xffffffff, /*our_ip*/ 0, UDP_H_LEN + command_len, /*UDP*/ 17, ip);
    make_udp(dport, sport, command, command_len, ip, udp);
    eth_txts((uint8 *) ether, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + command_len);

}

unsigned char opts_magic_starter[] = { 99, 130, 83, 99};
void dhcp_discover(unsigned int xid)
{
  unsigned char *opts = dhcp->data;
  printf("Emitting DHCP discovery packet with XID %x\n", xid);

  memset(&dhcp_conf, 0, sizeof(dhcp_conf));
  dhcp_xid = xid;

  memset(dhcp, 0, sizeof(*dhcp));
  dhcp->op     = 1;   // BOOTREQUEST
  dhcp->htype  = 1;   // hw type 10mb ethernet
  dhcp->hlen   = 6;   // 10mb ethernet
  dhcp->hops   = 0;   // zero for client
  dhcp->xid    = xid; // transaction id
  dhcp->secs   = 0;   // seconds elapsed since client began address acquisition or renewal process
  dhcp->flags  = htons(0); // 0x8000 = broadcast
  dhcp->ciaddr = htonl(0); // client ip address
  memcpy(dhcp->chaddr, eth_mac, sizeof(dhcp->chaddr));

  // magic options header
  memcpy(opts, opts_magic_starter, 4);
  opts += 4;

  *opts++ = 53; /* DHCP Message Type */
  *opts++ = 1;
  *opts++ = 1;  /* DHCPDISCOVER */

  *opts++ = 12; // host name
  *opts++ = lhost_name;
  memcpy(opts, host_name, lhost_name);
  opts += lhost_name;

  *opts++ = 61; // client identifier
  *opts++ = 1 + sizeof(eth_mac);
  *opts++ = 1; // hw type 10mb
  memcpy(opts, eth_mac, sizeof(eth_mac));
  opts += sizeof(eth_mac);

  *opts++ = 255;

  build_send_packet(67, 68, sizeof(*dhcp) - 1 + opts - dhcp->data);
}

static void pokel(unsigned char *addr, uint32 val)
{
  val = htonl(val);
  memcpy(addr, &val, 4);
}

void dhcp_request()
{
  unsigned char *opts = dhcp->data;
  printf("Emitting DHCP request packet with XID %x\n", dhcp_xid);

  memset(dhcp, 0, sizeof(*dhcp));
  dhcp->op     = 1;   // BOOTREQUEST
  dhcp->htype  = 1;   // hw type 10mb ethernet
  dhcp->hlen   = 6;   // 10mb ethernet
  dhcp->hops   = 0;   // zero for client
  dhcp->xid    = dhcp_xid; // transaction id
  dhcp->secs   = 0;   // seconds elapsed since client began address acquisition or renewal process
  dhcp->flags  = htons(0); // 0x8000 = broadcast
  dhcp->ciaddr = htonl(0); // client ip address
  memcpy(dhcp->chaddr, eth_mac, sizeof(dhcp->chaddr));

  // magic options header
  memcpy(opts, opts_magic_starter, 4);
  opts += 4;

  *opts++ = 53; /* DHCP Message Type */
  *opts++ = 1;
  *opts++ = 3;  /* DHCPREQUEST */

  *opts++ = 54; // server identifier
  *opts++ = 4;
  pokel(opts, dhcp_conf.server_identifier);
  opts += 4;

  *opts++ = 50; // requested ip address
  *opts++ = 4;
  pokel(opts, dhcp_conf.yourip);
  opts += 4;

  *opts++ = 12; // host name
  *opts++ = lhost_name;
  memcpy(opts, host_name, lhost_name);
  opts += lhost_name;

  *opts++ = 55; // parameter request list
  *opts++ = sizeof(parameter_request_list);
  memcpy(opts, parameter_request_list, sizeof(parameter_request_list));
  opts += sizeof(parameter_request_list);

  *opts++ = 61; // client identifier
  *opts++ = 1 + sizeof(eth_mac);
  *opts++ = 1; // hw type 10mb
  memcpy(opts, eth_mac, sizeof(eth_mac));
  opts += sizeof(eth_mac);

  *opts++ = 255;

  build_send_packet(67, 68, sizeof(*dhcp) - 1 + opts - dhcp->data);
}

int dhcp_handle(unsigned char *pkt)
{
  ether_header_t * lether = (ether_header_t *)pkt;
  ip_header_t * lip = (ip_header_t *)(pkt + ETHER_H_LEN);
  udp_header_t * ludp = (udp_header_t *)(pkt + ETHER_H_LEN + IP_H_LEN);
  dhcp_header_t * ldhcp = (dhcp_header_t *)(pkt + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
  unsigned char *opts = ldhcp->data;
  dhcp_conf_t c;

#if 0
  printf("Received DHCP packet :\n"
	 "srcip:\t%08x\n"
	 "dstip:\t%08x\n"
	 "xid:\t%x\n"
	 "ciaddr:\t%08x\n"
	 "yiaddr:\t%08x\n"
	 "siaddr:\t%08x\n"
	 "giaddr:\t%08x\n"
	 "sname:\t%64s\n"
	 "file:\t%128s\n",
	 lip->src, lip->dest,
	 ldhcp->xid,
	 ldhcp->ciaddr,
	 ldhcp->yiaddr,
	 ldhcp->siaddr,
	 ldhcp->giaddr,
	 ldhcp->sname,
	 ldhcp->file);
#endif

  if (dhcp_xid != ldhcp->xid || memcmp(eth_mac, ldhcp->chaddr, sizeof(eth_mac)))
    return -1;

  memset(&c, 0, sizeof(c));
  c.yourip = ntohl(ldhcp->yiaddr);

  if (!memcmp(opts, opts_magic_starter, 4)) {
    opts += 4;
    while (opts - pkt < 1500 && *opts != 255) {
      int len, type;
      char opt[256];
      int i;

      type = *opts++;
      if (type == 0)
	len = 0;
      else
	len = *opts++;

      //printf("opt %d len %d\n", type, len);

      // copy option over in an aligned buffer
      memcpy(opt, opts, len);

      switch (type) {
      case 53:
	//printf("dhcp type %d\n", *opt);
	switch (*opts) {
	case 2: // DHCPOFFER
	  /* ignore other offers if we're already ACKed */
	  if (dhcp_conf.valid == 5)
	    return 0;
	case 5: // DHCPACK
	    c.valid = *opts;
	  break;
	default:
	  return 0;
	}
	break;

      case 54:
	c.server_identifier = ntohl(*(unsigned int *)opt);
	break;

      case 51:
	c.lease_time = ntohl(*(unsigned int *)opt);
	break;

      case 1:
	c.subnet_mask = ntohl(*(unsigned int *)opt);
	break;

      case 3:
	c.router = ntohl(*(unsigned int *)opt);
	break;

      case 6:
	for (i = 0 ; i < len/4 && i < sizeof(c.dns) / sizeof(c.dns[0]) ; i++)
	  c.dns[i] = ntohl(((unsigned int *)opt)[i]);
	break;

      case 15:
	memcpy(c.domain, opts, len);
	c.domain[len] = 0;
	break;
      }

      opts += len;
    }
  }

  if (!c.valid || (dhcp_conf.valid && c.server_identifier != dhcp_conf.server_identifier))
    return 0;

  memcpy(&dhcp_conf, &c, sizeof(c));

  return 0;
}

int dhcp_get_conf(dhcp_conf_t *conf)
{
  memcpy(conf, &dhcp_conf, sizeof(dhcp_conf));

  return conf->valid? 0 : -1;
}
