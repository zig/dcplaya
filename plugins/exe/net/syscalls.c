/* VP 
 *  
 * This is quite a huge hack to have dc-load syscalls like available
 * through the net interface
 */

/* 
 * This file is part of the dcload Dreamcast ethernet loader
 *
 * Copyright (C) 2001 Andrew Kieschnick <andrewk@austin.rr.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
//#include <dirent.h>
#include <kos/fs.h>
#include <kos/sem.h>
#include "dc/ta.h"
#include "syscalls.h"
#include "net.h"
#include "commands.h"
#include "packet.h"


#define S_IFDIR 0040000 /* directory */

static int escape_loop;

static int running = 1;

unsigned int syscall_retval;

static unsigned char pkt_buf[1514];

ether_header_t * ether = (ether_header_t *)pkt_buf;
ip_header_t * ip = (ip_header_t *)(pkt_buf + ETHER_H_LEN);
udp_header_t * udp = (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN);


static semaphore_t * result_sema;
kthread_t * waiting_thd;


static int (*real_old_printk_func)(const uint8 *data, int len, int xlat);

/* from console.c */
extern int (*old_printk_func)(const uint8 *data, int len, int xlat);


/* send command, enable bb, bb_loop(), then return */


/* thread safe version of eth_tx */
# define STOPIRQ \
	if (!irq_inside_int()) \
 	  oldirq = irq_disable(); else

# define STARTIRQ \
	if (!irq_inside_int()) \
	  irq_restore(oldirq); else

/* If NICE is defined then sending packet will use less CPU but on the other hand it may 
   take a while before this thread is running again */
//#define NICE

extern int bba_dma_busy;
int eth_txts(uint8 *pkt, int len)
{
  int res;
  int oldirq;
  for( ; ; ) {
    if (!irq_inside_int())
      tx_lock();
#if 0
    STOPIRQ;
    while (bba_dma_busy) {
      STARTIRQ;
      thd_pass();
      STOPIRQ;
    }
#endif

#ifdef NICE
    res = net_tx(pkt, len, irq_inside_int());
#else
    res = net_tx(pkt, len, 1);
#endif

#if 0
    STARTIRQ;
#endif

    if (!irq_inside_int())
      tx_unlock();

    if (!res || irq_inside_int())
      break;
 
    thd_pass();
  }

  return res;
}


static void build_send_packet(int command_len)
{
    unsigned char * command = pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN;
/*
    scif_puts("build_send_packet\n");
    scif_putchar(command[0]);
    scif_putchar(command[1]);
    scif_putchar(command[2]);
    scif_putchar(command[3]);
    scif_puts("\n");
*/

    if (!tool_ip) {
      printf("Calling BBA syscall without dc-load client attached !\n");
      return;
    }

    make_ether(tool_mac, eth_mac, ether);
    make_ip(tool_ip, our_ip, UDP_H_LEN + command_len, 17, ip);
    make_udp(tool_port, 31313, command, command_len, ip, udp);
    //bb->start();
    eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + command_len);

}

static void wait_result()
{
  if (!tool_ip) {
    //printf("Calling BBA syscall without dc-load client attached !\n");
    syscall_retval = -1;
    return;
  }

  int fc = ta_state.frame_counter;
  waiting_thd = thd_current;
  do {
    //thd_pass();
    sem_wait_timed(result_sema, 500);
    if (!escape_loop && ta_state.frame_counter - fc > 60) {
      tool_ip = 0;
      printf("BBA syscall timed out !\n"
	     "Please reconnect the dc-load client.\n");
      syscall_retval = -1;
      waiting_thd = NULL;
      return;
    }
  } while (!escape_loop);
  waiting_thd = NULL;
  escape_loop = 0;
}

void sc_dcexit(void)
{
    command_t * command = (command_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_EXIT, 4); 
    command->address = htonl(0);
    command->size = htonl(0);
    build_send_packet(COMMAND_LEN);
    //bb->stop();
}

