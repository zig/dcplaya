
#include <kos.h>

#include "lwip/lwip.h"

#include "dcplaya/config.h"
#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "net_driver.h"

#include "commands.h"
#include "packet.h"

unsigned char broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

uint8 eth_mac[6];

uint32 our_ip;


unsigned int tool_ip;
unsigned char tool_mac[6];
unsigned short tool_port;

spinlock_t net_spinlock;

void net_lock()
{
  spinlock_lock(&net_spinlock);
}

void net_unlock()
{
  spinlock_unlock(&net_spinlock);
}


spinlock_t tx_mutex;

void tx_lock()
{
  //spinlock_lock(&tx_mutex);
}

void tx_unlock()
{
  //spinlock_unlock(&tx_mutex);
}



unsigned short checksum(unsigned short *buf, int count)
{
    unsigned long sum = 0;

    while (count--) {
	sum += *buf++;
	if (sum & 0xffff0000) {
	    sum &= 0xffff;
	    sum++;
	}
    }
     return ~(sum & 0xffff);
}

void make_ether(char *dest, char *src, ether_header_t *ether)
{
    memcpy(ether->dest, dest, 6);
    memcpy(ether->src, src, 6);
    ether->type[0] = 8;
    ether->type[1] = 0;
}

void make_ip(int dest, int src, int length, char protocol, ip_header_t *ip)
{
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->length = htons(20 + length);
    ip->packet_id = 0;
    ip->flags_frag_offset = htons(0x4000);
    ip->ttl = 0x40;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src = htonl(src);
    ip->dest = htonl(dest);

    ip->checksum = checksum((unsigned short *)ip, sizeof(ip_header_t)/2);
}

unsigned char crap[1514];

void make_udp(unsigned short dest, unsigned short src, unsigned char * data, int length, ip_header_t *ip, udp_header_t *udp)
{
  ip_udp_pseudo_header_t *pseudo = (ip_udp_pseudo_header_t *)crap;

  udp->src = htons(src);
  udp->dest = htons(dest);
  udp->length = htons(length + 8);
  udp->checksum = 0;
  memcpy(udp->data, data, length);

  pseudo->src_ip = ip->src;
  pseudo->dest_ip = ip->dest;
  pseudo->zero = 0;
  pseudo->protocol = ip->protocol;
  pseudo->udp_length = udp->length;
  pseudo->src_port = udp->src;
  pseudo->dest_port = udp->dest;
  pseudo->length = udp->length;
  pseudo->checksum = 0;
  memset(pseudo->data, 0, length + (length%2));
  memcpy(pseudo->data, udp->data, length);

  udp->checksum = checksum((unsigned short *)pseudo, (sizeof(ip_udp_pseudo_header_t) + length)/2);
  if (udp->checksum == 0)
      udp->checksum = 0xffff;
}



int process_broadcast(unsigned char *pkt, int len)
{
    ether_header_t *ether_header = (ether_header_t *)pkt;
    arp_header_t *arp_header = (arp_header_t *)(pkt + ETHER_H_LEN);
    unsigned char tmp[10];
    unsigned int ip = htonl(our_ip);

    if (ether_header->type[1] != 0x06) /* ARP */
      {
	return -1;
      }

    /* hardware address space = ethernet */
    if (arp_header->hw_addr_space != 0x0100)
	return -1;

    /* protocol address space = IP */
    if (arp_header->proto_addr_space != 0x0008)
	return -1;

    if (arp_header->opcode == 0x0100) { /* arp request */
	if (our_ip == 0) /* return if we don't know our ip */
	    return -1;

	int i;
/* 	printf("ARP request : "); */
/* 	for (i=0; i<4; i++) */
/* 	  printf("%d ", arp_header->proto_target[i]); */
/* 	printf("\n"); */

	if (!memcmp(arp_header->proto_target, &ip, 4)) { /* for us */
	  
/* 	  printf("ARP our IP : %x\n", ip); */

	    /* put src hw address into dest hw address */
	    memcpy(ether_header->dest, ether_header->src, 6);
	    /* put our hw address into src hw address */
	    memcpy(ether_header->src, eth_mac, 6);
	    arp_header->opcode = 0x0200; /* arp reply */
	    /* swap sender and target addresses */
	    memcpy(tmp, arp_header->hw_sender, 10);
	    memcpy(arp_header->hw_sender, arp_header->hw_target, 10);
	    memcpy(arp_header->hw_target, tmp, 10);
	    /* put our hw address into sender hw address */
	    memcpy(arp_header->hw_sender, eth_mac, 6);
	    /* transmit */
	    net_tx(pkt, ETHER_H_LEN + ARP_H_LEN, 1);
	}
    }

    return 0;
}

