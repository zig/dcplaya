/* VP : adapted for net driver in dcplaya */

/*
 * HTTP protocol for ffmpeg client
 * Copyright (c) 2000, 2001 Fabrice Bellard.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <kos.h>
#include <stdio.h>
#include <string.h>

#include "lwip/api.h"

/* XXX: POST protocol is not completly implemented because ffmpeg use
   only a subset of it */

//#define DEBUG

typedef struct {
  struct netconn * conn;
  struct netbuf * buf;

  u8_t * data;
  u16_t len;
  int inilen;
  int error;
} TCPContext;


int dns(const char * name, struct ip_addr * res)
{
    int i;
    int c;
    unsigned char *ip = (unsigned char *)&res->addr;

    i = 0;
    c = 0;
    
    res->addr = 0;
    while(name[c] != 0) {
	if (name[c] != '.') {
	    ip[i] *= 10;
	    ip[i] += name[c] - '0';
	}
	else
	    i++;
	c++;
    }

    //res->addr = ntohl(res->addr);

    return 0;
}



/* return zero if error */
static uint32 open(const char *uri, int flags, int udp)
{
    char hostname[128];
    int port;
    TCPContext *s;
    struct netconn * conn = 0;
    struct ip_addr addr;

    //h->is_streamed = 1;

    if (flags & O_DIR)
      return 0;

    s = malloc(sizeof(TCPContext));
    if (!s) {
        return 0;
    }

    s->buf = NULL;
    s->error = 0;

    /* fill the dest addr */
    /* needed in any case to build the host string */
    url_split(NULL, 0, hostname, sizeof(hostname), &port, 
              NULL, 0, uri);

    if (port < 0)
        port = 80;

    //    printf("TCP : addr '%s' port %d\n", hostname, port);

    conn = netconn_new(udp? NETCONN_UDP : NETCONN_TCP);

    if (conn == NULL)
      goto fail;

    if (dns(hostname, &addr))
      goto fail;

    //printf("ip = %x\n", addr.addr);

    if (netconn_connect(conn, &addr, port))
      goto fail;

    s->conn = conn;

    return (uint32) s;
 fail:
    if (conn)
        netconn_delete(conn);
    free(s);
    return 0;
}

static uint32 tcpfs_open(const char *uri, int flags)
{
  return open(uri, flags, 0);
}

static uint32 udpfs_open(const char *uri, int flags)
{
  return open(uri, flags, 1);
}


static int tcpfs_read(uint32 h, uint8 *buf, int size)
{
    TCPContext *s = (TCPContext *)h;
    int len = 0;

    //len = fs_read(s->conn, buf, size);
    if (s->error)
      return -1;

    while (size > 0) {
      int l;

      if (s->buf == NULL) {
	s->buf = netconn_recv(s->conn);
	if (s->buf == NULL) {
	  if (!len) {
	    s->error = 1;
	    //return -1;
	  }
	  return len;
	}
	netbuf_first(s->buf);
	netbuf_data(s->buf, &s->data, &s->len);
	s->inilen = s->len;
	//printf("TCPFS : recv %d\n", s->len);
	if (s->len == 0)
	  return len;
      }
      
      l = s->len;
      if (l > size)
	l = size;
      if (l > 0)
	memcpy(buf+len, s->data, l);

      s->len -= l;
      s->data += l;

      len += l;
      size -= l;

      if (s->len <= 0) {
	if (netbuf_next(s->buf) < 0) {
	  netbuf_delete(s->buf);
	  s->buf = NULL;
	  //return len;
	} else {
	  netbuf_data(s->buf, &s->data, &s->len);
	  s->inilen = s->len;
	  //printf("TCPFS : next %d\n", s->len);
	  if (s->len <= 0)
	    return len;
	}
      }
    }

    return len;
}

static off_t tcpfs_seek(uint32 hnd, int size, int whence)
{
    TCPContext *s = (TCPContext *)hnd;
    int len = 0;

    if (whence != SEEK_CUR)
      return 0;

    if (s->error)
      return -1;

    //len = fs_read(s->conn, buf, size);

    if (size < 0) {
      if (s->buf == NULL)
	return 0;

      size = -size;
      if (s->inilen - s->len < size)
	size = s->inilen - s->len;
      s->len += size;
      s->data -= size;
      return -size;
    }

    while (size > 0) {
      int l;

      if (s->buf == NULL) {
	s->buf = netconn_recv(s->conn);
	if (s->buf == NULL) {
	  //s->error = 1;
	  return len;
	}
	netbuf_first(s->buf);
	netbuf_data(s->buf, &s->data, &s->len);
	s->inilen = s->len;
	//printf("TCPFS : recv %d\n", s->len);
	if (s->len == 0)
	  return len;
      }
      
      l = s->len;
      if (l > size)
	l = size;
/*       if (l > 0) */
/* 	memcpy(buf+len, s->data, l); */

      s->len -= l;
      s->data += l;

      len += l;
      size -= l;

      if (s->len <= 0) {
	if (netbuf_next(s->buf) < 0) {
	  netbuf_delete(s->buf);
	  s->buf = NULL;
	  //return len;
	} else {
	  netbuf_data(s->buf, &s->data, &s->len);
	  s->inilen = s->len;
	  //printf("TCPFS : next %d\n", s->len);
	  if (s->len <= 0)
	    return len;
	}
      }
    }

    return len;
}

static int tcpfs_write(uint32 h, uint8 *buf, int size)
{
    TCPContext *s = (TCPContext *)h;
    //printf("TCPFS : write %d\n", size);
    return netconn_write(s->conn, buf, size, NETCONN_COPY);
}

static int tcpfs_close(uint32 h)
{
    TCPContext *s = (TCPContext *)h;
    if (s->buf)
	netbuf_delete(s->buf);
    netconn_delete(s->conn);
    free(s);
    return 0;
}


/* Pull all that together */
static vfs_handler vh = {
  { "/tcp" },          /* path prefix */
  0, 0,		       /* In-kernel, no cacheing */
  NULL,                /* linked list pointer */
  tcpfs_open, 
  tcpfs_close,
  tcpfs_read,
  tcpfs_write,
  tcpfs_seek,
  NULL,//dcload_tell,
  NULL,//dcload_total,
  NULL,//dcload_readdir,
  NULL,               /* ioctl */
  NULL,//dcload_rename,
  NULL,//dcload_unlink,
  NULL                /* mmap */
};

/* Pull all that together */
static vfs_handler vh_udpfs = {
  { "/udp" },          /* path prefix */
  0, 0,		       /* In-kernel, no cacheing */
  NULL,                /* linked list pointer */
  udpfs_open, 
  tcpfs_close,
  tcpfs_read,
  tcpfs_write,
  tcpfs_seek,
  NULL,//dcload_tell,
  NULL,//dcload_total,
  NULL,//dcload_readdir,
  NULL,               /* ioctl */
  NULL,//dcload_rename,
  NULL,//dcload_unlink,
  NULL                /* mmap */
};


static int init;
int tcpfs_init()
{
  if (init)
    return 0;

  init = 1;

  /* Register with VFS */
  return fs_handler_add("/tcp", &vh) ||  fs_handler_add("/udp", &vh_udpfs);
}

void tcpfs_shutdown()
{
  if (!init)
    return;
  init = 0;
  fs_handler_remove(&vh);
}


//#endif
