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
#include <ctype.h>

#include "lwip/lwip.h"
#include "lwip/sockets.h"

//#define DEBUG

typedef struct {
  int socket;
  int pos;
  int stream;
} TCPContext;

struct sockaddr_in dnssrv;

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
      if (!isdigit(name[c]))
	goto dns;
      ip[i] *= 10;
      ip[i] += name[c] - '0';
    }
    else {
      if (name[c+1] == '.')
	goto dns;
      i++;
    }
    if (i >= 5)
      goto dns;
    c++;
  }

  //res->addr = ntohl(res->addr);

  return 0;

 dns:
  if (dnssrv.sin_port) {
    if (lwip_gethostbyname(&dnssrv, name, &res->addr) < 0) {
      printf("gethostbyname: Can't look up name");
      return -1;
    } else {
      return 0;
    }
  } else
    return -1;
}



/* return zero if error */
static uint32 open(const char *uri, int flags, int udp)
{
    char hostname[128];
    char options[128];
    int port;
    TCPContext *s;
    int sock = -1;
    struct sockaddr_in sa;
    int res;

    //h->is_streamed = 1;

    if (flags & O_DIR)
      return 0;

    s = malloc(sizeof(TCPContext));
    if (!s) {
        return 0;
    }

    /* fill the dest addr */
    /* needed in any case to build the host string */
    url_split(NULL, 0, hostname, sizeof(hostname), &port, 
              options, sizeof(options), uri);

    if (port < 0)
        port = 80;

    //    printf("TCP : addr '%s' port %d\n", hostname, port);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (dns(hostname, &sa.sin_addr.s_addr))
      goto fail;
    
    sock = socket(PF_INET, udp? SOCK_DGRAM:SOCK_STREAM, 0);
    if (sock < 0)
      goto fail;

    res = connect(sock, &sa, sizeof(sa));
    //printf("connect --> %d\n", res);
    if (res)
      goto fail;

    s->socket = sock;
    s->pos = 0;
    s->stream = strstr(options, "<stream>");

    return (uint32) s;
 fail:
    if (sock >= 0)
      close(sock);
    free(s);
    return 0;
}

static uint32 tcpfs_open(vfs_handler_t * vfs, const char *uri, int flags)
{
  return open(uri, flags, 0);
}

static uint32 udpfs_open(vfs_handler_t * vfs, const char *uri, int flags)
{
  return open(uri, flags, 1);
}


static int tcpfs_read(uint32 h, uint8 *buf, int size)
{
  int len = 0;
  TCPContext *s = (TCPContext *)h;

  do {
    int l;
    //printf("reading size %d -->", size);
    l = read(s->socket, buf, size);
    //printf(" %d\n", l);
    if (l >= 0) {
      len += l;
      s->pos += l;
      buf += l;
      size -= l;
    } else if (len == 0)
      return l;
    else
      return len;
  } while(s->stream && 0 < size);

  return len;
}

static off_t tcpfs_seek(uint32 hnd, int size, int whence)
{
  TCPContext *s = (TCPContext *)hnd;
  int len = 0;

  if (whence != SEEK_CUR)
    return s->pos;

  if (size <= 0)
    return s->pos;

  {
    uint8 buf[1024];
    while (size > 0) {
      int l = size>1024? 1024:size;

      l = read(s->socket, buf, l);
      if (l<=0)
	break;

      len += l;
      size -= l;
    }
  }

  return s->pos + len;
}

static int tcpfs_write(uint32 h, uint8 *buf, int size)
{
    TCPContext *s = (TCPContext *)h;
    //printf("TCPFS : write %d\n", size);
    return write(s->socket, buf, size);
}

static int tcpfs_close(uint32 h)
{
    TCPContext *s = (TCPContext *)h;
    //printf("close socket %d\n", s->socket);
    close(s->socket);
    free(s);
    return 0;
}

static int tcpfs_tell(uint32 h)
{
    TCPContext *s = (TCPContext *)h;
    return s->pos;
}


/* Pull all that together */
static vfs_handler_t vh = {
  {
    { "/tcp" },         /* path prefix */
    0, 
    0x00010000,		/* Version 1.0 */
    0,			/* flags */
    NMMGR_TYPE_VFS,	/* VFS handler */
    NMMGR_LIST_INIT	/* list */
  },
  0, 0,		        /* In-kernel, no cacheing */
  tcpfs_open, 
  tcpfs_close,
  tcpfs_read,
  tcpfs_write,
  tcpfs_seek,
  tcpfs_tell,
  NULL,//dcload_total,
  NULL,//dcload_readdir,
  NULL,               /* ioctl */
  NULL,//dcload_rename,
  NULL,//dcload_unlink,
  NULL                /* mmap */
};

/* Pull all that together */
static vfs_handler_t vh_udpfs = {
  {
    { "/udp" },         /* path prefix */
    0, 
    0x00010000,		/* Version 1.0 */
    0,			/* flags */
    NMMGR_TYPE_VFS,	/* VFS handler */
    NMMGR_LIST_INIT	/* list */
  },
  0, 0,		        /* In-kernel, no cacheing */
  udpfs_open, 
  tcpfs_close,
  tcpfs_read,
  tcpfs_write,
  tcpfs_seek,
  tcpfs_tell,
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
  return nmmgr_handler_add(&vh) ||  nmmgr_handler_add(&vh_udpfs);
}

void tcpfs_shutdown()
{
  if (!init)
    return;
  init = 0;
  nmmgr_handler_remove(&vh);
  nmmgr_handler_remove(&vh_udpfs);
}


//#endif
