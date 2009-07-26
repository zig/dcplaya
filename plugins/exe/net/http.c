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

/* XXX: POST protocol is not completly implemented because ffmpeg use
   only a subset of it */

#define DEBUG

/* used for protocol handling */
#define URL_SIZE    256

#define MAX_HEADER_ENTRIES 64

typedef struct {
  file_t hd;
  int line_count;
  int len;
  int tread;
  int bpos, bsz;
  int http_code;
  char location[URL_SIZE];
  char hoststr[256];
  char path[256];
  int flags;

  char * header_entries[MAX_HEADER_ENTRIES];
  int nb_header_entries;
} HTTPContext;


static void hdr_clear(HTTPContext * h)
{
  int i;
  for (i=0; i<h->nb_header_entries; i++) {
    free(h->header_entries[i]);
    h->header_entries[i] = NULL;
  }
  h->nb_header_entries = 0;
}

static void hdr_add(HTTPContext * h, const char * entry)
{
  if (h->nb_header_entries >= MAX_HEADER_ENTRIES)
    return;

  h->header_entries[h->nb_header_entries ++] = strdup(entry);
}

static const char * hdr_get(HTTPContext * h, int i)
{
  if (i >= h->nb_header_entries)
    return NULL;

  return h->header_entries[i];
}


static int http_connect(HTTPContext * h, const char *path, const char *hoststr, int flags, int wait);
static int http_write(uint32 h, uint8 *buf, int size);


/**
 * Copy the string str to buf. If str length is bigger than buf_size -
 * 1 then it is clamped to buf_size - 1.
 * NOTE: this function does what strncpy should have done to be
 * useful. NEVER use strncpy.
 * 
 * @param buf destination buffer
 * @param buf_size size of destination buffer
 * @param str source string
 */
static void pstrcpy(char *buf, int buf_size, const char *str)
{
    int c;
    char *q = buf;

    if (buf_size <= 0)
        return;

    for(;;) {
        c = *str++;
        if (c == 0 || q >= buf + buf_size - 1)
            break;
        *q++ = c;
    }
    *q = '\0';
}



void url_split(char *proto, int proto_size,
               char *hostname, int hostname_size,
               int *port_ptr,
               char *path, int path_size,
               const char *url)
{
    const char *p;
    char *q;
    int port;

    port = -1;

    p = url;
/*     q = proto; */
/*     while (*p != ':' && *p != '\0') { */
/*         if ((q - proto) < proto_size - 1) */
/*             *q++ = *p; */
/*         p++; */
/*     } */
/*     if (proto_size > 0) */
/*         *q = '\0'; */
    if (*p == '\0') {
        if (proto_size > 0)
            proto[0] = '\0';
        if (hostname_size > 0)
            hostname[0] = '\0';
        p = url;
    } else {
        p++;
        if (*p == '/')
            p++;
        if (*p == '/')
            p++;
        q = hostname;
        while (*p != ':' && *p != '/' && *p != '?' && *p != '\0') {
            if ((q - hostname) < hostname_size - 1)
                *q++ = *p;
            p++;
        }
        if (hostname_size > 0)
            *q = '\0';
        if (*p == ':') {
            p++;
            port = (unsigned long) strtol(p, (char **)&p, 10);
        }
    }
    if (port_ptr)
        *port_ptr = port;
    pstrcpy(path, path_size, p);
}



/* return zero if error */
static uint32 http_open(vfs_handler_t * dummy, const char *uri, int flags)
{
  const char *path, *proxy_path;
  char hostname[128];
  char path1[256];
  int port, use_proxy, err;
  HTTPContext *s;
  file_t hd = -1;
  int wait = 0;

  if (flags & O_DIR)
    return 0;

  //h->is_streamed = 1;

  s = malloc(sizeof(HTTPContext));
  if (!s) {
    return 0;
  }

  /*     proxy_path = getenv("http_proxy"); */
  /*     use_proxy = (proxy_path != NULL) && !getenv("no_proxy") &&  */
  /*         strstart(proxy_path, "http://", NULL); */
  use_proxy = 0;

  s->nb_header_entries = 0;

  /* fill the dest addr */
 redo:
  /* needed in any case to build the host string */
  url_split(NULL, 0, hostname, sizeof(hostname), &port, 
	    path1, sizeof(path1), uri);
  if (port > 0) {
    snprintf(s->hoststr, sizeof(s->hoststr), "%s:%d", hostname, port);
  } else {
    pstrcpy(s->hoststr, sizeof(s->hoststr), hostname);
  }

  if (use_proxy) {
    url_split(NULL, 0, hostname, sizeof(hostname), &port, 
	      NULL, 0, proxy_path);
    path = uri;
  } else {
    if (path1[0] == '\0')
      path = "/";
    else
      path = path1;
  }
  if (port < 0)
    port = 80;

  snprintf(s->location, sizeof(s->location), "/tcp/%s:%d", hostname, port);

  //printf("HTTPFS : opening '%s' '%s'\n", s->location, path);

 redo2:
  hd = fs_open(s->location, O_RDWR);
  err = hd >= 0? 0:-1;
  //    err = url_open(&hd, buf, URL_RDWR);
    
  if (err < 0)
    goto fail;

  s->hd = hd;
  strcpy(s->path, path);
  s->flags = flags;
  if (http_connect(s, path, s->hoststr, flags, wait) < 0) {
    if (0&&wait <= 2000) {
      /* try again with a sleep */
      wait += 1000;
      fs_close(hd);
      hd = -1;
      goto redo2;
    }
    goto fail;
  }
  if ((s->http_code == 303 || s->http_code == 302) && s->location[0] != '\0') {
    /* url moved, get next */
    uri = s->location+6;
#ifdef DEBUG
    printf("URL moved get next '%s'\n", uri);
#endif
    fs_close(hd);
    hd = -1;
    //wait = 4000;
    goto redo;
  }
  if (s->http_code != 200)
    goto fail;
  return (uint32) s;
 fail:
  hdr_clear(s);

  if (hd>=0)
    fs_close(hd);
  free(s);
  return 0;
}

