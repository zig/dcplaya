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

static int do_gz(const char *zname, const char *iname, int size)
{
  char tmp[256];
  FILE * in;
  gzFile out;

/*   fprintf(stderr,"compress [%s] <= [%s] %d\n", zname, iname, size); */

  in = fopen(iname,"rb");
  if (!in) {
    perror(iname);
    return -1;
  }
  out = gzopen(zname,"wb9");
  if (!out) {
    fclose(in);
    perror("gzopen");
    return -1;
  }

  while(size) {
    int n;

    n = sizeof(tmp);
    if (n > size) n = size;

    if (fread(tmp,1,n,in) != n) {
      perror(iname);
      break;
    }
    if (gzwrite(out,tmp,n) != n) {
      perror("gzwrite");
      break;
    }
    size -= n;
  }
  gzflush(out, Z_SYNC_FLUSH);
  gzclose(out);
  fclose(in);

  if (size) {
    size = -1;
  } else {
    in = fopen(zname,"rb");
    if (!in) {
      perror(zname);
      size = -1;
    } else {
      fseek(in,0,SEEK_END);
      size = ftell(in);
      fclose(in);
    }
  }
  return size;
}

static int fappend(FILE * out, const char *iname, int size)
{
  char tmp[256];
  FILE * in;
  int pad = 0; //-size & 3;

  in = fopen(iname, "rb");
  if (!in) {
    return -1;
  }

  while(size) {
    int n;

    n = sizeof(tmp);
    if (n > size) n = size;

    if (fread(tmp,1,n,in) != n) {
      perror("read error");
      break;
    }
    if (fwrite(tmp,1,n,out) != n) {
      perror(tmp);
      break;
    }
    size -= n;
  }

  memset(tmp,0,pad);
  if(fwrite(tmp,1,pad,out) != pad) {
    size = pad;
  }

  fclose(in);
  return -!!size;
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
    const char * tname = "tmp.z"; /* $$$ fix me */
    int namelen = strlen(name)+1;
    if (verbose) fprintf(stderr, "SOURCE \"%s\"(%d) %d\n",
			 name, namelen, e->size);

    if (e->next) {
      if (verbose) fprintf(stderr, "COMPRESS\n");
      e->clen = do_gz(tname, e->name, e->size);
      if (e->clen < 0) {
	unlink(tname);
	perror("compress");
	goto error;
      }
      head.clen += e->clen;
      if (verbose) fprintf(stderr, "COMPRESS %d / %d / %d%%\n",
			   e->size,e->clen, e->clen*100/e->size);
    }
    
    if (verbose) fprintf(stderr, "DIR \"%s\" %d/%d %d]\n",
			 name, e->size, e->clen, namelen);
    if (namelen > 32) {
      unlink(tname);
      perror("name too long");
      goto error;
    }
    fputint(out, namelen, 1);
    fputint(out, e->size, 3);
    fputint(out, e->clen, 3);

    if (fwrite(name, 1, namelen, out) != namelen) {
      unlink(tname);
      perror(e->name);
      goto error;
    }
    if (e->next) {
      if (verbose) fprintf(stderr, "DATA\n");
      if (fappend(out, tname, e->clen) < 0) {
	unlink(tname);
	perror(e->name);
	goto error;
      }
      unlink(tname);
    } else {
      if (verbose) fprintf(stderr, "TOTAL %d / %d / %d%%\n",
			   e->size,e->clen, e->clen*100/e->size);
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
