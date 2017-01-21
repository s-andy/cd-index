/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_CDINDEX_H_
#define _CD_CDINDEX_H_

#define DEBUG_BASEIO    0
#define DEBUG_INDEX     1
#define DEBUG_DEBUG     2

#define DEBUG_LEVEL     DEBUG_DEBUG

#ifdef DEBUG
#define DEBUG_OUTPUT(LEVEL, FORMAT, ARGS ...) \
    if (LEVEL >= DEBUG_LEVEL) printf("[debug] " FORMAT, ARGS)
#else
#define DEBUG_OUTPUT(FORMAT, ARGS ...)
#endif

#define __DEBUG(STRING) \
    printf(STRING)
#define _DEBUG(FORMAT, ARGS ...) \
    printf(FORMAT, ARGS)

#endif /* _CD_CDINDEX_H_ */
