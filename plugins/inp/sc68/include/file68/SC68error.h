/**
 * @ingroup   file68_devel
 * @file      SC68error.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1998/09/08
 * @brief     sc68 error message handler.
 * @version   $Id: SC68error.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 *
 *    SC68 error message handling consist on a fixed size stack of fixed
 *    length strings. It provides functions for both error pushing and
 *    poping.
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _SC68ERROR_H_
#define _SC68ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Push a formatted error message.
 *
 *    The SC68error_add() function format error string into stack buffer. If
 *    stack is full, the older error message is removed.
 *
 *  @return error-code
 *  @retval 0xDEAD0???
 */
int SC68error_add(char *format, ... );

/** Get last error message.
 *
 *    The SC68error_get() function retrieves last error message and removes
 *    it from error message stack.
 *
 *  @param  format  printf() like format string
 *
 *  @return  Static string to last error message
 *  @retval  0  No stacked error message
 */
char *SC68error_get( void );

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68ERROR_H_ */
