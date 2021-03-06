/*
Copyright Rene Rivera 2011-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef NDNBOOST_PREDEF_ARCHITECTURE_PYRAMID_H
#define NDNBOOST_PREDEF_ARCHITECTURE_PYRAMID_H

#include <ndnboost/predef/version_number.h>
#include <ndnboost/predef/make.h>

/*`
[heading `NDNBOOST_ARCH_PYRAMID`]

Pyramid 9810 architecture.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`pyr`] [__predef_detection__]]
    ]
 */

#define NDNBOOST_ARCH_PYRAMID NDNBOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(pyr)
#   undef NDNBOOST_ARCH_PYRAMID
#   define NDNBOOST_ARCH_PYRAMID NDNBOOST_VERSION_NUMBER_AVAILABLE
#endif

#if NDNBOOST_ARCH_PYRAMID
#   define NDNBOOST_ARCH_PYRAMID_AVAILABLE
#endif

#define NDNBOOST_ARCH_PYRAMID_NAME "Pyramid 9810"

#include <ndnboost/predef/detail/test.h>
NDNBOOST_PREDEF_DECLARE_TEST(NDNBOOST_ARCH_PYRAMID,NDNBOOST_ARCH_PYRAMID_NAME)


#endif
