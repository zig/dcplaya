/* 2002/02/20 */

#include <string.h>
#include <stdio.h>

#include "config.h"

#include "draw/text.h"
#include "option.h"
#include "controler.h"
#include "file_wrapper.h"
#include "driver_list.h"
#include "sysdebug.h"
#include "exceptions.h"

static spinlock_t visual_mutex;
static vis_driver_t * visual;
static int lcd_visual;

static int set_visual(vis_driver_t * d);

int option_setup(void)
{
  spinlock_init(&visual_mutex);

  /* $$$ Must be modified to suit clean driver list API */
  driver_list_lock(&vis_drivers);
  visual = 0;
  option_set_visual((vis_driver_t *)vis_drivers.drivers);
  driver_list_unlock(&vis_drivers);

  lcd_visual = OPTION_LCD_VISUAL_FFT;
  return 0;
}

vis_driver_t * option_visual(void)
{
  vis_driver_t *d;

  spinlock_lock(&visual_mutex);
  d = visual;
  driver_reference(&visual->common);
  spinlock_unlock(&visual_mutex);
  return d;
}

static int set_visual(vis_driver_t * d)
{
  if (d == visual) {
    driver_dereference(&d->common);
    return 0;
  }

  if (d) {
    int err;
    SDDEBUG("Start visual [%s,%p]\n", d->common.name, d);
    EXPT_GUARD_BEGIN;
    err = d->start();
    EXPT_GUARD_CATCH;
    err = -1;
    EXPT_GUARD_END;
    if (err) {
      driver_dereference(&d->common);
      return -1;
    }
  }

  if (visual) {
    SDDEBUG("Stop visual [%s,%p]\n",visual->common.name,visual);
    EXPT_GUARD_BEGIN;
    visual->stop();
    EXPT_GUARD_CATCH;
    EXPT_GUARD_END;
    driver_dereference(&visual->common);
  }

  visual = d;

  SDDEBUG("New visual [%s,%p]\n",
	  d ? d->common.name : "NONE", d);
  return 0;
}

int option_set_visual(vis_driver_t * d)
{
  int err;

  spinlock_lock(&visual_mutex);
  err = set_visual(d);
  spinlock_unlock(&visual_mutex);
  return err;
}

void option_no_visual()
{
  option_set_visual(0);
}

int option_lcd_visual()
{
  return lcd_visual;
}

