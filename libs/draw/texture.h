/**
 * @ingroup  dcplaya_draw_texture
 * @file     draw/texture.h
 * @author   benjamin gerard
 * @author   vincent penne
 * @date     2002/10/20
 * @brief    texture manager
 *
 * $Id: texture.h,v 1.12 2004-06-30 15:17:35 vincentp Exp $
 */

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

/** @defgroup  dcplaya_draw_texture Texture Manager
 *  @ingroup   dcplaya_draw
 *  @brief     texture manager
 *  @author    benjamin gerard
 *  @author    vincent penne
 *  @{
 */

#include <arch/types.h>
#include "exheap.h"

/** Texture name maximum length. */
#define TEXTURE_NAME_MAX 24

/** Texture idetifier type. */
typedef int texid_t;

/** Texture definition type. */
typedef struct {
  /** Name of texture. */
  char name[TEXTURE_NAME_MAX];
  int width;     /**< Texture width in pixel         */
  int height;    /**< Texture height in pixel        */
  int wlog2;     /**< Log2 of texture width in pixel */
  int hlog2;     /**< Log2 of height in pixel        */
  int format;    /**< see list in ta.h               */
  int ref;       /**< Reference counter              */
  int lock;      /**< Lock counter                   */
  int twiddled;  /**< Twiddled state                 */
  int twiddlable;/**< Should we twiddle it ?         */

  eh_block_t ehb;/**< External heap block            */

  void * addr;   /**< Mapped VRAM address            */
  uint32 ta_tex; /**< Texture address for TA         */
} texture_t;


struct _texture_create_s;

/** Function to get texture data.
 *
 *     The texture_reader_f function is called by the texture_create()
 *     function to get the texture bitmap.
 */
typedef int (*texture_reader_f)(uint8 *buffer, int n,
				struct _texture_create_s * tc);

/** Texture creation type. */
typedef struct _texture_create_s {
  /*** Name of texture. */
  char name[TEXTURE_NAME_MAX];
  int  width;               /**< Width in pixel           */
  int  height;              /**< Height in pixel          */
  const char * formatstr;   /**< @see texture_formatstr() */
  texture_reader_f reader;  /**< texture data reader      */
  /* Put any other info you need from this point */
} texture_create_t;

/** Initialize the texture manager. */
int texture_init(void);

/** Shutdown the texture manager.
 *
 *    The texture_shutdown() function free all texture without verifying
 *    that all textures has been released.
 */
void texture_shutdown(void);

/** Get a texture identifier by name.
 *  @param  texture_name  texture name or filename.
 *  @return texture-id
 */
texid_t texture_get(const char * texture_name);

/** Check if a given texture id is valid.
 *  @param  texid  texture-id to test.
 * @return  texid
 * @retval  -1 on error (texture does nor exist).
 */
texid_t texture_exist(texid_t texid);

/** Duplicate a texture.
 *
 *     The texture_dup() function duplicates a texture. Everything gets copied
 *     including texture bitmap.
 *
 *  @param  texid   texture-id to duplicate
 *  @param  name    duplicated texture name
 *
 *  @return texture-id
 *  @retval -1 Error
 */
texid_t texture_dup(texid_t texid, const char * name);

/** Create a new texture.
 *
 *     The texture_create() function creates a new texture from a 
 *     texture_create_t creator. This creator contains the basic information
 *     for the new texture (name, dimension, format...) and a function which
 *     is call the reader that wiil be call to get texture bitmap.
 *     This reader function recieve the creator as parameter. Any supplemental 
 *     information could be put after the required creator data such as FILE or
 *     memory pointer.
 *
 *  @param  creator  texture creator used to create this texture.
 *
 *  @return texture-id
 *  @retval -1 Error
 *
 * @see texture_reader_f
 */
texid_t texture_create(texture_create_t * creator);

/** Create a flat texture.
 *
 *  @param  name       Name of texture.
 *  @param  width      Width in pixel
 *  @param  height     Height in pixel
 *  @param  argb       Texture pixel format
 *
 *  @return  texture-id
 *  @return  -1  Error
 */
texid_t texture_create_flat(const char *name, int width, int height,
			    unsigned int argb);

/** Create a new texture from a image file.
 *
 *    The texture_create_file() tries to load an file via the image translator
 *    LoadImageFile() function. And creates a texture with it.
 *
 *  @param  fname      Path of an image file.
 *  @param  formatstr  Output format. 0 tries to keep original image one.
 *
 *  @return  texture-id
 *  @return  -1  Error
 */
texid_t texture_create_file(const char *fname, const char * formatstr);

/** Destroy a texture. */
int texture_destroy(texid_t texid, int force);

/** Add, remove or get texture reference counter. */
int texture_reference(texid_t texid, int count);

/** Get a pointer on a texture definition and make sure pixels
    are not twiddled. */
texture_t * texture_lock(texid_t texid, int wait);

/** Get a pointer on a texture definition without de-twiddling. */
texture_t * texture_fastlock(texid_t texid, int wait);

/** Release a previously locked texture. */
void texture_release(texture_t * t);

/** Perform texture garbage colector. */
void texture_collector(void);

/** Get texture format string.
 *
 *    The function texture_formatstr() returns a string that describe a
 *    texture format. Currently it is only describes the "ARGB" pixel format. 
 *    The returned string is 4 char long and each char gives the number of bit
 *    of a channel in this order Alpha, Red, Green and Blue.
 *
 *  @return  A string that describe the texture format.
 *  @retval  "0565"   16 bit: Alpha:0, Red:5, Green:6, Blue:5. 
 *  @retval  "1555"   16 bit: Alpha:1, Red:5, Green:5, Blue:5. 
 *  @retval  "4444"   16 bit: Alpha:4, Red:4, Green:4, Blue:4. 
 *  @retval  "????"   Unknown texture format.
 *
 *  @see texture_strtoformat()
 */
const char * texture_formatstr(int format);

/** Get texture format from a format  string.
 *
 * @see texture_formatstr()
 */
int texture_strtoformat(const char * formatstr);


/** Display statistics about the video memory */
void texture_memstats();

/** Twiddle or de-twiddle a texture as required.
 *
 *   The texture_twiddle() checks if the texture is twiddlable by
 *   calling the texture_twiddlable() function. If texture is twiddlable
 *   the fucntion performs neccessary operations to set the texture twiddle
 *   stat as wanted.
 *
 *    @param  t       texture
 *    @param  wanted  0:De-twiddle 1:Twiddle
 *
 *  @return twiddled stat.
 *
 *  @see int texture_twiddlable();
 *  @warning The returned stat is not neccessary the wanted one.
 */
int texture_twiddle(texture_t * t, int wanted);

/* Check for twiddlable texture and set twiddlable bit properly.
 *
 *   The texture_twiddlable() looks at texture dimension to check if it
 *   is a twiddable texture and set the twiddable field.
 *   Currently only texture with power of 2 dimensions and a width greater
 *   or equal to height are twiddlable.
 *
 *    @param  t  texture
 *
 *  @return twiddlable stat.
 */
int texture_twiddlable(texture_t * t);

/**@}*/

#endif /* #define _TEXTURE_H_ */
