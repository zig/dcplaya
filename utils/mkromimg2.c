/**
 *  @file    decibel.c
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @brief   make new format romdisk image
 *  @date    2003/03/03
 *
 *  romdisk image format : 
 *
 *  - 'RD-BEN' (identifier)
 *  - nb_entries (16-bit-LE)
 *  - for each entry:
 *        - 8-bit name-length (includes trailing '\0', max 32)
 *        - 24-MSB file-size (uncompressed file size)
 *        - 24-MSB data-size (compressed file size)
 *     - name-data
 *     - file-data
 *  - supplemental entry : same format but
 *     - name is always '.'
 *     - file-size is total disk size.
 *     - data-size is -1
 *     - NO DATA !
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

typedef struct _entry {
  int size;
  int clen;
  const char * name;
  struct _entry *next;
} fentry_t;

static const char * basename(const char *fname)
{
  const char *s = strrchr(fname,'/');
  return s ? s+1 : fname;
}

static void usage(const char *name)
{
  printf("%s : <rom-img> [input-files ...]\n"
	 "(c) 2003 benjamin gerard\n\n", name);
  exit(1);
}

static int fputint(FILE *out, int v, int size)
{
  while(size--) {
    if (fputc(v&255,out) == EOF) return EOF;
    v = v>>8;
  }
  return 0;
}


/* ===========================================================================
 * Test deflate() with small buffers
 */
static int do_deflate(FILE * out, const char *iname, ulong len)
{
  FILE * in = 0;
  Byte tmpIn[1024];
  Byte tmpOut[1024];
  z_stream c_stream; /* compression stream */
  int err;
  int total_out = 0;
  int r,w;
  int rtotal,wtotal;
  int inited = 0;

  in = fopen(iname, "rb");
  if (!in) {
    return -1;
  }

  c_stream.zalloc = (alloc_func)0;
  c_stream.zfree = (free_func)0;
  c_stream.opaque = (voidpf)0;

  err = deflateInit(&c_stream, Z_BEST_COMPRESSION);
  if (err != Z_OK) {
    fprintf(stderr, "error : deflate init := [%d]\n",err);
    goto error;
  }
  inited = 1;

  c_stream.avail_in = 0;
  wtotal = rtotal = 0;

  do {
    r = sizeof(tmpIn) - c_stream.avail_in;
    memmove(tmpIn, c_stream.next_in, c_stream.avail_in);
    r = fread(tmpIn + c_stream.avail_in, 1, r, in);
    if (r == -1) {
      perror("deflate input read");
      goto error;
    }
    rtotal += r;
    c_stream.next_in   = tmpIn;
    c_stream.avail_in  += r;
    c_stream.next_out  = tmpOut;
    c_stream.avail_out = sizeof(tmpOut);

    err = deflate(&c_stream, r ? Z_NO_FLUSH : Z_FINISH);
    if (err < 0) {
      fprintf(stderr, "error : deflate error := [%d]\n",err);
      goto error;
    }
    w = sizeof(tmpOut) - c_stream.avail_out;
/*     printf("total out: %d\n" */
/* 	   "available: %d\n" */
/* 	   "written:   %d\n", */
/* 	   c_stream.total_out, c_stream.avail_out, w); */
    if (fwrite(tmpOut, 1, w, out) != w) {
      fprintf(stderr, "error : writing deflate data\n");
      goto error;
    }
    wtotal += w;
  } while (err != Z_STREAM_END);

  if (c_stream.avail_in) {
    fprintf(stderr, "error : deflate unexecped available input [%d]\n",
	    c_stream.avail_in);
    goto error;
  }

  err = deflateEnd(&c_stream);
  if (err != Z_OK) {
    fprintf(stderr, "warning : deflate end := [%d]\n",err);
  }
  inited = 0;

  if (rtotal != len || rtotal != c_stream.total_in) {
    fprintf(stderr,
	    "deflate unexpected input size "
	    "[expected:%d read:%d deflate:%d]\n",
	    len, rtotal, c_stream.total_in);
    goto error;
  }

  if (wtotal != c_stream.total_out) {
    fprintf(stderr,
	    "deflate unexpected output size "
	    "[write:%d deflate:%d]\n",
	    wtotal, c_stream.total_in);
    goto error;
  }
/*   printf("expected in  : %d\n", len); */
/*   printf("total in     : %d\n", c_stream.total_in); */
/*   printf("total out    : %d\n", c_stream.total_out); */

  fclose(in);
  return c_stream.total_out;

 error:
    if (inited) deflateEnd(&c_stream);
    if (in) fclose(in);
    return -1;
}

