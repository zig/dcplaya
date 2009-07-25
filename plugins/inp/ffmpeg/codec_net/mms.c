
/* This is an adaptation from mimms to dcplaya */

#include "lwip/lwip.h"
#include "lwip/sockets.h"

#include "ffmpeg.h"

#include <assert.h>

#undef printf
#undef fprintf
#undef srand

#define _(a) a


//#define fprintf(a, ARGS...) printf(ARGS)

/* used for protocol handling */
#define BUFFER_SIZE 1024
#define URL_SIZE    512

typedef struct {
  //URLContext *hd;
  file_t hd;
  char * buffer;
  int pos;
  int hdr;
  int asf_header_len;
  int packet_length;
  uint8_t asf_header[8192];
} MMSContext;

#undef read
#undef write

static int read(MMSContext * s, void * buf, int len)
{
  return fs_read(s->hd, buf, len);
}
#undef recv
#define recv(a,b,c,d) read(a,b,c)

static int write(MMSContext * s, void * buf, int len)
{
  return fs_write(s->hd, buf, len);
}
#undef send
#define send(a,b,c,d) write(a,b,c)


/*
 * Copyright (C) 2004 Wesley J. Landaker <wjl@icecavern.net> 
 *
 * MiMMS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * MiMMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/* 
 * Copyright (C) 2000-2001 major mms <http://www.geocities.com/majormms/>
 *
 * This file is part of libmms
 * 
 * libmms is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libmms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * utility functions to handle communication with an mms server
 */

/*
 * mms://netshow.msn.com/msnbc8 
 * mms://216.106.172.144/bbc1099/ads/ibeam/0_ibeamEarth_aaab00020_15_350k.asf
 * mms://195.124.124.82/56/081001_angriffe_1200.wmv
 * mms://193.159.244.12/n24_wmt_mid
 */


/* #include <stdint.h> */
/* #include <stdlib.h> */
/* #include <unistd.h> */
/* #include <stdio.h> */
/* #include <netdb.h> */
/* #include <string.h> */
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
/* #include <uuid/uuid.h> */
/* #include "bswap.h" */
/* #include "gettext.h" */

#undef dprintf
//#define DEBUG
#ifdef DEBUG
# define dprintf(x...) printf(x)
#else
# define dprintf(x...) 
#endif

#define BUF_SIZE (1024)  /* VP : was 128*1024 */

typedef struct {

  uint8_t buf[BUF_SIZE];
  int     num_bytes;

} command_t;
static command_t  cmd;

static int seq_num;
static int num_stream_ids;
static int stream_ids[20];
//int output_fh;
#define output_fh s


static void put_32 (command_t *cmd, uint32_t value) {
  int i;
  for (i=0; i<4; i++) {
    cmd->buf[cmd->num_bytes++] = (value >> (8*i)) & 0xff;
  }
}

static uint32_t get_32 (const void *cmd, int offset) {
  const unsigned char *ccmd = (const unsigned char *)cmd;
  uint32_t value = 0;
  int i;
  for (i=0; i<4; i++) {
    value |= ((uint32_t)ccmd[offset+i]&0xff)<<(8*i);
  }
  return value;
}