static unsigned char pkt_buf[1514];

void process_icmp(ether_header_t *ether, ip_header_t *ip, icmp_header_t *icmp)
{
  unsigned int i;
  unsigned char tmp[6];

  memset(pkt_buf, 0, ntohs(ip->length) + (ntohs(ip->length)%2) - 4*(ip->version_ihl & 0x0f));

  /* check icmp checksum */
  i = icmp->checksum;
  icmp->checksum = 0;
  memcpy(pkt_buf, icmp, ntohs(ip->length) - 4*(ip->version_ihl & 0x0f));
  icmp->checksum = checksum((unsigned short *)pkt_buf, (ntohs(ip->length)+1)/2 - 2*(ip->version_ihl & 0x0f));
  if (i != icmp->checksum)
    return;
  
  if (icmp->type == 8) { /* echo request */
    icmp->type = 0; /* echo reply */
    /* swap src and dest hw addresses */
    memcpy(tmp, ether->dest, 6);
    memcpy(ether->dest, ether->src, 6);
    memcpy(ether->src, tmp, 6);
    /* swap src and dest ip addresses */
    memcpy(&i, &ip->src, 4);
    memcpy(&ip->src, &ip->dest, 4);
    memcpy(&ip->dest, &i, 4);
    /* recompute ip header checksum */
    ip->checksum = 0;
    ip->checksum = checksum((unsigned short *)ip, 2*(ip->version_ihl & 0x0f));
    /* recompute icmp checksum */
    icmp->checksum = 0;
    icmp->checksum = checksum((unsigned short *)icmp, ntohs(ip->length)/2 - 2*(ip->version_ihl & 0x0f));
    /* transmit */
    net_tx(ether, ETHER_H_LEN + ntohs(ip->length), 1);
  }
}

void cmd_retval(ip_header_t * ip, udp_header_t * udp, command_t * command);

int process_udp(ether_header_t *ether, ip_header_t *ip, udp_header_t *udp)
{
  ip_udp_pseudo_header_t *pseudo;
  unsigned short i;
  command_t *command;

  if (tool_ip && (tool_port != ntohs(udp->src) || tool_ip != ntohl(ip->src)))
    return -1;

  pseudo = (ip_udp_pseudo_header_t *)pkt_buf;
  pseudo->src_ip = ip->src;
  pseudo->dest_ip = ip->dest;
  pseudo->zero = 0;
  pseudo->protocol = ip->protocol;
  pseudo->udp_length = udp->length;
  pseudo->src_port = udp->src;
  pseudo->dest_port = udp->dest;
  pseudo->length = udp->length;
  pseudo->checksum = 0;
  memset(pseudo->data, 0, ntohs(udp->length) - 8 + (ntohs(udp->length)%2));
  memcpy(pseudo->data, udp->data, ntohs(udp->length) - 8);

  /* checksum == 0 means no checksum */
  if (udp->checksum != 0)
      i = checksum((unsigned short *)pseudo, (sizeof(ip_udp_pseudo_header_t) + ntohs(udp->length) - 9 + 1)/2);
  else
      i = 0;
  /* checksum == 0xffff means checksum was really 0 */
  if (udp->checksum == 0xffff)
      udp->checksum = 0;

  if (i != udp->checksum) {
/*    scif_puts("UDP CHECKSUM BAD\n"); */
    return -1;
  }

  if (!tool_ip) {
    printf("Set dc-tool IP to 0x%x, port %d\n", ntohl(ip->src), ntohs(udp->src));
    tool_ip = ntohl(ip->src);
    tool_port = ntohs(udp->src);
    memcpy(tool_mac, ether->src, 6);
  } else {
/*     if (tool_ip != ntohs(ip->src)) */
/*       return -1; */
  }

  make_ether(ether->src, ether->dest, (ether_header_t *)pkt_buf);

  command = (command_t *)udp->data;

/*   printf("Received command '%c%c%c%c'\n",  */
/* 	 command->id[0], */
/* 	 command->id[1], */
/* 	 command->id[2], */
/* 	 command->id[3]); */

  if (!memcmp(command->id, CMD_EXECUTE, 4)) {
      cmd_execute(ether, ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_REBOOT, 4)) {
      cmd_reboot(ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_LOADBIN, 4)) {
      cmd_loadbin(ip, udp, command);
  } else  
  if (!memcmp(command->id, CMD_PARTBIN, 4)) {
      cmd_partbin(ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_DONEBIN, 4)) {
      cmd_donebin(ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_SENDBINQ, 4)) {
      cmd_sendbinq(ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_SENDBIN, 4)) {
      cmd_sendbin(ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_VERSION, 4)) {
      cmd_version(ip, udp, command);
  } else
  if (!memcmp(command->id, CMD_RETVAL, 4)) {
      cmd_retval(ip, udp, command);
  } else {
    tool_ip = 0;
    return -1;
  }

  return 0;
}

