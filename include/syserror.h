/**
 * @ingroup    dcplaya_devel
 * @file       syserror.h
 * @author     vincent penne
 * @date       2002/11/09
 * @brief      Error functions.
 *
 * @version    $Id: syserror.h,v 1.4 2003-03-26 23:02:48 ben Exp $
 */

#ifndef _SYSERROR_H_
#define _SYSERROR_H_

#include "sysdebug.h"
#include "sysmacro.h"

/** Throw error: send error message and jump to specified label */
#define STHROW_ERROR(error)              \
  SMACRO_START                           \
    SDERROR( "GOTO '" #error "'\n" );    \
    goto error;                          \
  SMACRO_END

#endif /* #ifndef _SYSERROR_H_ */