static int http_getc(uint32 h)
{
  char c;
  int res;
  HTTPContext *s = (HTTPContext *)h;
  int retry = 10;

#if 0
  if (s->bpos >= s->bsz) {
    s->bsz = fs_read(s->hd, s->path, sizeof(s->path));    
/*     while (s->bsz < 1 && retry > 0) { */
/*       retry--; */
/*       printf("http_getc failed. Retrying in 0.5 seconds ...\n"); */
/*       thd_sleep(500); */
/*       s->bsz = fs_read(s->hd, s->path, sizeof(s->path));     */
/*     } */
    s->bpos = 0;
  }

  if (s->bpos >= s->bsz)
    return -1;

  c = s->path[s->bpos++];

#else

  res = fs_read(s->hd, &c, 1);
/*   while (res < 1 && retry > 0) { */
/*     retry--; */
/*     printf("http_getc failed. Retrying in 0.5 seconds ...\n"); */
/*     thd_sleep(500); */
/*     res = fs_read(s->hd, &c, 1); */
/*   } */
  if (res < 1)
    return -1;

#endif
  
  return c;
}

static int process_line(HTTPContext *s, char *line, int line_count)
{
    char *tag, *p;
    
    /* end of header */
    if (line[0] == '\0')
        return 0;

    p = line;
    if (line_count == 0) {
        while (!isspace(*p) && *p != '\0')
            p++;
        while (isspace(*p))
            p++;
        s->http_code = strtol(p, NULL, 10);
#ifdef DEBUG
        printf("http_code=%d\n", s->http_code);
#endif
    } else {
        while (*p != '\0' && *p != ':')
            p++;
        if (*p != ':') 
            return 1;
        
        *p = '\0';
        tag = line;
        p++;
        while (isspace(*p))
            p++;
        if (!strcmp(tag, "Location")) {
            strcpy(s->location, p);
        }
        if (!strcmp(tag, "Content-Length")) {
	  s->len = strtol(p, NULL, 10);
#ifdef DEBUG
	  printf("len=%d\n", s->len);
#endif
        }
    }
    return 1;
}

static int http_connect(HTTPContext *s, const char *path, const char *hoststr, int flags, int wait)
{
    int post, err, ch;
    char line[512], *q;

    hdr_clear(s);

    /* send http header */
    post = flags & O_WRONLY;

    s->len = 0;
    s->tread = 0;
    s->bpos = 0; s->bsz = 0;

    snprintf(line, sizeof(line),
             "%s %s HTTP/1.0\r\n"
             "User-Agent: %s\r\n"
             "Host: %s\r\n"
             "Accept: */*\r\n"
	     "Connection: keep-alive\r\n"
             "\r\n",
             post ? "POST" : "GET",
             path,
             //"Wget/1.9",
	     "dcplaya_net",
             hoststr);
    
    if (http_write((uint32) s, line, strlen(line)) < 0)
        return -1;

#ifdef DEBUG
    printf("http : sent header -->\n%s", line);
#endif
        
    /* init input buffer */
    s->line_count = 0;
    //s->location[0] = '\0';
    if (post) {
      //sleep(1);
      
        return 0;
    }
    
    /* wait for header */
    q = line;
    if (wait)
      thd_sleep(wait);
    for(;;) {
        ch = http_getc((uint32) s);
#ifdef DEBUG
	//printf("%c", ch);
#endif
        if (ch < 0) {
	  printf("http header truncated\n");
	  return 0;
	}
        if (ch == '\n') {
            /* process line */
            if (q > line && q[-1] == '\r')
                q--;
            *q = '\0';
#ifdef DEBUG
            printf("header='%s'\n", line);
#endif

	    if (line[0])
	      hdr_add(s, line);

            err = process_line(s, line, s->line_count);
            if (err < 0)
                return err;
            if (err == 0)
	      return 0;
	      //return s->len? 0 : -1;

            s->line_count++;
            q = line;
        } else {
            if ((q - line) < sizeof(line) - 1)
                *q++ = ch;
        }
    }
}