int sc_read(int fd, void *buf, size_t count)
{
    command_3int_t * command = (command_3int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_READ, 4); 
    command->value0 = htonl(fd);
    command->value1 = htonl(buf);
    command->value2 = htonl(count);
    build_send_packet(sizeof(command_3int_t));
    wait_result();
    
    return syscall_retval;
}

int sc_write(int fd, const void *buf, size_t count)
{
    command_3int_t * command = (command_3int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_WRITE, 4); 
    command->value0 = htonl(fd);
    command->value1 = htonl(buf);
    command->value2 = htonl(count);
    build_send_packet(sizeof(command_3int_t));
    wait_result();
    
    return syscall_retval;
}

int sc_open(const char *pathname, int flags, ...)
{
    va_list ap;
    command_2int_string_t * command = (command_2int_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);

    int namelen = strlen(pathname);
 
    memcpy(command->id, CMD_OPEN, 4); 

    va_start(ap, flags);
    command->value0 = htonl(flags);
    command->value1 = htonl(va_arg(ap, int));
    va_end(ap);

    memcpy(command->string, pathname, namelen+1);
    
    build_send_packet(sizeof(command_2int_string_t)+namelen);
    wait_result();

    return syscall_retval;
}

int sc_close(int fd)
{
    command_int_t * command = (command_int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);

    memcpy(command->id, CMD_CLOSE, 4); 
    command->value0 = htonl(fd);

    build_send_packet(sizeof(command_int_t));
    wait_result();
    
    return syscall_retval;
}

typedef int mode_t;
int sc_creat(const char *pathname, mode_t mode)
{
    command_int_string_t * command = (command_int_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);

    int namelen = strlen(pathname);
 
    memcpy(command->id, CMD_CREAT, 4); 

    command->value0 = htonl(mode);

    memcpy(command->string, pathname, namelen+1);
    
    build_send_packet(sizeof(command_int_string_t)+namelen);
    wait_result();

    return syscall_retval;
}

