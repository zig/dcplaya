/* KallistiOS Ogg/Vorbis Decoder Library
 *
 * sndoggvorbis.h
 * (c)2001 Thorsten Titze
 *
 * An Ogg/Vorbis player library using sndstream and the official Xiphophorus
 * libogg and libvorbis libraries.
 * 
 * last mod: $Id: sndoggvorbis.h,v 1.1 2002-08-26 14:14:35 ben Exp $
 */

int sndoggvorbis_init();
int sndoggvorbis_start(char *filename,int loop);
void sndoggvorbis_stop();
void sndoggvorbis_shutdown();

int sndoggvorbis_isplaying();

void sndoggvorbis_volume(int vol);

void sndoggvorbis_mainloop();
void sndoggvorbis_wait_start();

void sndoggvorbis_setbitrateinterval(int interval);
long sndoggvorbis_getbitrate();
long sndoggvorbis_getposition();

char *sndoggvorbis_getcommentbyname(char *commentfield);
char *sndoggvorbis_getartist();
char *sndoggvorbis_gettitle();
char *sndoggvorbis_getgenre();
