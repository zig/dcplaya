/**
 * @ingroup   file68_devel
 * @file      SC68rsc.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1998/10/07
 * @brief     sc68 resource type definitions
 * @version   $id$
 *
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _SC68RSC_H_
#define _SC68RSC_H_

#ifdef __cplusplus
extern "C" {
#endif

/** SC68 resource file type. */
typedef enum
{
  SC68rsc_replay,       /**< 68000 external replay */
  SC68rsc_browseinfo,   /**< Browse info */
  SC68rsc_html,         /**< HTML file (browse info) */
  SC68rsc_config,       /**< Config file */
  SC68rsc_unix,         /**< Unix specific file (key_anchor) */
  SC68rsc_sample,       /**< Sc68 sample files */
  SC68rsc_dll,          /**< Sc68 dynamic library */
  SC68rsc_unknown=-1    /**< unknown or error code */
} SC68rsc_t;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68RSC_H_ */
