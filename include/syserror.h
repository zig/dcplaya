/**
 * @ingroup    dcplaya
 * @file       syserror.h
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Error functions.
 *
 * @version    $Id: syserror.h,v 1.1 2002-09-13 14:51:27 zig Exp $
 */

#ifndef _SYSERROR_H_
#define _SYSERROR_H_


#include "sysdebug.h"
#include "sysmacro.h"


#define SERROR(error)                    \
  SMACRO_START                           \
    SDERROR(level, "GOTO '"#error"'\n"); \
    goto error;                          \
  SMACRO_END




#endif /* #ifndef _SYSERROR_H_ */

