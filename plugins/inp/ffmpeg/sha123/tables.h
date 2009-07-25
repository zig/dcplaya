/**
 *  @ingroup sha123
 *  @file    sha123/tables.h
 *  @author  benjamin gerard
 *  @date    2003/04/09
 *  @brief   look up tables.
 */

#ifndef _SHA123_TABLES_H_
#define _SHA123_TABLES_H_

#include "sha123/types.h"

extern real sha123_decwin[];      /**< Decimation table.       */
extern real * sha123_pnts[];      /**< Cosinus tables.         */
extern real sha123_muls[27][64];  /**< Used by layer I and II. */

/** Creates mpeg look-up tables.
 *
 *  @param scaleval final output level (typically 32767)
 *
 *  @return error-code
 *  @retval -1 on error
 */
int sha123_make_decode_tables(int scaleval);

#endif /* #define _SHA123_TABLES_H_ */
