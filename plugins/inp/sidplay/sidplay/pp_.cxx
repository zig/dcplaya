//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/pp_.cpp,v
//
// For more info get ``depp.cpp'' and ``ppcl.cpp''.

extern "C" {
# include <string.h>
# include <stdlib.h>
}

#include "pp_.h"


// exports
//bool depp(ifstream& source, ubyte** destRef);

bool ppIsCompressed();
udword ppUncompressedLen();  // return the length of the uncompressed data
const char* ppErrorString;

static const char PP_ID[] = "PP20";

static const udword PP_BITS_FAST = 0x09090909;
static const udword PP_BITS_MEDIOCRE = 0x090a0a0a;
static const udword PP_BITS_GOOD = 0x090a0b0b;
static const udword PP_BITS_VERYGOOD = 0x090a0c0c;
static const udword PP_BITS_BEST = 0x090a0c0d;

static const char text_packeddatacorrupt[]	= "PowerPacker: Packed data is corrupt";
static const char text_unrecognized[]	= "PowerPacker: Unrecognized compression method";
static const char text_uncompressed[] = "Not compressed with PowerPacker (PP20)";
static const char text_notenoughmemory[] = "Not enough free memory";
static const char text_fast[] = "PowerPacker: fast compression";
static const char text_mediocre[] = "PowerPacker: mediocre compression";
static const char text_good[] = "PowerPacker: good compression";
static const char text_verygood[] = "PowerPacker: very good compression";
static const char text_best[] = "PowerPacker: best compression";

static ubyte* sourceBuf;
static ubyte* readPtr;
static ubyte* writePtr;
static ubyte* startPtr;
static udword current;       // compressed data longword
static int bits;             // number of bits in 'current' to evaluate
static ubyte efficiency[4];
static udword outputLen;
static bool isCompressed;
static bool globalError;


// Move four bytes to Motorola big-endian double-word.
static inline void bytesTOudword()
{
  readPtr -= 4;
  if ( readPtr < sourceBuf )
    {
      ppErrorString = text_packeddatacorrupt;
      globalError = true;
    }
  else 
    current = readEndian(*readPtr,*(readPtr+1),*(readPtr+2),*(readPtr+3));
}

static void ppFreeMem()
{
  if (sourceBuf != 0)      // should not be required
    delete[] sourceBuf;
  sourceBuf = 0;
}

static inline udword ppRead(int count)
{
  udword data = 0;
  // read 'count' bits of packed data
  for (; count > 0; count--)
    {
      // equal to shift left
      data += data;
      // merge bit 0
      data = (data | (current&1));
      current = (current >> 1);
      if ((--bits) == 0)
	{
	  bytesTOudword();
	  bits = 32;
	}
    }
  return data;
}

static inline void ppBytes()
{
  udword count, add;
  count = (add = ppRead(2));
  while (add == 3)
    {
      add = ppRead(2);
      count += add;
    }
  count++;
  for (; count > 0 ; count--)
    {
      if (writePtr > startPtr)
	{
	  *(--writePtr) = (ubyte)ppRead(8);
	}
      else
	{
	  ppErrorString = text_packeddatacorrupt;
	  globalError = true;
	}
    }
}

static inline void ppSequence()
{
  udword offset, length, add;
  int offsetBitLen;
  length = ppRead( 2 );  // length -2
  offsetBitLen = (int)efficiency[length];
  length += 2;
  if ( length != 5 )
    offset = ppRead( offsetBitLen );
  else
    {
      if ( ppRead( 1 ) == 0 )
	offsetBitLen = 7;
      offset = ppRead( offsetBitLen );
      add = ppRead( 3 );
      length += add;
      while ( add == 7 )
	{
	  add = ppRead(3);
	  length += add;
	}
    }
  for ( ; length > 0 ; length-- )
    {
      if ( writePtr > startPtr )
	{
	  --writePtr;
	  *writePtr = *(writePtr+1+offset);
	}
      else
	{
	  ppErrorString = text_packeddatacorrupt;
	  globalError = true;
	}
    }
}

udword ppUncompressedLen()
{
  return outputLen;
}

bool ppIsCompressed()
{
  return isCompressed;
}



// returns: false = file is not in (supported) PowerPacker format,
//                  or some error (=> ppErrorString) has occured
//          true  = successful return (size via ppUncompressedLen())           

bool depp(ubyte *input, int inputSize, ubyte** destRef)
{
  globalError = false;  // assume no error
  isCompressed = false;
  outputLen = 0;

  ubyte * inp = input;


  // Check for header signature.
  char sig[5];
  memcpy(sig, inp, 4); inp += 4;
  sig[4] = 0;
  if ( strcmp(sig,PP_ID) != 0 )
    {
      ppErrorString = text_uncompressed;
      return false;
    }
	
  // Load efficiency table.
  memcpy((char*)efficiency, inp, 4); inp += 4;
  udword eff = readEndian(efficiency[0],efficiency[1],efficiency[2],
			  efficiency[3]);
  if (( eff != PP_BITS_FAST ) &&
      ( eff != PP_BITS_MEDIOCRE ) &&
      ( eff != PP_BITS_GOOD ) &&
      ( eff != PP_BITS_VERYGOOD ) &&
      ( eff != PP_BITS_BEST ))
    {
      ppErrorString = text_unrecognized;
      return false;
    }
	
  // We set this flag here and not before previous block, because we hope
  // that every file with a valid signature and a valid efficiency table
  // actually is PP-compressed.
  isCompressed = true;

  // Uncompressed size is stored at end of source file.
  int inputlen = inputSize;
  sourceBuf = input;

  inp = input;

  // backwards decompression
  readPtr = sourceBuf + inputlen -4;

  // uncompressed length in bits 31-8 of last dword
  outputLen = readEndian(0,*readPtr,*(readPtr+1),*(readPtr+2));

  // Free any previously existing destination buffer.
  if ( *destRef != 0 )
    delete[] *destRef;
	
  // Allocate memory for output data.
  if (( *destRef = new ubyte[outputLen]) == 0 )
    {
      ppErrorString = text_notenoughmemory;
      return false;
    }
  
  switch ( eff)
    {
    case PP_BITS_FAST:
      ppErrorString = text_fast;
      break;
    case PP_BITS_MEDIOCRE:
      ppErrorString = text_mediocre;
      break;
    case PP_BITS_GOOD:
      ppErrorString = text_good;
      break;
    case PP_BITS_VERYGOOD:
      ppErrorString = text_verygood;
      break;
    case PP_BITS_BEST:
      ppErrorString = text_best;
      break;
    }
  
  // put destptr to end of uncompressed data
  writePtr = *destRef + outputLen;
  // lowest dest. address for range-checks
  startPtr = *destRef;

  // read number of unused bits in 1st data dword
  // from lowest bits 7-0 of last dword
  bits = 32 - *(sourceBuf + inputlen -1);

  // decompress data
  bytesTOudword();
  if ( bits != 32 )
    current = ( current >> ( 32 - bits ));
  do
    {
      if ( ppRead( 1) == 0 )  
	ppBytes();
      if ( writePtr > *destRef )  
	ppSequence();
      if ( globalError )
	{
	  ppFreeMem();
	  return false;
	}
    } while ( writePtr > *destRef );

  // Finished.
  ppFreeMem();
  return true;
}
