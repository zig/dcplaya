/**
 * @ingroup    dcplaya
 * @file       syserror.h
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Error functions.
 *
 * @version    $Id: syserror.h,v 1.2 2002-09-13 16:48:39 zig Exp $
 */

#ifndef _SYSERROR_H_
#define _SYSERROR_H_


#include "sysdebug.h"
#include "sysmacro.h"


#define STHROW_ERROR(error)                    \
  SMACRO_START                           \
    SDERROR("GOTO '"#error"'\n"); \
    goto error;                          \
  SMACRO_END




#endif /* #ifndef _SYSERROR_H_ */

