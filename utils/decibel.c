/**
 *  @file    decibel.c
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @brief   decibal table calculation
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define LN_10_OVER_10   0.230258509299
#define _10_OVER_LN_10  4.34294481904

/* Db from [0..-oo[
 * ->      [1..0[
 */
static double db2lin(double db)
{
  return exp(db * LN_10_OVER_10);
}


/* V from ]0..1]
 * ->     ]-oo..0] 
 */
static double lin2db(double v)
{
  return _10_OVER_LN_10 * log(v);
}

static int usage(void)
{
  printf(
		 "decibel [options] [file]\n"
		 "options:\n"
		 " -n#  : Number of entry (default=4096)\n"
		 " -s#  : Scaling factor (default=32767)\n"
		 " -i   : Produce decibel to linear table\n"
		 "\n"
		 );
}

static void lin2db_table(FILE * out, int n, int s)
{
  int i;
  double v,db,stp;
  double dbmax;
  double sc;

  stp = 1.0 / (n+1);
  dbmax = -lin2db(stp); /* */
  sc = (double) s / dbmax;

  for (i=0, v=stp; i<n; ++i, v+=stp) {
	int r;
	db = (i==n-1) ? 0 : lin2db(v);
	r = ( (dbmax + db) * sc);
	fprintf(out,"%lf %d,\n", db, r);
  }

}

static void db2lin_table(FILE * out, int n, int s)
{
}

int main(int na, char **a)
{
  int i;
  const char * fname = 0;
  FILE * out = stdout;
  int n = 4096;
  int s = 32767;
  int action = 0;

  for (i = 1; i<na; ++i) {
	if (!strcmp(a[i],"--help")) {
	  return usage();
	}

	if (a[i][0] == '-') {
	  switch(a[i][1]) {
	  case 'n':
		n = strtol(a[i]+2,0,0);
		break;
	  case 's':
		s = strtol(a[i]+2,0,0);
		break;
	  case 'i':
		action = 1;
		break;
	  }
	} else {
	  fname = a[i];
	}
  }

  if (fname) {
	out = fopen(fname,"wt");
	if (!out) {
	  perror(fname);
	  return 2;
	}
  }

  if (action) {
	db2lin_table(out,n,s);
  } else {
	lin2db_table(out,n,s);
  }

  if (out != stdout) {
	fclose(out);
  }

  return 0;
}
