/**
 * @ingroup   io68_devel
 * @file      mfp_io.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MFP emulator plugin
 * @version   $Id: mfp_io.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MFP_IO_H_
#define _MFP_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

/** EMU68 compatible IO plugin for MFP emulation. */
extern io68_t mfp_io;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MFP_IO_H_ */