int sc_link(const char *oldpath, const char *newpath)
{
    command_string_t * command = (command_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    int namelen1 = strlen(oldpath);
    int namelen2 = strlen(newpath);
 
    memcpy(command->id, CMD_LINK, 4); 

    memcpy(command->string, oldpath, namelen1 + 1);
    memcpy(command->string + namelen1 + 1, newpath, namelen2 + 1); 
    
    build_send_packet(sizeof(command_string_t)+namelen1+namelen2+1);
    wait_result();

    return syscall_retval;

}

int sc_unlink(const char *pathname)
{
    command_string_t * command = (command_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    int namelen = strlen(pathname);
 
    memcpy(command->id, CMD_UNLINK, 4); 

    memcpy(command->string, pathname, namelen + 1);
    
    build_send_packet(sizeof(command_string_t)+namelen);
    wait_result();

    return syscall_retval;
}

int sc_chdir(const char *path)
{
    command_string_t * command = (command_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    int namelen = strlen(path);
 
    memcpy(command->id, CMD_CHDIR, 4); 

    memcpy(command->string, path, namelen + 1);

    build_send_packet(sizeof(command_string_t)+namelen);
    wait_result();

    return syscall_retval;
}

int sc_chmod(const char *path, mode_t mode)
{
    command_int_string_t * command = (command_int_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);

    int namelen = strlen(path);
 
    memcpy(command->id, CMD_CHMOD, 4); 

    command->value0 = htonl(mode);

    memcpy(command->string, path, namelen+1);
    
    build_send_packet(sizeof(command_int_string_t)+namelen);
    wait_result();

    return syscall_retval;
}

off_t sc_lseek(int fildes, off_t offset, int whence)
{
    command_3int_t * command = (command_3int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_LSEEK, 4); 
    command->value0 = htonl(fildes);
    command->value1 = htonl(offset);
    command->value2 = htonl(whence);

    build_send_packet(sizeof(command_3int_t));
    wait_result();
    
    return syscall_retval;
}

int sc_fstat(int filedes, dcload_stat_t *buf)
{
    command_3int_t * command = (command_3int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_FSTAT, 4); 
    command->value0 = htonl(filedes);
    command->value1 = htonl(buf);
    command->value2 = htonl(sizeof(struct dcload_stat));
    
    build_send_packet(sizeof(command_3int_t));
    wait_result();
    
    return syscall_retval;
}

#if 0
time_t sc_time(time_t * t)
{
    command_int_t * command = (command_int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);

    
    memcpy(command->id, CMD_TIME, 4); 
    build_send_packet(sizeof(command_int_t));
    wait_result();
    
    return syscall_retval;
}
#endif

int sc_stat(const char *file_name, struct dcload_stat *buf)
{
    command_2int_string_t * command = (command_2int_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    int namelen = strlen(file_name);

    memcpy(command->id, CMD_STAT, 4); 
    memcpy(command->string, file_name, namelen+1);

    command->value0 = htonl(buf);
    command->value1 = htonl(sizeof(struct dcload_stat));
    
    build_send_packet(sizeof(command_2int_string_t)+namelen);
    wait_result();
    
    return syscall_retval;
}

#if 0
int sc_utime(const char *filename, struct utimbuf *buf)
{
    command_3int_string_t * command = (command_3int_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    int namelen = strlen(filename);
    
    memcpy(command->id, CMD_UTIME, 4); 
    memcpy(command->string, filename, namelen+1);
    
    command->value0 = htonl(buf);

    if (!buf) {
	command->value1 = htonl(buf->actime);
	command->value2 = htonl(buf->modtime);
    }

    build_send_packet(sizeof(command_3int_string_t)+namelen);
    wait_result();
    
    return syscall_retval;
}
#endif

DIR * sc_opendir(const char *name)
{
    command_string_t * command = (command_string_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    int namelen = strlen(name);
    
    memcpy(command->id, CMD_OPENDIR, 4); 
    memcpy(command->string, name, namelen+1);

    build_send_packet(sizeof(command_string_t)+namelen);
    wait_result();
    
    return (DIR *)syscall_retval;
}

int sc_closedir(DIR *dir)
{
    command_int_t * command = (command_int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_CLOSEDIR, 4); 
    command->value0 = htonl(dir);

    build_send_packet(sizeof(command_int_t));
    wait_result();
    
    return syscall_retval;
}

struct dcload_dirent our_dir;

struct dcload_dirent *sc_readdir(DIR *dir)
{
    command_3int_t * command = (command_3int_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN + UDP_H_LEN);
    
    memcpy(command->id, CMD_READDIR, 4); 
    command->value0 = htonl(dir);
    command->value1 = htonl(&our_dir);
    command->value2 = htonl(sizeof(struct dcload_dirent));

    build_send_packet(sizeof(command_3int_t));
    wait_result();
    
    if (syscall_retval)
	return &our_dir;
    else
	return 0;
}




#define DCLOAD_VERSION "1.0.4"
#define NAME "dcload-ip " DCLOAD_VERSION
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    unsigned int load_address;
    unsigned int load_size;
    unsigned char map[16384];
} bin_info_t;

static bin_info_t bin_info;

static unsigned char buffer[COMMAND_LEN + 1024]; /* buffer for response */
static command_t * response = (command_t *)buffer;

void cmd_loadbin(ip_header_t * ip, udp_header_t * udp, command_t * command)
{
    bin_info.load_address = ntohl(command->address);
    bin_info.load_size = ntohl(command->size);
    //memset(bin_info.map, 0, 16384);
    memset(bin_info.map, 0, (bin_info.load_size + 1023)/1024);

    our_ip = ntohl(ip->dest);
    
    make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
    make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) command, COMMAND_LEN, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
    eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN);

/*     if (!running) { */
/* 	if (!booted) */
/* 	    disp_info(); */
/* 	disp_status("receiving data..."); */
/*     } */
}
 
void cmd_partbin(ip_header_t * ip, udp_header_t * udp, command_t * command)
{ 
    int index = 0;

    memcpy((unsigned char *)ntohl(command->address), command->data, ntohl(command->size));
    
    index = (ntohl(command->address) - bin_info.load_address) >> 10;
    bin_info.map[index] = 1;
}

void cmd_donebin(ip_header_t * ip, udp_header_t * udp, command_t * command)
{
    int i;
    
    for(i = 0; i < (bin_info.load_size + 1023)/1024; i++)
	if (!bin_info.map[i])
	    break;
    if ( i == (bin_info.load_size + 1023)/1024 ) {
	command->address = htonl(0);
	command->size = htonl(0);
    }	else {
	command->address = htonl( bin_info.load_address + i * 1024);
	command->size = htonl(min(bin_info.load_size - i * 1024, 1024));
    }
    
    make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
    make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) command, COMMAND_LEN, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
    eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN);

/*     if (!running) { */
/* 	if (!booted) */
/* 	    disp_info(); */
/* 	disp_status("idle..."); */
/*     }	 */
}

void cmd_sendbinq(ip_header_t * ip, udp_header_t * udp, command_t * command)
{
    int numpackets, i;
    unsigned char *ptr;
    unsigned int bytes_left;
    unsigned int bytes_thistime;

    bytes_left = ntohl(command->size);
    numpackets = (ntohl(command->size)+1023) / 1024;
    ptr = (unsigned char *)ntohl(command->address);
    
    memcpy(response->id, CMD_SENDBIN, 4);
    for(i = 0; i < numpackets; i++) {
	if (bytes_left >= 1024)
	    bytes_thistime = 1024;
	else
	    bytes_thistime = bytes_left;
	bytes_left -= bytes_thistime;
		
	response->address = htonl((unsigned int)ptr);
	memcpy(response->data, ptr, bytes_thistime);
	response->size = htonl(bytes_thistime);
	make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN + bytes_thistime, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
	make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) response, COMMAND_LEN + bytes_thistime, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
	eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN + bytes_thistime);
	ptr += bytes_thistime;
    }
    
    memcpy(response->id, CMD_DONEBIN, 4);
    response->address = htonl(0);
    response->size = htonl(0);
    make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
    make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) response, COMMAND_LEN, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
    eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN);
}