static void send_command (MMSContext * s, int command, uint32_t switches, 
			  uint32_t extra, int length,
			  char *data) {
  
  int        len8;
  int        i;

  len8 = (length + 7) / 8;

  cmd.num_bytes = 0;

  put_32 (&cmd, 0x00000001); /* start sequence */
  put_32 (&cmd, 0xB00BFACE); /* #-)) */
  put_32 (&cmd, len8*8 + 32);
  put_32 (&cmd, 0x20534d4d); /* protocol type "MMS " */
  put_32 (&cmd, len8 + 4);
  put_32 (&cmd, seq_num);
  seq_num++;
  put_32 (&cmd, 0x0);        /* unknown */
  put_32 (&cmd, 0x0);
  put_32 (&cmd, len8+2);
  put_32 (&cmd, 0x00030000 | command); /* dir | command */
  put_32 (&cmd, switches);
  put_32 (&cmd, extra);

  memcpy (&cmd.buf[48], data, length);
  if (length & 7)
    memset(&cmd.buf[48 + length], 0, 8 - (length & 7));

  if (send (s, cmd.buf, len8*8+48, 0) != (len8*8+48)) {
    fprintf (stderr,"write error\n");
  }

  dprintf (_("\n***************************************************\ncommand sent, %d bytes\n"), length+48);

  dprintf (_("start sequence %08x\n"), get_32 (cmd.buf,  0));
  dprintf (_("command id     %08x\n"), get_32 (cmd.buf,  4));
  dprintf (_("length         %8x \n"), get_32 (cmd.buf,  8));
  dprintf (_("len8           %8x \n"), get_32 (cmd.buf, 16));
  dprintf (_("sequence #     %08x\n"), get_32 (cmd.buf, 20));
  dprintf (_("len8  (II)     %8x \n"), get_32 (cmd.buf, 32));
  dprintf (_("dir | comm     %08x\n"), get_32 (cmd.buf, 36));
  dprintf (_("switches       %08x\n"), get_32 (cmd.buf, 40));

  dprintf (_("ascii contents>"));
  for (i=48; i<(length+48); i+=2) {
    unsigned char c = cmd.buf[i];

    if ((c>=32) && (c<=128))
      dprintf ("%c", c);
    else
      dprintf ("<%d>", c);
  }
  dprintf ("\n");

/*   dprintf (_("complete hexdump of package follows:\n")); */
/*   for (i=0; i<(length+48); i++) { */
/*     dprintf ("%02x", cmd.buf[i]); */

/*     if ((i % 16) == 15) */
/*       dprintf ("\n"); */

/*     if ((i % 2) == 1) */
/*       dprintf (" "); */

/*   } */
/*   dprintf ("\n"); */

}

static void string_utf16(char *dest, char *src, int len) {
  int i;

  memset (dest, 0, len*2+16);

  for (i=0; i<len; i++) {
    dest[i*2] = src[i];
    dest[i*2+1] = 0;
  }

  dest[i*2] = 0;
  dest[i*2+1] = 0;
}

static void print_answer (char *data, int len) {

  int i;

  dprintf (_("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\nanswer received, %d bytes\n"), len);

  dprintf (_("start sequence %08x\n"), get_32 (data, 0));
  dprintf (_("command id     %08x\n"), get_32 (data, 4));
  dprintf (_("length         %8x \n"), get_32 (data, 8));
  dprintf (_("len8           %8x \n"), get_32 (data, 16));
  dprintf (_("sequence #     %08x\n"), get_32 (data, 20));
  dprintf (_("len8  (II)     %8x \n"), get_32 (data, 32));
  dprintf (_("dir | comm     %08x\n"), get_32 (data, 36));
  dprintf (_("switches       %08x\n"), get_32 (data, 40));

  for (i=48; i<len; i++ /*+=2*/) {
    unsigned char c = data[i];
    
    if ((c>=32) && (c<128))
      dprintf ("%c", c);
    else
      dprintf (" %02x ", c);
    
  }
  dprintf ("\n");
}  

static void get_answer (MMSContext * s) {

  char  data[BUF_SIZE];
  int   command = 0x1b;

  while (command == 0x1b) {
    int len;

    len = recv (s, data, BUF_SIZE, 0) ;
    if (!len) {
      dprintf ("\nalert! eof\n");
      return;
    }

    command = get_32 ((unsigned char *)data, 36) & 0xFFFF;

    if (command == 0x1b) 
      send_command (s, 0x1b, 0, 0, 0, data);
  }
}