static int http_read(uint32 h, uint8 *buf, int size)
{
    HTTPContext *s = (HTTPContext *)h;
    int len;

    if (!s->hd)
      return -1;

    while (size > 0 && s->bpos < s->bsz) {
      *buf++ = s->path[s->bpos++];
      size--;
    }

    if (s->len && size > s->len - s->tread)
      size = s->len - s->tread;
    if (size <= 0) {
      //printf("http: reached end of file\n");
      return 0;// -1;
    }

    len = fs_read(s->hd, buf, size);
    if (len < 0)
      return 0;// -1;
    s->tread += len;

/*     if (size > 10) { */
/*       char buf2[64]; */
/*       int len2 = len; */
/*       if (len2 > 63) len2 = 63; */
/*       memcpy(buf2, buf, len2); */
/*       buf2[len2] = 0; */
/*       printf("http_read %d %d\n%s\n", size, len, buf2); */
/*     } */

    return len;
}

/* used only when posting data */
static int http_write(uint32 h, uint8 *buf, int size)
{
    HTTPContext *s = (HTTPContext *)h;
    if (!s->hd)
      return -1;
    return fs_write(s->hd, buf, size);
}

static off_t http_seek(uint32 hnd, off_t offset, int whence)
{
  HTTPContext *s = (HTTPContext *)hnd;
  file_t hd;

  //printf("http_seek %d %d\n", offset, whence);
  if (whence == SEEK_SET) {
    whence = SEEK_CUR;
    offset = offset - s->tread;
  }
  
  if (whence == SEEK_CUR) {
    int pos = fs_seek(s->hd, 0, SEEK_CUR);
    int res = fs_seek(s->hd, offset, SEEK_CUR) - pos;
    //printf("http_seek res = %d\n", res);
    s->tread += res;
    return s->tread;
  }

  if (whence != SEEK_SET)
    return -1;

  fs_close(s->hd);
  s->hd = -1;

  //printf("location = '%s'\n", s->location);
  hd = fs_open(s->location, O_RDWR);
    
  if (hd<0) {
    printf("http_seek : could not reconnect to '%s'\n", s->location);
    goto fail;
  }

  s->hd = hd;
  if (http_connect(s, s->path, s->hoststr, s->flags, 0) < 0)
    goto fail;

  if (offset) {
    int pos = fs_seek(s->hd, 0, SEEK_CUR);
    s->tread = fs_seek(s->hd, offset, SEEK_CUR) - pos;
    //printf("http_seek res = %d\n", s->tread);
    return s->tread;
  }

  s->tread = 0;
  return 0;

 fail:
  printf("http_seek : failed reopen\n");
  if (s->hd >= 0)
    fs_close(s->hd);
  s->hd = 0;

  return -1;
}


static int http_close(uint32 h)
{
    HTTPContext *s = (HTTPContext *)h;
    hdr_clear(s);
    if (s->hd >= 0)
      fs_close(s->hd);
    free(s);
    return 0;
}

#if 0
URLProtocol http_protocol = {
    "http",
    http_open,
    http_read,
    http_write,
    NULL, /* seek */
    http_close,
};
#endif

/* Pull all that together */
static vfs_handler_t vh = {
  {
    { "/http" },          /* path prefix */
    0, 
    0x00010000,		/* Version 1.0 */
    0,			/* flags */
    NMMGR_TYPE_VFS,	/* VFS handler */
    NMMGR_LIST_INIT	/* list */
  },
  0, 0,		       /* In-kernel, no cacheing */
  http_open, 
  http_close,
  http_read,
  http_write,
  http_seek,
  NULL,//dcload_tell,
  NULL,//dcload_total,
  NULL,//dcload_readdir,
  NULL,               /* ioctl */
  NULL,//dcload_rename,
  NULL,//dcload_unlink,
  NULL                /* mmap */
};


static int init;
int httpfs_init()
{
  if (init)
    return 0;

  init = 1;

  /* Register with VFS */
  return nmmgr_handler_add(&vh);
}

void httpfs_shutdown()
{
  if (!init)
    return;
  init = 0;
  nmmgr_handler_remove(&vh);
}


const char * http_get_header(file_t f, int i)
{
  HTTPContext * h;

  if (fs_get_handler(f) != &vh)
    return NULL;

  h = fs_get_handle(f);
  if (!h)
    return NULL;

  return hdr_get(h, i);
}



//#endif
