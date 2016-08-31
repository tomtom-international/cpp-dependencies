#ifndef __DEP_CHECKER__FSTREAMINCLUDE_H
#define __DEP_CHECKER__FSTREAMINCLUDE_H

#if ( defined _MSC_VER && ( _MSC_VER > 1900 || _MSC_VER == 1900 )  ) || \
        ( defined __GNUC__ && (__GNUC__ > 5 || \
                                (__GNUC__ == 5 && \
                                    (__GNUC_MINOR__ > 3 || (__GNUC_MINOR__ == 3 && __GNUC_PATCHLEVEL__ > 0) ) ) ) )

#include "StdFstream.h"

#else

#include "BoostFstream.h"

#endif

#endif // !__DEP_CHECKER__FSTREAMINCLUDE_H