static int get_data (MMSContext * s, char *buf, size_t count) {

  ssize_t  len;
  size_t total = 0;

  while (total < count) {

    len = recv (s, &buf[total], count-total, 0);

    if (len<0) {
      perror ("read error:");
      return 0;
    }

    total += len;

    if (len != 0) {
      dprintf ("[%d/%d]", total, count);
      fflush (stdout);
    }

  }

  return 1;

}

static int get_header (MMSContext * s, uint8_t *header) {

  unsigned char  pre_header[8];
  int            i, header_len;

  header_len = 0;

  while (1) {

    if (!get_data (s, (char *)pre_header, 8)) {
      fprintf (stderr,_("pre-header read failed\n"));
      return 0;
    }
    
    for (i=0; i<8; i++)
      dprintf (_("pre_header[%d] = %02x (%d)\n"),
	      i, pre_header[i], pre_header[i]);
    
    if (pre_header[4] == 0x02) {
      
      int packet_len;
      
      packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;

      dprintf (_("asf header packet detected, len=%d\n"),
	      packet_len);

      if (!get_data (s, (char *)&header[header_len], packet_len)) {
	fprintf (stderr,_("header data read failed\n"));
	return 0;
      }

      header_len += packet_len;

      if ( (header[header_len-1] == 1) && (header[header_len-2]==1)) {

	//write (output_fh, header, header_len);

	dprintf (_("get header packet finished\n"));

	return (header_len);

      } 

    } else {

      int packet_len, command;
      char data[BUF_SIZE];

      if (!get_data (s, (char *)&packet_len, 4)) {
	fprintf (stderr,_("packet_len read failed\n"));
	return 0;
      }
      
      packet_len = get_32 ((unsigned char *)&packet_len, 0) + 4;
      
      dprintf (_("command packet detected, len=%d\n"),
	      packet_len);
      
      if (!get_data (s, data, packet_len)) {
	fprintf (stderr,_("command data read failed\n"));
	return 0;
      }
      
      command = get_32 ((unsigned char *)data, 24) & 0xFFFF;
      
      dprintf (_("command: %02x\n"), command);
      
      if (command == 0x1b) 
	send_command (s, 0x1b, 0, 0, 0, data);
      
    }

    dprintf (_("get header packet succ\n"));
  }
}

int interp_header (uint8_t *header, int header_len) {

  int i;
  int packet_length = 0;

  /*
   * parse header
   */

  i = 30;
  while (i<header_len) {
    
    uint64_t  guid_1, guid_2, length;

    guid_2 = (uint64_t)header[i] | ((uint64_t)header[i+1]<<8) 
      | ((uint64_t)header[i+2]<<16) | ((uint64_t)header[i+3]<<24)
      | ((uint64_t)header[i+4]<<32) | ((uint64_t)header[i+5]<<40)
      | ((uint64_t)header[i+6]<<48) | ((uint64_t)header[i+7]<<56);
    i += 8;

    guid_1 = (uint64_t)header[i] | ((uint64_t)header[i+1]<<8) 
      | ((uint64_t)header[i+2]<<16) | ((uint64_t)header[i+3]<<24)
      | ((uint64_t)header[i+4]<<32) | ((uint64_t)header[i+5]<<40)
      | ((uint64_t)header[i+6]<<48) | ((uint64_t)header[i+7]<<56);
    i += 8;
    
    dprintf (_("guid found: %016llx%016llx\n"), guid_1, guid_2);

    length = (uint64_t)header[i] | ((uint64_t)header[i+1]<<8) 
      | ((uint64_t)header[i+2]<<16) | ((uint64_t)header[i+3]<<24)
      | ((uint64_t)header[i+4]<<32) | ((uint64_t)header[i+5]<<40)
      | ((uint64_t)header[i+6]<<48) | ((uint64_t)header[i+7]<<56);

    i += 8;

    if ( (guid_1 == 0x6cce6200aa00d9a6ll) && (guid_2 == 0x11cf668e75b22630ll) ) {
      dprintf (_("header object\n"));
    } else if ((guid_1 == 0x6cce6200aa00d9a6ll) && (guid_2 == 0x11cf668e75b22636ll)) {
      dprintf (_("data object\n"));
    } else if ((guid_1 == 0x6553200cc000e48ell) && (guid_2 == 0x11cfa9478cabdca1ll)) {

      packet_length = get_32(header, i+92-24);

      dprintf (_("file object, packet length = %d (%d)\n"),
	      packet_length, get_32(header, i+96-24));


    } else if ((guid_1 == 0x6553200cc000e68ell) && (guid_2 == 0x11cfa9b7b7dc0791ll)) {

      int stream_id = header[i+48] | header[i+49] << 8;

      dprintf (_("stream object, stream id: %d\n"), stream_id);

      stream_ids[num_stream_ids] = stream_id;
      num_stream_ids++;
      

      /*
	} else if ((guid_1 == 0x) && (guid_2 == 0x)) {
	dprintf ("??? object\n");
      */
    } else {
      dprintf (_("unknown object\n"));
    }

    dprintf (_("length    : %lld\n"), length);

    i += length-24;

  }

  return packet_length;

}


