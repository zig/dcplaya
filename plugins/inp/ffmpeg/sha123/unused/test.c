#include <stdio.h>
#include <string.h>
#include "sha123/api.h"

#include "istream/istream_file.h"

int main(int na, char **a)
{
  int err, info;
  const char * fname;
  istream_t * isf;
  sha123_param_t param;
  sha123_t * sha123;

  fprintf(stderr, "Initialising sha123...\n");
  err = sha123_init();
  fprintf(stderr, " -> %d\n",err);

  fname = na > 1 ? a[1] : 0;
  info = 0;
  if (fname && !strcmp(fname,"--info")) {
    info = 1;
    fname = na > 2 ? a[2] : 0;
  }
  fprintf(stderr, "Creating stream [%s]\n", fname);
  isf = istream_file_create(fname, 1);
  fprintf(stderr, "-> %p [%s]\n", isf, istream_filename(isf));

  param.istream = isf;
  param.loop = 1;
  param.bsi.buffer = 0;
  param.bsi.size = 0;

  fprintf(stderr, "Starting [%s]\n", fname);
  sha123 = sha123_start(&param);
  fprintf(stderr, "->%p\n", sha123);
  err = -!sha123;

  if (info) {
    const sha123_info_t * inf = sha123_info(sha123);

    if (inf) {
      printf("SHA123_LAYER=%d\n"
	     "SHA123_FRQ=%d\n"
	     "SHA123_CHAN=%d\n",
	     inf->layer,
	     inf->sampling_rate,
	     inf->channels
	     );
    }
  } else {
    while ( !sha123_decode(sha123,0,0) )
      ;
  }

  fprintf(stderr, "Stop\n");
  sha123_stop(sha123);

  fprintf(stderr, "Shutdown sha123...\n");
  sha123_shutdown();
  fprintf(stderr, " -> shutdowned\n");

  return err;
}