int main(int na, char **a)
{
  const char * oname = 0;
  FILE *out = 0;
  int err = -1;
  fentry_t head, *phead, *e;
  int i, cnt;
  int verbose = 0;

  if (na < 2) {
    usage(a[0]);
  }

  oname = a[1];
  if (oname[0] == '-' && !oname[1]) {
    oname = 0;
    out = stdout;
  } else {
    oname = a[1];
    /* Do this before input parse, prevent from being taken as imput !! */
    unlink(oname);
  }

  phead = &head;
  memset(phead,0,sizeof(*phead));
  head.name = ".";

  for (cnt=0, i=2; i<na; ++i) {
    const char *iname = a[i];
    FILE *in;
    size_t len;
    
    in = fopen(iname, "rb");
    if (!in) {
      perror(iname);
      goto error;
    }

    fseek(in,0,SEEK_END);
    len = ftell(in);
    fclose(in);

    if (len == -1) {
      perror(iname);
      goto error;
    }
    if (len > 0) {
      e = malloc(sizeof(*e));
      if (!e) {
	perror(iname);
	goto error;
      }
      e->next = phead;
      phead = e;
      e->size = len;
      e->name = iname;
      head.size += len;
      ++cnt;
      if (verbose) fprintf(stderr, "scaning [%s] %d\n",e->name,e->size);
    }
  }

  if (verbose) fprintf(stderr, "%d files, %d bytes\n",cnt,head.size);

  if (!oname) {
    oname = "<stdout>";
    out = stdout;
  } else {
    out = fopen(oname,"wb");
  }
  if (!out) {
    perror(oname);
    goto error;
  }
  if (verbose) fprintf(stderr, "output to [%s]\n", oname);

  if (verbose) fprintf(stderr, "directory header\n");
  fputs("RD-BEN",out);
  fputint(out,cnt,2);

  for (e=phead; e; e = e->next) {
    const char * name = basename(e->name);
    int namelen = strlen(name)+1;
    int pos;
    if (verbose) fprintf(stderr, "SOURCE \"%s\"(%d) %d\n",
			 name, namelen, e->size);
    
    if (namelen > 32) {
      perror("name too long");
      goto error;
    }

    fputint(out, namelen, 1);
    fputint(out, e->size, 3);
    pos = ftell(out);
    fputint(out, e->clen, 3);
    if (fwrite(name,1,namelen,out) != namelen) {
      perror(e->name);
      goto error;
    }

    if (e->next) {
      e->clen = do_deflate(out, e->name, e->size);
      if (e->clen < 0) {
	goto error;
      }
      if (verbose) fprintf(stderr, "DIR \"%s\"(%d) %d / %d / %d%%]\n",
			   name, namelen,
			   e->size, e->clen, e->clen*100/e->size);
      fseek(out,pos,SEEK_SET);
      fputint(out, e->clen, 3);
      fseek(out,0,SEEK_END);
      head.clen += e->clen;
    } else {
      if (verbose) fprintf(stderr, "TOTAL %d / %d / %d%%\n",
			   e->size, e->clen, e->clen*100/e->size);
    }
  }
  err = 0;

 error:
  if (out && out != stdout) {
    fclose(out);
    if (err && oname) {
      if (verbose) fprintf(stderr, "removing [%s]\n", oname);
      unlink(oname);
    }
  }

  return err;
}
