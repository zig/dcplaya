/* KallistiOS Ogg/Vorbis Decoder Library
 *
 * sndvorbisfile.h
 * (c)2001 Thorsten Titze
 *
 * Basic Ogg/Vorbis stream information and decoding routines used by
 * libsndoggvorbis. May also be used directly for playback without
 * threading.
 */

#include <kos.h>

/* Typedefinition for Header Information of Ogg-Vorbis Files
 * Not including Comments which are stored in a VorbisFile_info_t
 */
typedef struct
{
  int     channels;
  int     frequency;
  char    *vendor;
  int     convsize;
  int     bitrate;
  unsigned int bytes;
} VorbisFile_headers_t;

/* Typedefinition for File Information Following the Ogg-Vorbis Spec
 *
 * TITLE :Track name
 * VERSION :The version field may be used to differentiate multiple version of the
 *         same track title in a single collection. (e.g. remix info)
 * ALBUM :The collection name to which this track belongs
 * TRACKNUMBER :The track number of this piece if part of a specific larger collection
 *              or album
 * ARTIST :Track performer
 * ORGANIZATION :Name of the organization producing the track (i.e. the 'record label')
 * DESCRIPTION :A short text description of the contents
 * GENRE :A short text indication of music genre
 * DATE :Date the track was recorded
 * LOCATION :Location where track was recorded
 * COPYRIGHT :Copyright information
 * ISRC :ISRC number for the track; see {the ISRC intro page} for more information on
 *       ISRC numbers.
 *
 * (Based on v-comment.html found in original OggVorbis packages)
 */
typedef struct
{
  char    *artist;
  char    *title;
  char    *version;
  char    *album;
  char    *tracknumber;
  char    *organization;
  char    *description;
  char    *genre;
  char    *date;
  char    *location;
  char    *copyright;
  char    *isrc;
 
  char    *filename;
 
  long    nominalbitrate;
  long    actualbitrate;
  long    actualposition;
} VorbisFile_info_t;

typedef uint32		VorbisFile_handle_t;

/* Static String that is used in case a comment is not
 * set in a Stream
 */
//static char VorbisFile_NULL[]="N/A";
#define VorbisFile_NULL 0

/* Public Function Declarations
 */
long VorbisFile_getBitrateNominal();
long VorbisFile_getBitrateInstant();

char *VorbisFile_getCommentByName(char *commentfield);
char *VorbisFile_getCommentByID(long commentid);

int VorbisFile_openFile(const char *filename, VorbisFile_headers_t *v_headers);
void VorbisFile_closeFile();

int VorbisFile_decodePCMint8(VorbisFile_headers_t vhd, uint8 *target, int requested);
int VorbisFile_decodePCM(VorbisFile_headers_t vhd, ogg_int16_t * target, int requested);

int VorbisFile_isEOS();

