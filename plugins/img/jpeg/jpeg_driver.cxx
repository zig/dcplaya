/**
 * @ingroup dcplaya_plugin_img
 * @file    jpeg_driver.cxx
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/12/14
 * @brief   Jpeg (.jpg) image driver.
 */

#include "img_driver.h"
#include "sysdebug.h"
#include "SHAtranslatorJpg.h"

static SHAtranslatorJpg * translator;

/* Driver init fucntion. */
static int init(any_driver_t * driver)
{
  img_driver_t * d = (img_driver_t *)driver;

  SDDEBUG("[%s] : [%s]\n", __FUNCTION__, driver->name);
  if (translator) {
	SDERROR("already initialized\n");
	return -1;
  }

  translator = new SHAtranslatorJpg;
  if (!translator) {
	SDERROR("translator creation error.\n");
	return -1;
  }

  /* Set up translator. */
  d->translator = translator;

  /* Set up extensions. */
  d->extensions = *translator->Extension();

  return 0;
}

/* Driver shutdown function. */
static int shutdown(any_driver_t * driver)
{
  SDDEBUG("[%s] : [%s]\n", __FUNCTION__, driver->name);
  if (translator) {
	delete translator;
	translator = 0;
  }
  return 0;
}

/* Driver option function. */
static driver_option_t * options(any_driver_t * d, int idx,
								 driver_option_t * o)
{
  return o;
}


static img_driver_t driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    IMG_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "jpeg",               /* Driver name */
    "Benjamin Gerard, "
	"The Independent "
	"JPEG Group",         /* Driver authors */
    "jpeg (jpg) image"
	"translator",         /**< Description */
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },
  
  /* Image driver specific */
  0,                      /* User Id */
  0,                      /* Extension list (setted at init time.) */
  0,                      /* translator. */
};

extern "C" {

EXPORT_DRIVER(driver)

}