static int get_media_packet (MMSContext * s, int padding, char * data) {

  unsigned char  pre_header[8];
  int            i;

# define dprintf(x...) printf(x)
  if (!get_data (s, (char *)pre_header, 8)) {
    fprintf (stderr,_("pre-header read failed\n"));
    return -1;
  }

/*   for (i=0; i<8; i++) */
/*     dprintf (_("pre_header[%d] = %02x (%d)\n"), */
/* 	    i, pre_header[i], pre_header[i]); */

  if (pre_header[4] == 0x04) {

    int packet_length;

    packet_length = (pre_header[7] << 8 | pre_header[6]) - 8;

    if (packet_length > s->packet_length) {
      fprintf (stderr,_("packet_length > s->packet_length !!\n"));
      fs_seek(s, packet_length - s->packet_length, SEEK_CUR);
      packet_length = s->packet_length;
    }

/*     dprintf (_("asf media packet detected, len=%d\n"), */
/* 	    packet_length); */

    if (!get_data (s, data, packet_length)) {
      fprintf (stderr,_("media data read failed\n"));
      return -1;
    }

    //write (output_fh, data, padding);
    memset(data+packet_length, 0, s->packet_length - packet_length);

    return packet_length;

  } else {

    int packet_len, command;

    if ( (pre_header[7] != 0xb0) || (pre_header[6] != 0x0b)
	 || (pre_header[5] != 0xfa) || (pre_header[4] != 0xce) ) {

      dprintf (_("missing signature\n"));
      return -1;

    }

    if (!get_data (s, (char *)&packet_len, 4)) {
      dprintf (_("packet_len read failed\n"));
      return -1;
    }

    packet_len = get_32 ((unsigned char *)&packet_len, 0) + 4;

    dprintf (_("command packet detected, len=%d\n"),
	    packet_len);

    if (!get_data (s, data, packet_len)) {
      fprintf (stderr,_("command data read failed\n"));
      return -1;
    }

    command = get_32 ((unsigned char *)data, 24) & 0xFFFF;

    dprintf (_("command: %02x\n"), command);

    if (command == 0x1b) 
      send_command (s, 0x1b, 0, 0, 0, data);
    else if (command == 0x1e) {

      printf (_("everything done.\n"));

      return 0;
    } else if (command != 0x05) {
      dprintf (_("unknown command %02x\n"), command);
      return 0;
    }
  }
#define dprintf(...) (void) 0
  dprintf (_("get media packet succ\n"));

  return 0;
}