void cmd_sendbin(ip_header_t * ip, udp_header_t * udp, command_t * command)
{
    our_ip = ntohl(ip->dest);

/*     if (!running) { */
/* 	if (!booted) */
/* 	    disp_info(); */
/* 	disp_status("sending data..."); */
/*     } */
    
    cmd_sendbinq(ip, udp, command);

/*     if (!running) { */
/* 	disp_status("idle..."); */
/*     } */
}

void cmd_version(ip_header_t * ip, udp_header_t * udp, command_t * command)
{
    int i;

    i = strlen("DCLOAD-IP " DCLOAD_VERSION) + 1;
    memcpy(response, command, COMMAND_LEN);
    strcpy(response->data, "DCLOAD-IP " DCLOAD_VERSION);
    make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN + i, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
    make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) response, COMMAND_LEN + i, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
    eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN + i);
}

void cmd_retval(ip_header_t * ip, udp_header_t * udp, command_t * command)
{
  make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
  make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) command, COMMAND_LEN, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
  eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN);

  syscall_retval = ntohl(command->address);
  escape_loop = 1;
  sem_signal(result_sema);
  if (waiting_thd) {
    thd_schedule_next(waiting_thd);
    waiting_thd = NULL;
  } else
    thd_schedule(1, 0);
}


void cmd_reboot(ether_header_t * ether, ip_header_t * ip, udp_header_t * udp, command_t * command)
{
/*     booted = 0; */
/*     running = 0; */

/*     disable_cache(); */
/*     go(0x8c004000); */
}

