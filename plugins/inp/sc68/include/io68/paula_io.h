/**
 * @ingroup   io68_devel
 * @file      paula_io.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1998/06/18
 * @brief     IO plugin for Paula (Amiga sound)
 * @version   $Id: paula_io.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _PAULA_IO_H_
#define _PAULA_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

/** EMU68 compatible IO plugin for Paula emulation */
extern io68_t paula_io;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PAULA_IO_H_ */
