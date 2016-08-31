#ifndef __DEP_CHECKER__FSTREAMINCLUDE_H
#define __DEP_CHECKER__FSTREAMINCLUDE_H

#ifdef _MSC_VER

#if ( _MSC_VER < 1900 )

#include "BoostFstream.h"

#else

#include "StdFstream.h"

#endif

#endif // _MSC_VER

#ifdef __GNUC__

#if __GNUC__ > 5 || \
    (__GNUC__ == 5 && \
        (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0)))

#include "StdFstream.h"

#else

#include "BoostFstream.h"

#endif

#endif // __GNUC__

#ifdef __clang__

#include "BoostFstream.h"

#endif // __clang__

#endif // !__DEP_CHECKER__FSTREAMINCLUDE_H