void cmd_execute(ether_header_t * ether, ip_header_t * ip, udp_header_t * udp, command_t * command)
{
    if (!running) {
	tool_ip = ntohl(ip->src);
	tool_port = ntohs(udp->src);
	memcpy(tool_mac, ether->src, 6);
	our_ip = ntohl(ip->dest);

	make_ip(ntohl(ip->src), ntohl(ip->dest), UDP_H_LEN + COMMAND_LEN, 17, (ip_header_t *)(pkt_buf + ETHER_H_LEN));
	make_udp(ntohs(udp->src), ntohs(udp->dest),(unsigned char *) command, COMMAND_LEN, (ip_header_t *)(pkt_buf + ETHER_H_LEN), (udp_header_t *)(pkt_buf + ETHER_H_LEN + IP_H_LEN));
	eth_txts(pkt_buf, ETHER_H_LEN + IP_H_LEN + UDP_H_LEN + COMMAND_LEN);

#if 0	
	if (!booted)
	    disp_info();
	else
	    disp_status("executing...");
	
	if (ntohl(command->size)&1)
	    *(unsigned int *)0x8c004004 = 0xdeadbeef; /* enable console */
	else
	    *(unsigned int *)0x8c004004 = 0xfeedface; /* disable console */
	if (ntohl(command->size)>>1)
	    cdfs_redir_enable();
	
	bb->stop();
	
	running = 1;

	disable_cache();
	go(ntohl(command->address));
#endif
    }
}





const int dcload_buffering = 1024;

#define BENPATCH

#define STOPIRQ
#define STARTIRQ

#ifdef BENPATCH
/* Added by ben for buffering ! */
typedef struct  {
  int max;        /* buffer max size. */
  int cur;        /* current read index into buffer */
  int cnt;        /* number of byte in buffer */
  int hdl;        /* kos file descriptor */
  int tell;       /* shadow tell() */
  int8 buffer[4]; /* read buffer */
} dcload_handler_t;

static dcload_handler_t * dcload_new_handler(int max)
{
  dcload_handler_t * h;
  int size;

  if (max < 1) {
    max = 1;
  }
  size = sizeof(*h) - sizeof(h->buffer) + max;
  h = malloc(size);
  if (h) {
    h->max = max;
    h->cur = 0;
    h->cnt = 0;
    h->hdl = 0;
    h->tell = 0;
    if ((int)h > 0) {
      free(h);
      h = 0;
    }
  }
  return h;
}

static dcload_handler_t * dcload_get_buffer_handler(uint32 hnd)
{
  return ((int)hnd < 0) ? (dcload_handler_t *) hnd : 0;
}

static int dcload_get_handler(uint32 hnd)
{
  dcload_handler_t * dh = dcload_get_buffer_handler(hnd);
  hnd = dh ? dh->hdl : hnd;
  return hnd;
}

/* $$$ ben : used sign for testing dcload direct handle or
   bufferised access. */
static void dcload_close_handler(uint32 hnd)
{
  int oldirq = 0;
  dcload_handler_t *dh;

  if (!hnd) {
    return;
  }
  dh = dcload_get_buffer_handler(hnd);
  if (dh) {
    hnd = dh->hdl;
    free(dh);
  }

  if (hnd > 100) { /* hack */
    STOPIRQ;
    sc_closedir(hnd);
    STARTIRQ;
  } else {
    hnd--; /* KOS uses 0 for error, not -1 */
    STOPIRQ;
    sc_close(hnd);
    STARTIRQ;
  }
}
#endif

/* Printk replacement */

void dcload_printk(const char *str) {
  int oldirq = 0;

  net_lock();
  STOPIRQ;
  sc_write(1, str, strlen(str));
  STARTIRQ;
  net_unlock();
}

static int printk_func(const uint8 *data, int len, int xlat)
{
  if (tool_ip) {
    int oldirq = 0;
    int res;

    if (irq_inside_int())
      return 0;

    net_lock();
    STOPIRQ;
    res = sc_write(1, data, len);
    STARTIRQ;
    net_unlock();
    return res;
  } else {
    if (real_old_printk_func)
      return real_old_printk_func(data, len, xlat);
    else
      return len;
  }
}


static char *dcload_path = NULL;