#if 0
int main (int argc, char **argv) {

  int                  s ;
  struct sockaddr_in   sa;
  struct hostent      *hp;
  static char                 str[1024];
  static char                 data[1024];
  uint8_t              asf_header[8192];
  int                  asf_header_len;
  int                  len, i, packet_length;
  char                 host[256];
  char                *path, *url, *file, *cp;

  if (argc != 2 || 
      (argc == 1 &&
       (!strncmp("--h", argv[1], 3) ||
	!strncmp("-h", argv[1], 2))) ||
      (argc == 2 && strncmp("mms://", argv[1], 6))
      ) {
    fprintf (stderr,_("usage: %s mms://<host>[:port]\n"), argv[0]);
    exit(1);
  }

  /* parse url */
  
  url = argv[1];
  strncpy (host, &url[6], 255);
  cp = strchr(host,'/');
  if (cp) *cp= 0;

  fprintf (stderr,_("host : >%s<\n"), host);

  path = strchr(&url[6], '/');
  if (!path) path = "/invalid";
  path += 1;

  fprintf (stderr,_("path : >%s<\n"), path);

  file = strrchr (url, '/');
  if (!file) file = "invalid";

  fprintf (stderr,_("file : >%s<\n"), file);

  output_fh = open (&file[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (output_fh<0) {
    fprintf (stderr,_("cannot create output file '%s'.\n"),
	    &file[1]);
    exit(1);
  }

  fprintf (stderr,_("creating output file '%s'\n"), &file[1]);

  /* DNS lookup */

  if ((hp = gethostbyname(host)) == NULL) {
    fprintf(stderr,_("Host name lookup failure.\n"));
    return 1 ;
  }

  /* fill socket structure */

  bcopy ((char *) hp->h_addr, (char *) &sa.sin_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;
  /*sa.sin_port = 0x5000;*/ /* http port (80 => 50 Hex, switch Hi-/Lo-Word ) */

  sa.sin_port = htons(1755) ; /* be2me_16(1755);  mms port 1755 */

  fprintf (stderr,_("port: %08x\n"), sa.sin_port);

  /* open socket */

  if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0))<0) {
    perror (_("socket"));
    return  1 ;
  }

  dprintf (_("socket open\n"));

  /* try to connect */

  if (connect (s, (struct sockaddr *)&sa, sizeof sa)<0) {
    perror (_("request"));
    return(1);
  }

  fprintf (stderr,_("connected\n"));

  /* cmd1 */
  uuid_t client_uuid;
  uuid_generate(client_uuid);

  char uuid_string[37];
  uuid_unparse(client_uuid, uuid_string);
  
  sprintf (str,
	   "\034\003NSPlayer/9.0.0.2800; "
	   "{%s}; "
	   "Host: %s", uuid_string, host);
  string_utf16 (data, str, strlen(str)+2);

  send_command (s, 1, 0, 0x0004000b, strlen(str) * 2+8, data);

  len = read (s, data, BUF_SIZE) ;
  if (len)
    print_answer (data, len);
  
  /* cmd2 */

  string_utf16 (&data[8], "\002\000\\\\192.168.0.129\\TCP\\1037\0000", 
		28);
  memset (data, 0, 8);
  send_command (s, 2, 0, 0, 28*2+8, data);

  len = read (s, data, BUF_SIZE) ;
  if (len)
    print_answer (data, len);

  /* 0x5 */

  string_utf16 (&data[8], path, strlen(path));
  memset (data, 0, 8);
  send_command (s, 5, 0, 0, strlen(path)*2+12, data);

  get_answer (s);

  /* 0x15 */

  memset (data, 0, 40);
  data[32] = 2;

  send_command (s, 0x15, 1, 0, 40, data);

  num_stream_ids = 0;
  /* get_headers(s, asf_header);  */

  asf_header_len = get_header (s, asf_header);
  packet_length = interp_header (asf_header, asf_header_len);

  /* 0x33 */

  memset (data, 0, 40);

  for (i=1; i<num_stream_ids; i++) {
    data [ (i-1) * 6 + 2 ] = 0xFF;
    data [ (i-1) * 6 + 3 ] = 0xFF;
    data [ (i-1) * 6 + 4 ] = stream_ids[i];
    data [ (i-1) * 6 + 5 ] = 0x00;
  }

  send_command (s, 0x33, num_stream_ids, 
		0xFFFF | stream_ids[0] << 16, 
		(num_stream_ids-1)*6+2 , data);

  get_answer (s);

  /* 0x07 */

  memset (data, 0, 40);

  for (i=8; i<16; i++)
    data[i] = 0xFF;

  data[20] = 0x04;

  send_command (s, 0x07, 1, 
		0xFFFF | stream_ids[0] << 16, 
		24, data);



  while (get_media_packet(s, packet_length)) {
    fprintf(stderr,".");
    /* nothing */

  }

  close (output_fh);
  close (s);

  return 0;
}
#endif



