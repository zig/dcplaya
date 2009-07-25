#include "sha123/debug.h"
#include "sha123/dct.h"
#include "sha123/dct64.h"
#include "sha123/dct36.h"

struct _dcts_ {
  int points;
  sha123_dct_info_t ** info;
  void ** dest;
  sha123_dct_info_t * current;
  sha123_dct_info_t * sorted[DCT_MODE_MAX];
};

static struct _dcts_ dcts [] = {
  { 64, sha123_dct64_info, (void**)&sha123_dct64 },
  { 36, sha123_dct36_info, (void**)&sha123_dct36 },
  { 0,0 }
};


int sha123_dct_init(sha123_dct_mode_t mode, int out_scale)
{
  struct _dcts_ * _dct;

  sha123_debug("sha123_dct_init(%d,%d)\n", mode, out_scale);

  if ((unsigned int)mode >= DCT_MODE_MAX) {
    sha123_debug(" + invalid mode (%d), switching to default.\n", mode);
    mode = DCT_MODE_DEFAULT;
  }

  for (_dct=dcts; _dct->points; ++_dct) {
    sha123_dct_info_t * info;
    int i, mode2;

    sha123_debug(" + parsing %d point dct\n", _dct->points);

    for (i=0; i<DCT_MODE_MAX; ++i) {
      _dct->sorted[i] = 0;
    }
    _dct->current = 0;
    _dct->sorted[DCT_MODE_DEFAULT] = _dct->info[0];

    for (info = _dct->info[i=0]; info; info = _dct->info[++i]) {
      int j, modes = info->modes;

      sha123_debug("  - found [%s] modes:%x\n", info->name, modes);
      if (!info->dct) {
	sha123_debug("  - [%s] has no dct function, skip\n", info->name);
	continue;
      }
      for (j=0; j<DCT_MODE_MAX; ++j) {
	if (modes & (1<<j)) {
	  _dct->sorted[j] = info;
	}
      }
    }
    
    for (mode2=mode, info=0; mode2 != DCT_MODE_MAX; ) {
      sha123_debug(" + searching for mode [%d]\n", mode2);
      info = _dct->sorted[mode2];
      if (!info) {
	if (mode2 == DCT_MODE_DEFAULT) {
	  mode2 = DCT_MODE_MAX;
	} else {
	  sha123_debug("  - mode not supported , switching to default.\n");
	  mode2 = DCT_MODE_DEFAULT;
	}
      } else {
	if (info->init && info->init(out_scale)) {
	  sha123_debug("  - dct init failed\n");
	  mode2 =  (mode2 == DCT_MODE_DEFAULT)
	    ? DCT_MODE_MAX : DCT_MODE_DEFAULT;
	  info = 0;
	} else {
	  break;
	}
      }
    }
    
    if (!info) {
      sha123_debug(" -> error : no %d points dct found\n", _dct->points);
      sha123_dct_shutdown();
      return -1;
    }
    sha123_debug(" + using [%s] as %d points dct\n", info->name, _dct->points);
    _dct->current = info;
    *_dct->dest = info->dct;

  }
  sha123_debug(" -> initialized\n");
  return 0;
}

void sha123_dct_shutdown(void)
{
  struct _dcts_ * _dct;

  sha123_debug("sha123_dct_shutdown()\n");

  for (_dct=dcts; _dct->points; ++_dct) {
    sha123_debug(" + shutdown %d point dct\n", _dct->points);
    if (_dct->current) {
      if (_dct->current->shutdown) {
	sha123_debug("  - dct [%s] shutdown()\n", _dct->current->name);
	_dct->current->shutdown();
      } else {
	sha123_debug("  - dct [%s] has no shutdown function\n",
		     _dct->current->name);
      }
      *_dct->dest = 0;
      _dct->current = 0;
    } else {
      sha123_debug("  - no current dct\n");
    }
  }

  sha123_debug(" -> dcts shutdowned\n");

}