uint32 dcload_open(vfs_handler_t * dummy, const char *fn, int mode)
{
#ifdef BENPATCH
  dcload_handler_t * hdl = 0;
#endif
  int hnd = 0;
  int dcload_mode = 0;
  int oldirq = 0;
  int max_buffer = 0;

/*   dbglog(DBG_DEBUG, */
/* 	 "fs_dcload : open [%s,%d]\n",fn,mode); */
//  printf("open '%s' %d\n", fn, mode);
    
  if (!tool_ip)
    return 0;
  net_lock();
    
  if (mode & O_DIR) {
    if (fn[0] == '\0') {
      fn = "/";
    }
    STOPIRQ;
    hnd = (int) sc_opendir(fn);
    STARTIRQ;
    if (hnd) {
      if (dcload_path)
	free(dcload_path);
      if (fn[strlen(fn)] == '/') {
	dcload_path = malloc(strlen(fn)+1);
	strcpy(dcload_path, fn);
      } else {
	dcload_path = malloc(strlen(fn)+2);
	strcpy(dcload_path, fn);
	strcat(dcload_path, "/");
      }
    }
  } else { /* hack */
    if (mode == O_RDONLY) {
      max_buffer = dcload_buffering;
      dcload_mode = 0;
    } else if (mode == O_RDWR) {
      dcload_mode = 2 | 0x0200;
    } else if (mode == O_WRONLY) {
      dcload_mode = 1 | 0x0200;
    } else if (mode == O_APPEND) {
      dcload_mode =  2 | 8 | 0x0200;
    }
    STOPIRQ;
    hnd = sc_open(fn, dcload_mode, 0644);
    STARTIRQ;
    hnd++; /* KOS uses 0 for error, not -1 */
  }

#ifdef BENPATCH
  hdl = 0;
  if (hnd > 0 && max_buffer) {
    hdl = dcload_new_handler(max_buffer);
    if (hdl) {
      hdl->hdl = hnd;
      hnd = (int) hdl;
    }
    /* if alloc failed, fallback to nornal handle. */
  }
#endif

  net_unlock();

/*   dbglog(DBG_DEBUG, */
/* 	 "fs_dcload : open handler = [%p]\n", hdl); */

  return hnd;
}

void dcload_close(uint32 hnd)
{
  if (!tool_ip)
    return;
  net_lock();
#ifndef BENPATCH
  if (hnd > 100) /* hack */
    sc_closedir(hnd);
  else
    sc_close(hnd-1);
#else
  dcload_close_handler(hnd);
#endif
  net_unlock();
}

#ifdef BENPATCH
#define dcload_read_buffer sc_read
#if 0
#endif

#else

//#define FRAG 5*1024
#define FRAG 2048
static ssize_t dcload_read_buffer(uint32 hnd, int8 *buf, size_t cnt)
{
  const ssize_t frag = FRAG;
  int oldirq = 0;
  ssize_t ret = 0;
  
  while (cnt) {
    ssize_t err, n;

    n = cnt;
    if (n > frag) {
      n = frag;
    }
    cnt -= n;

    STOPIRQ;
/*    vid_border_color(0xff,0,0); */
    err = sc_read(hnd, buf, n);
/*    vid_border_color(0,0,0); */
    STARTIRQ;

    if (err < 0) {
/*      vid_border_color(0,0xff,0);*/
      if (!ret) {
	ret = -1;
      }
      break;
    } else if (!err) {
      break;
    } else {
      buf += err;
      ret += err;
/*       if (err != n) { */
/* 	vid_border_color(0,0,0xff); */
/* 	break; */
/*       } */
    }
/*     if (cnt && thd_enabled) { */
/*       thd_pass(); */
/*     } */
  }

  return ret;
}
#define dcload_read_buffer sc_read

#endif

#ifdef BENPATCH
static ssize_t retell(dcload_handler_t *h, int inc)
{
  int oldirq = 0;
  ssize_t ret;

  ret = h->tell;
  if (ret == -1) {
    STOPIRQ;
    ret = sc_lseek(h->hdl-1, 0, SEEK_CUR);
    STARTIRQ;
  } else {
    ret += inc;
  }
  return h->tell = ret;
}
#endif