static int mms_connect(URLContext *h, const char *path, const char *hoststr);
static int mms_write(URLContext *h, uint8_t *buf, int size);


/* return non zero if error */
static int mms_open(URLContext *h, const char *uri, int flags)
{
    const char *path, *proxy_path;
    char hostname[256], hoststr[256];
    char path1[256];
    char buf[256];
    int port, use_proxy, err;
    MMSContext *s;
    file_t hd = -1;

    h->is_streamed = 1;

    s = av_malloc(sizeof(MMSContext));
    if (!s) {
        return -ENOMEM;
    }
    h->priv_data = s;

    /* fill the dest addr */
    /* needed in any case to build the host string */
    url_split(NULL, 0, hostname, sizeof(hostname), &port, 
              path1, sizeof(path1), uri);
    if (port > 0) {
        snprintf(hoststr, sizeof(hoststr), "%s:%d", hostname, port);
    } else {
        pstrcpy(hoststr, sizeof(hoststr), hostname);
    }

    if (path1[0] == '\0')
      path = "/";
    else
      path = path1;

    if (port < 0)
        port = 1755;

    //snprintf(buf, sizeof(buf), "tcp://%s:%d", hostname, port);
    snprintf(buf, sizeof(buf), "/tcp/%s:%d", hostname, port);
    hd = fs_open(buf, O_RDWR);
    if (hd < 0)
        goto fail;

    s->hd = hd;
    s->pos = -1;
    if (mms_connect(h, path, hoststr) < 0)
        goto fail;

    return 0;
 fail:
    if (hd >= 0)
      fs_close(hd);
    av_free(s);
    return AVERROR_IO;
}