int process_mine(unsigned char *pkt, int len)
{
    ether_header_t *ether_header = (ether_header_t *)pkt;
    ip_header_t *ip_header = (ip_header_t *)(pkt + 14);
    icmp_header_t *icmp_header;
    udp_header_t *udp_header;
    ip_udp_pseudo_header_t *ip_udp_pseudo_header;
    unsigned char tmp[6];
    int i;
    
    if (ether_header->type[1] != 0x00)
	return -1;

    /* ignore fragmented packets */

    if (ntohs(ip_header->flags_frag_offset) & 0x3fff)
	return -1;
    
    /* check ip header checksum */
    i = ip_header->checksum;
    ip_header->checksum = 0;
    ip_header->checksum = checksum((unsigned short *)ip_header, 2*(ip_header->version_ihl & 0x0f));
    if (i != ip_header->checksum) {
      ip_header->checksum = i; /* VP : put back the original checksum so that we can forward
				  this packet to lwip */
      return -1;
    }

    switch (ip_header->protocol) {
    case 1: /* icmp */
      if (!lwip_cb) {
	icmp_header = (icmp_header_t *)(pkt + ETHER_H_LEN + 4*(ip_header->version_ihl & 0x0f));
	process_icmp(ether_header, ip_header, icmp_header);
	return 0;
      }
      break;
    case 17: /* udp */
      udp_header = (udp_header_t *)(pkt + ETHER_H_LEN + 4*(ip_header->version_ihl & 0x0f));
      return process_udp(ether_header, ip_header, udp_header);
    }

    return -1;
}

int eth_interrupt;

void rx_callback(netif_t * netif, unsigned char *pkt, int len)
{
  ether_header_t *ether_header = (ether_header_t *)pkt;

  //vid_border_color(255, 255, 0);

  if (ether_header->type[0] == 0x08 &&
      !memcmp(ether_header->dest, eth_mac, 6)) {
    if (!process_mine(pkt, len))
      goto end;
  }

  if (lwip_cb) {
    //printf("lwip cb\n");
    eth_interrupt = 1;
    lwip_cb(netif, pkt, len);
    eth_interrupt = 0;
    goto end;
  }

  if (ether_header->type[0] != 0x08)
    goto end;

  if (ether_header->type[0] == 0x08 &&
      !memcmp(ether_header->dest, broadcast, 6)) {
    if (process_broadcast(pkt, len))
      goto end;
  }
  
 end:
  //vid_border_color(0, 0, 0);
}

void eth_setip(int a, int b, int c, int d)
{
    unsigned char *ip = (unsigned char *)&our_ip;

    ip[0] = a;
    ip[1] = b;
    ip[2] = c;
    ip[3] = d;

    our_ip = ntohl(our_ip);
}

