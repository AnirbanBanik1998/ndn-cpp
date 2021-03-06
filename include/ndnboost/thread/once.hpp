#ifndef NDNBOOST_THREAD_ONCE_HPP
#define NDNBOOST_THREAD_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2006-7 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <ndnboost/thread/detail/config.hpp>
#include <ndnboost/thread/detail/platform.hpp>
#if defined(NDNBOOST_THREAD_PLATFORM_WIN32)
#include <ndnboost/thread/win32/once.hpp>
#elif defined(NDNBOOST_THREAD_PLATFORM_PTHREAD)
#if defined NDNBOOST_THREAD_ONCE_FAST_EPOCH
#include <ndnboost/thread/pthread/once.hpp>
#elif defined NDNBOOST_THREAD_ONCE_ATOMIC
#include <ndnboost/thread/pthread/once_atomic.hpp>
#else
#error "Once Not Implemented"
#endif
#else
#error "Boost threads unavailable on this platform"
#endif

#include <ndnboost/config/abi_prefix.hpp>

namespace ndnboost
{
  // template<class Callable, class ...Args> void
  // call_once(once_flag& flag, Callable&& func, Args&&... args);
template<typename Function>
inline void call_once(Function func,once_flag& flag)
//inline void call_once(void (*func)(),once_flag& flag)
    {
        call_once(flag,func);
    }
}

#include <ndnboost/config/abi_suffix.hpp>

#endif
