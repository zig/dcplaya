/* define this if you don't want FILE support. */
#ifndef ISTREAM_NO_FILE

#include <stdio.h>
#include <string.h>

#include "istream/istream_def.h"
#include "istream/alloc.h"

/** istream file structure. */
typedef struct {
  istream_t istream; /**< istream function. */
  FILE *f;           /**< FILE handle.      */

  /* Open modes. */
  struct {
    unsigned int read:1;   /**< open for reading.  */
    unsigned int write:1;  /**< open for writing.  */
  } mode;

  /* MUST BE at the end of the structure because supplemental bytes will
   * be allocated to store filename.
   */
  char name[1];       /**< filename. */

} istream_file_t;

static const char * name(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->name[0])
    ? 0
    : isf->name;
}

static int open(istream_t * istream)
{
  int imode;
  char mode[8];
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->name) {
    return -1;
  }

  imode = 0;
  if (isf->mode.read) {
    mode[imode++] = 'r';
  }
  if (isf->mode.write) {
    mode[imode++] = !imode ? 'w' : '+';
  }
  if (!imode) {
    /* No open mode.. */
    return -1;
  }
  mode[imode++] = 'b';
  mode[imode] = 0;

  isf->f = fopen(isf->name, mode);
  return isf->f ? 0 : -1; 
}

static int close(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;
  int err;

  if (!isf) {
    return -1;
  }
  err = isf->f ? fclose(isf->f) : -1;
  isf->f = 0;
  return err;
}

static int read(istream_t * istream, void * data, int n)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f || !isf->mode.read)
    ? -1
    : fread(data, 1, n, isf->f);
}

static int write(istream_t * istream, void * data, int n)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f || !isf->mode.write)
    ? -1
    : fwrite(data, 1, n, isf->f);
}


/* We could have store the length value at opening, but this way it handles
 * dynamic changes of file size.
 */
static int length(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;
  int pos,len;

  if (!isf || !isf->f) {
    return -1;
  }
  /* save current position. */
  len = ftell(isf->f);
  if (len != -1) {
    pos = len;
    /* seek t end of file */
    len = fseek(isf->f, 0, SEEK_END);
    if (len != -1) {
      /* get end of file position (AKA file length) */
      len = ftell(isf->f);
      /* restore saved position. ( $$$ no error check here ) */
      fseek(isf->f, pos, SEEK_SET);
    }
  }
  return len;
}

static int tell(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f)
    ? -1 
    : ftell(isf->f);
}

static int seek(istream_t * istream, int offset)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->f)
    ? -1
    : fseek(isf->f, offset, SEEK_CUR);
}

static const istream_t istream_file = {
  name, open, close, read, write, length, tell, seek, seek 
};

istream_t * istream_file_create(const char * fname, int mode)
{
  istream_file_t *isf;
  int len;

  if (!fname || !fname[0]) {
    return 0;
  }

  /* Don't need 0, because 1 byte already allocated in the
   * istream_file_t::fname.
   */
  len = strlen(fname);
  isf = istream_alloc(sizeof(istream_file_t) + len);
  if (isf) {
    /* Copy istream functions. */
    memcpy(&isf->istream, &istream_file, sizeof(istream_file));
    /* Clean file handle. */
    isf->f = 0;
    isf->mode.read = mode;
    isf->mode.write = mode>>1;

    /* Copy filename. */
    /* $$$ May be later, we should add a check for relative path and add
     * CWD ... */
    strcpy(isf->name, fname);
  }
  return &isf->istream;
}

#else /* #ifndef ISTREAM_NO_FILE */

/* istream file must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

istream_t * istream_file_create(const char * fname, int mode)
{
  return 0;
}

#endif /* #ifndef ISTREAM_NO_FILE */