ssize_t dcload_read(uint32 hnd, void *buf, size_t cnt)
{
  int oldirq = 0;
  ssize_t ret = -1;
    
  net_lock();

#ifndef BENPATCH
  if (hnd)
    ret = dcload_read_buffer(hnd-1, buf, cnt);
#else
  if (hnd) {
    dcload_handler_t * dh = dcload_get_buffer_handler(hnd);
    
    if (!dh || cnt > dcload_buffering) {
      if (dh) {
	hnd = dh->hdl;
	dh->cur = dh->cnt = 0;
	dh->tell = -1;
      }
      ret = dcload_read_buffer(hnd-1, buf, cnt);
    } else {
      int eof = 0;
      hnd = dh->hdl-1;
      ret = 0;

      while (cnt) {
	ssize_t n;

	n = dh->cnt - dh->cur;
	if (n <= 0) {
	  n = dh->cnt = dh->cur = 0;
	  if (!eof) {
	    n = dcload_read_buffer(hnd, dh->buffer, dh->max);
	    eof = n != dh->max;
	    if (n == -1) {
	      /* $$$ Try */
	      if (!ret)
		ret = n;
	      break;
	    }
	  }
	  dh->cnt = n;
	}

	if (!n) {
	  break;
	}
	retell(dh,n);

	if (n > cnt) {
	  n = cnt;
	}

	/* Fast copy */
	memcpy(buf, dh->buffer+dh->cur, n);
	dh->cur += n;
	cnt -= n;
	ret += n;
	buf = (void *)((int8 *)buf + n);
      }
    }
  }
#endif

  net_unlock();
  return ret;
}

ssize_t dcload_write(uint32 hnd, const void *buf, size_t cnt)
{
  int oldirq = 0;
  ssize_t ret = -1;
    	
  net_lock();

#ifndef BENPATCH
  if (hnd)
    ret = sc_write(hnd-1, buf, cnt);
#else
  hnd = dcload_get_handler(hnd);
    
  if (hnd) {
    hnd--; /* KOS uses 0 for error, not -1 */
    STOPIRQ;
    ret = sc_write(hnd, buf, cnt);
    STARTIRQ;
  }
#endif

  net_unlock();
  return ret;
}

off_t dcload_seek(uint32 hnd, off_t offset, int whence)
{
  int oldirq = 0;
  off_t ret = -1;

  net_lock();

#ifndef BENPATCH
  if (hnd)
    ret = sc_lseek(hnd-1, offset, whence);
#else
  if (hnd) {
    dcload_handler_t * dh = dcload_get_buffer_handler(hnd);
    if (!dh) {
      hnd--; /* KOS uses 0 for error, not -1 */
      STOPIRQ;
      ret = sc_lseek(hnd, offset, whence);
      STARTIRQ;
    } else {
      const int skip = 0x666;
      off_t cur, start, end;

      hnd = dh->hdl-1;

      switch (whence) {
      case SEEK_END:
	break;

      case SEEK_SET:
      case SEEK_CUR:
	cur = retell(dh,0);
	if (cur == -1) {
	  whence = skip;
	  break;
	}
	/* end is the buffer end file position */
	end = cur;
	/* start is the buffer start file position */
	start = end - dh->cnt;
	/* cur is the buffer current file position */
	cur = start + dh->cur;

	/* offset is the ABSOLUTE seeking position */
	if (whence == SEEK_CUR) {
	  offset += cur;
	  whence = SEEK_SET;
	}

	if (offset >= start && offset <= end) {
	  /* Seeking in the buffer :) */
	  dh->cur = offset - start;
	  whence = skip;
	  ret = offset;
	}
	break;

      default:
	whence = skip;
	break;
      }

      if (whence != skip) {
	STOPIRQ;
	ret = sc_lseek(hnd, offset, whence);
	STARTIRQ;
	dh->cur = dh->cnt = 0;
	dh->tell = ret;
      }
    }
  }
#endif
  net_unlock();
  return ret;
}

off_t dcload_tell(uint32 hnd)
{
  return dcload_seek(hnd, 0, SEEK_CUR);
}

