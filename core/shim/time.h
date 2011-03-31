/*
 * Copyright (c) 2010 Apple Inc. All rights reserved.
 * Copyright (c) 2011 MLBA. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

/*
 * IMPORTANT: This header file describes INTERNAL interfaces to libdispatch
 * which are subject to change in future releases of Mac OS X. Any applications
 * relying on these interfaces WILL break.
 */

#ifndef __DISPATCH_SHIMS_TIME__
#define __DISPATCH_SHIMS_TIME__

#ifdef __MACH__
# include <mach/mach_time.h>
#endif

#if defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || !defined(HAVE_MACH_ABSOLUTE_TIME)
 // these architectures always return mach_absolute_time() in nanoseconds
# define _dispatch_time_mach2nano(x) (x)
# define _dispatch_time_nano2mach(x) (x)
#else
typedef struct _dispatch_host_time_data_s {
    long double frac;
    bool ratio_1_to_1;
    dispatch_once_t pred;
} _dispatch_host_time_data_s;
extern _dispatch_host_time_data_s _dispatch_host_time_data;
_dispatch_get_host_time_init(void *context);

static inline uint64_t
_dispatch_time_mach2nano(uint64_t machtime)
{
    _dispatch_host_time_data_s *const data = &_dispatch_host_time_data;
    dispatch_once_f(&data->pred, NULL, _dispatch_get_host_time_init);

    return (uint64_t)(machtime * data->frac);
}

static inline int64_t
_dispatch_time_nano2mach(int64_t nsec)
{
    _dispatch_host_time_data_s *const data = &_dispatch_host_time_data;
    dispatch_once_f(&data->pred, NULL, _dispatch_get_host_time_init);

    if (slowpath(_dispatch_host_time_data.ratio_1_to_1)) {
        return nsec;
    }

    long double big_tmp = (long double)nsec;

    // Divide by tbi.numer/tbi.denom to convert nsec to Mach absolute time
    big_tmp /= data->frac;

    // Clamp to a 64bit signed int
    if (slowpath(big_tmp > INT64_MAX)) {
        return INT64_MAX;
    }
    if (slowpath(big_tmp < INT64_MIN)) {
        return INT64_MIN;
    }
    return (int64_t)big_tmp;
}
#endif

#ifdef _WIN32
    static inline uint64_t
    _dispatch_absolute_time(void)
    {
        // TODO: We need a more precise solution in here!
        return GetTickCount()*1000*1000;
    }

#else
    static inline uint64_t
    _dispatch_absolute_time(void)
    {
    #ifdef __MACH__
        return mach_absolute_time();
    #else
        struct timespec ts;
        int ret;

    #if HAVE_DECL_CLOCK_UPTIME
        ret = clock_gettime(CLOCK_UPTIME, &ts);
    #else
        ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    #endif
        (void)dispatch_assume_zero(ret);

        /* XXXRW: Some kind of overflow detection needed? */
        return (ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec);
    #endif
    }
#endif

#endif /* __DISPATCH_SHIMS_TIME__ */