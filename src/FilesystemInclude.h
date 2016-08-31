#ifndef __DEP_CHECKER__FILESYSTEMINCLUDE_H
#define __DEP_CHECKER__FILESYSTEMINCLUDE_H

#ifdef _MSC_VER

#if ( _MSC_VER < 1900 )

#include "BoostFilesystem.h"

#else

#include "StdFilesystem.h"

#endif // _msc version

#endif // _MSC_VER

#ifdef __GNUC__

#if __GNUC__ > 5 || \
    (__GNUC__ == 5 && \
        (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0)))

#include "StdFilesystem.h"

#else

#include "BoostFilesystem.h"

#endif // gcc version

#endif // __GNUC__

#ifdef __clang__
#define __clang__

#include "BoostFilesystem.h"

#endif // __clang__

#endif // !__DEP_CHECKER__FILESYSTEMINCLUDE_H