size_t dcload_total(uint32 hnd) {
  int oldirq = 0;
  size_t ret = -1;
  size_t cur;
	
  net_lock();

#ifdef BENPATCH
  hnd = dcload_get_handler(hnd);
#endif
	
  if (hnd) 
  {
    hnd--; /* KOS uses 0 for error, not -1 */
    STOPIRQ;
    cur = sc_lseek(hnd, 0, SEEK_CUR);
    ret = sc_lseek(hnd, 0, SEEK_END);
    sc_lseek(hnd, cur, SEEK_SET);
    STARTIRQ;
  }
	
  net_unlock();
  return ret;
}

/* Not thread-safe, but that's ok because neither is the FS */
static dirent_t dirent;
dirent_t *dcload_readdir(uint32 hnd)
{
  int oldirq = 0;
  dirent_t *rv = NULL;
  dcload_dirent_t *dcld;
  dcload_stat_t filestat;
  char *fn;

#ifdef BENPATCH
  hnd = dcload_get_handler(hnd);
#endif

  if (hnd < 100) return NULL; /* hack */

  net_lock();

  STOPIRQ;
  dcld = (dcload_dirent_t *)sc_readdir(hnd);
  STARTIRQ;
    
  if (dcld) {
    rv = &dirent;
    strcpy(rv->name, dcld->d_name);
    rv->size = 0;
    rv->time = 0;
    rv->attr = 0; /* what the hell is attr supposed to be anyways? */

    fn = malloc(strlen(dcload_path)+strlen(dcld->d_name)+1);
    strcpy(fn, dcload_path);
    strcat(fn, dcld->d_name);

    STOPIRQ;
    if (!sc_stat(fn, &filestat)) {
      if (filestat.st_mode & S_IFDIR)
	rv->size = -1;
      else
	rv->size = filestat.st_size;
      rv->time = filestat.st_mtime;
	    
    }
    STARTIRQ;
	
    free(fn);
  }
    
  net_unlock();
  return rv;
}

int dcload_rename(vfs_handler_t * dummy, const char *fn1, const char *fn2) {
  int oldirq = 0;
  int ret;

  net_lock();

  /* really stupid hack, since I didn't put rename() in dcload */

  STOPIRQ;
  ret = sc_link(fn1, fn2);

  if (!ret)
    ret = sc_unlink(fn1);
  STARTIRQ;

  net_unlock();
  return ret;
}

int dcload_unlink(vfs_handler_t * dummy, const char *fn) {
  int oldirq = 0;
  int ret;

  net_lock();

  STOPIRQ;
  ret = sc_unlink(fn);
  STARTIRQ;

  net_unlock();
  return ret;
}


/* Pull all that together */
static vfs_handler_t vh = {
  {
    { "/pc" },          /* path prefix */
    0, 
    0x00010000,		/* Version 1.0 */
    0,			/* flags */
    NMMGR_TYPE_VFS,	/* VFS handler */
    NMMGR_LIST_INIT	/* list */
  },
  0, 0,		       /* In-kernel, no cacheing */
  dcload_open, 
  dcload_close,
  dcload_read,
  dcload_write,
  dcload_seek,
  dcload_tell,
  dcload_total,
  dcload_readdir,
  NULL,               /* ioctl */
  dcload_rename,
  dcload_unlink,
  NULL                /* mmap */
};


static int init;

static nmmgr_handler_t * oldfs;

int fs_init()
{
  nmmgr_handler_t * hnd;

  if (init)
    return 0;

  init = 1;

  result_sema = sem_create(0);

  real_old_printk_func = old_printk_func;
  old_printk_func = printk_func;

  hnd = nmmgr_lookup("/pc");
  if (hnd) {
    nmmgr_handler_remove(hnd);
    oldfs = hnd;
  }

  /* Register with VFS */
  return tcpfs_init() || httpfs_init() || nmmgr_handler_add(&vh);
}

void fs_shutdown()
{
  httpfs_shutdown();
  tcpfs_shutdown();

  if (!init)
    return;
  init = 0;
  nmmgr_handler_remove(&vh);

  old_printk_func = real_old_printk_func;

  if (oldfs) {
    nmmgr_handler_add(oldfs);
    oldfs = NULL;
  }

  sem_destroy(result_sema);
  result_sema = NULL;
}

