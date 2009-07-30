#ifndef __PACKET_H__
#define __PACKET_H__

#include "bswap.h"

#include <kos/net.h>

//extern unsigned char pkt_buf[1514];
extern unsigned char broadcast[6];
extern uint8 eth_mac[6];
extern uint32 our_ip;
extern net_input_func lwip_cb;

extern netif_t * netif;

static int net_tx(const uint8 *data, int len, int mode)
{
  if (netif) {
    int res;
    //vid_border_color(255, 0, 0);
    res =  netif->if_tx(netif, data, len, mode);
    //vid_border_color(0, 0, 0);
    return res;
  } else
    return 0;
}

#define packed __attribute__((packed))

typedef struct {
  unsigned char dest[6] packed;
  unsigned char src[6]  packed;
  unsigned char type[2] packed;
} ether_header_t;

typedef struct {
  unsigned char version_ihl packed;
  unsigned char tos packed;
  unsigned short length packed;
  unsigned short packet_id packed;
  unsigned short flags_frag_offset packed;
  unsigned char ttl packed;
  unsigned char protocol packed;
  unsigned short checksum packed;
  unsigned int src packed;
  unsigned int dest packed;
} ip_header_t;

typedef struct {
  unsigned short src packed;
  unsigned short dest packed;
  unsigned short length packed;
  unsigned short checksum packed;
  unsigned char  data[1] packed;
} udp_header_t;

typedef struct {
  unsigned char type packed;
  unsigned char code packed;
  unsigned short checksum packed;
  unsigned int misc packed;
} icmp_header_t;

typedef struct {
  unsigned short hw_addr_space packed;
  unsigned short proto_addr_space packed;
  unsigned char hw_addr_len packed;
  unsigned char proto_addr_len packed;
  unsigned short opcode packed;
  unsigned char hw_sender[6] packed;
  unsigned char proto_sender[4] packed;
  unsigned char hw_target[6] packed;
  unsigned char proto_target[4] packed;
} arp_header_t;

typedef struct {
  unsigned int src_ip packed;
  unsigned int dest_ip packed;
  unsigned char zero packed;
  unsigned char protocol packed;
  unsigned short udp_length packed;
  unsigned short src_port packed;
  unsigned short dest_port packed;
  unsigned short length packed;
  unsigned short checksum packed;
  unsigned char data[1] packed;
} ip_udp_pseudo_header_t;

typedef struct {
  unsigned char op packed;
  unsigned char htype packed;
  unsigned char hlen packed;
  unsigned char hops packed;
  unsigned int xid packed;
  unsigned short secs packed;
  unsigned short flags packed;
  unsigned int ciaddr packed;
  unsigned int yiaddr packed;
  unsigned int siaddr packed;
  unsigned int giaddr packed;
  unsigned char chaddr[16] packed;
  unsigned char sname[64] packed;
  unsigned char file[128] packed;
  unsigned char data[1] packed;
} dhcp_header_t;

unsigned short checksum(unsigned short *buf, int count);
void make_ether(char *dest, char *src, ether_header_t *ether);
void make_ip(int dest, int src, int length, char protocol, ip_header_t *ip);
void make_udp(unsigned short dest, unsigned short src, unsigned char * data, int length, ip_header_t *ip, udp_header_t *udp);

#define ntohl bswap32
#define htonl bswap32
#define ntohs bswap16
#define htons bswap16

#define ETHER_H_LEN 14
#define IP_H_LEN    20
#define UDP_H_LEN   8
#define ICMP_H_LEN  8
#define ARP_H_LEN   28

/* thread safe version of eth_tx, waiting nicely with thd_pass */
//int eth_txts(uint8 *pkt, int len);

#endif