static char          str[256];
static char          data[1024];
static int mms_connect(URLContext *h, const char *path, const char *hoststr)
{
  MMSContext *s = h->priv_data;

  int                  len, i;

  if (path && *path)
    path++;

  /* cmd1 */
  char uuid_string[37];
  uuid_string[36] = 0;
  srand((int)timer_ms_gettime64());
  for (i=0; i<36; i++)
    uuid_string[i] = '0' + (random()%10);

  /* format : d07e254f-7589-4faf-ae72-35509fd5cf6c */
  /* sample : 58167496-0121-9832-6147-501470361450 */
  uuid_string[8] = '-';
  uuid_string[13] = '-';
  uuid_string[18] = '-';
  uuid_string[23] = '-';
  uuid_string[23] = '-';

  //strcpy(uuid_string, "bfc13d1d-3da7-4529-8c5a-15b6e2f8050f");

  printf("uuid = '%s'\n", uuid_string);
  
  sprintf (str,
	   "\034\003NSPlayer/9.0.0.2800; "
	   "{%s}; "
	   "Host: %s", uuid_string, hoststr);

  dprintf("hdr -->\n%s", str);

  string_utf16 (data, str, strlen(str)+2);

  send_command (s, 1, 0, 0x0004000b, strlen(str) * 2+8, data);

  len = read (s, data, BUF_SIZE) ;
  if (len > 0)
    print_answer (data, len);
  else
    return -1;
  
  /* cmd2 */

  string_utf16 (&data[8], "\002\000\\\\192.168.0.129\\TCP\\1037\0000", 
		28);
  memset (data, 0, 8);
  send_command (s, 2, 0, 0, 28*2+8, data);

  len = read (s, data, BUF_SIZE) ;
  if (len > 0)
    print_answer (data, len);
  else
    return -1;

  /* 0x5 */

  string_utf16 (&data[8], path, strlen(path));
  memset (data, 0, 8);
  send_command (s, 5, 0, 0, strlen(path)*2+12, data);

  get_answer (s);

  /* 0x15 */

  memset (data, 0, 40);
  data[32] = 2;

  send_command (s, 0x15, 1, 0, 40, data);

  num_stream_ids = 0;
  /* get_headers(s, asf_header);  */

  s->asf_header_len = get_header (s, s->asf_header);
  s->packet_length = interp_header (s->asf_header, s->asf_header_len);

  /* 0x33 */

  memset (data, 0, 40);

  for (i=1; i<num_stream_ids; i++) {
    data [ (i-1) * 6 + 2 ] = 0xFF;
    data [ (i-1) * 6 + 3 ] = 0xFF;
    data [ (i-1) * 6 + 4 ] = stream_ids[i];
    data [ (i-1) * 6 + 5 ] = 0x00;
  }

  send_command (s, 0x33, num_stream_ids, 
		0xFFFF | stream_ids[0] << 16, 
		(num_stream_ids-1)*6+2 , data);

  get_answer (s);

  /* 0x07 */

  memset (data, 0, 40);

  for (i=8; i<16; i++)
    data[i] = 0xFF;

  data[20] = 0x04;

  send_command (s, 0x07, 1, 
		0xFFFF | stream_ids[0] << 16, 
		24, data);


  if (s->asf_header_len >= s->packet_length)
    s->buffer = s->asf_header;
  else
    s->buffer = av_malloc(s->packet_length);

  //assert(asf_header_len <= s->packet_length);
  printf("s->packet_length = %d\n", s->packet_length);
  printf("s->asf_header_len = %d\n", s->asf_header_len);
  s->hdr = 0;

  return 0;
}


static int mms_read(URLContext *h, uint8_t *buf, int size)
{
  MMSContext *s = h->priv_data;
  int len = 0;

  while (size > 0) {
    int rsize;

    if (s->hdr < s->asf_header_len) {
      if (size < s->asf_header_len - s->hdr)
	rsize = size;
      else
	rsize = s->asf_header_len - s->hdr;

      memcpy(buf, s->asf_header+s->hdr, rsize);
      s->hdr += rsize;
      len += rsize;
      buf += rsize;
      size -= rsize;

      if (size <= 0)
	return len;
    }

    if (s->pos < 0 || s->pos >= s->packet_length) {
      int res = 0;
      while ( !res ) {
	res = get_media_packet(s, s->packet_length, s->buffer);
	//printf("get_media_packet --> %d\n", res);
      }
      if (res < 0)
	return -1;
      s->pos = 0;
    }

    if (size < s->packet_length - s->pos)
      rsize = size;
    else
      rsize = s->packet_length - s->pos;

    memcpy(buf, s->buffer+s->pos, rsize);
    s->pos += rsize;
    len += rsize;
    buf += rsize;
    size -= rsize;
  }

  return len;
}

/* used only when posting data */
static int mms_write(URLContext *h, uint8_t *buf, int size)
{
    MMSContext *s = h->priv_data;
    return fs_write(s->hd, buf, size);
}

static int mms_close(URLContext *h)
{
    MMSContext *s = h->priv_data;
    fs_close(s->hd);
    if (s->buffer != s->asf_header)
      av_free(s->buffer);
    av_free(s);
    return 0;
}


URLProtocol mms_protocol = {
    "mms",
    mms_open,
    mms_read,
    mms_write,
    NULL, /* seek */
    mms_close,
};

