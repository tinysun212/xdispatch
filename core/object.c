/*
* Copyright (c) 2008-2009 Apple Inc. All rights reserved.
* Copyright (c) 2011 MLBA. All rights reserved.
*
* @MLBA_OPEN_LICENSE_HEADER_START@
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
* @MLBA_OPEN_LICENSE_HEADER_END@
*/


#include "internal.h"


void
dispatch_debug(dispatch_object_t obj, const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);

    dispatch_debugv(obj, msg, ap);

    va_end(ap);
}

void
dispatch_debugv(dispatch_object_t dou, const char *msg, va_list ap)
{
    char buf[4096];
    size_t offs;

    struct dispatch_object_s *obj = DO_CAST(dou);

    if (obj && obj->do_vtable->do_debug) {
        offs = dx_debug(obj, buf, sizeof(buf));
    } else {
        offs = snprintf(buf, sizeof(buf), "NULL vtable slot");
    }

    snprintf(buf + offs, sizeof(buf) - offs, ": %s", msg);

    _dispatch_logv(buf, ap);
}

void
dispatch_retain(dispatch_object_t dou)
{
    struct dispatch_object_s *obj = DO_CAST(dou);

    if (obj->do_xref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT) {
        return; // global object
    }
    if ((dispatch_atomic_inc(&obj->do_xref_cnt) - 1) == 0) {
        DISPATCH_CLIENT_CRASH("Resurrection of an object");
    }
}

void
_dispatch_retain(struct dispatch_object_s* obj)
{

    if (obj->do_ref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT) {
        return; // global object
    }
    if ((dispatch_atomic_inc(&obj->do_ref_cnt) - 1) == 0) {
        DISPATCH_CLIENT_CRASH("Resurrection of an object");
    }
}

void
dispatch_release(dispatch_object_t dou)
{
    struct dispatch_object_s *obj = DO_CAST(dou);

    unsigned int oldval;

    if (obj->do_xref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT) {
        return;
    }

    oldval = dispatch_atomic_dec(&obj->do_xref_cnt) + 1;

    if (fastpath(oldval > 1)) {
        return;
    }
    if (oldval == 1) {
        if ((uintptr_t)obj->do_vtable == (uintptr_t)&_dispatch_source_kevent_vtable) {
            return _dispatch_source_xref_release(DSOURCE_CAST(dou));
        }
        if (slowpath(DISPATCH_OBJECT_SUSPENDED(obj))) {
            // Arguments for and against this assert are within 6705399
            DISPATCH_CLIENT_CRASH("Release of a suspended object");
        }
        return _dispatch_release(obj);
    }
    DISPATCH_CLIENT_CRASH("Over-release of an object");
}

void
_dispatch_dispose(struct dispatch_object_s* obj)
{

    dispatch_queue_t tq = obj->do_targetq;
    dispatch_function_t func = obj->do_finalizer;
    void *ctxt = obj->do_ctxt;

    obj->do_vtable = (struct dispatch_object_vtable_s *)0x200;

    free(obj);

    if (func && ctxt) {
        dispatch_async_f(tq, ctxt, func);
    }
    _dispatch_release(DO_CAST(tq));
}

void
_dispatch_release(struct dispatch_object_s* obj)
{
    unsigned int oldval;

    if (obj->do_ref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT) {
        return; // global object
    }

    oldval = dispatch_atomic_dec(&obj->do_ref_cnt) + 1;

    if (fastpath(oldval > 1)) {
        return;
    }
    if (oldval == 1) {
        if (obj->do_next != DISPATCH_OBJECT_LISTLESS) {
            DISPATCH_CRASH("release while enqueued");
        }
        if (obj->do_xref_cnt) {
            DISPATCH_CRASH("release while external references exist");
        }

        return dx_dispose(obj);
    }
    DISPATCH_CRASH("over-release");
}

void *
dispatch_get_context(dispatch_object_t dou)
{
    struct dispatch_object_s *obj = DO_CAST(dou);

    return obj->do_ctxt;
}

void
dispatch_set_context(dispatch_object_t dou, void *context)
{
    struct dispatch_object_s *obj = DO_CAST(dou);

    if (obj->do_ref_cnt != DISPATCH_OBJECT_GLOBAL_REFCNT) {
        obj->do_ctxt = context;
    }
}

void
dispatch_set_finalizer_f(dispatch_object_t dou, dispatch_function_t finalizer)
{
    struct dispatch_object_s *obj = DO_CAST(dou);

    obj->do_finalizer = finalizer;
}

void
dispatch_suspend(dispatch_object_t dou)
{
    struct dispatch_object_s *obj = DO_CAST(dou);

    if (slowpath(obj->do_ref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT)) {
        return;
    }
    (void)dispatch_atomic_add(&obj->do_suspend_cnt, DISPATCH_OBJECT_SUSPEND_INTERVAL);
}

void
dispatch_resume(dispatch_object_t dou)
{
    struct dispatch_object_s* obj = DO_CAST(dou);

    // Global objects cannot be suspended or resumed. This also has the
    // side effect of saturating the suspend count of an object and
    // guarding against resuming due to overflow.
    if (slowpath(obj->do_ref_cnt == DISPATCH_OBJECT_GLOBAL_REFCNT)) {
        return;
    }

    // Switch on the previous value of the suspend count.  If the previous
    // value was a single suspend interval, the object should be resumed.
    // If the previous value was less than the suspend interval, the object
    // has been over-resumed.
    switch (dispatch_atomic_sub(&obj->do_suspend_cnt, DISPATCH_OBJECT_SUSPEND_INTERVAL) + DISPATCH_OBJECT_SUSPEND_INTERVAL) {
    case DISPATCH_OBJECT_SUSPEND_INTERVAL:
        _dispatch_wakeup((struct dispatch_queue_s*)obj);
        break;
    case DISPATCH_OBJECT_SUSPEND_LOCK:
    case 0:
        DISPATCH_CLIENT_CRASH("Over-resume of an object");
        break;
    default:
        break;
    }
}

size_t
dispatch_object_debug_attr(struct dispatch_object_s* dou, char* buf, size_t bufsiz)
{

    return snprintf(buf, bufsiz, "refcnt = 0x%x, suspend_cnt = 0x%x, ",
                    dou->do_ref_cnt, dou->do_suspend_cnt);
}
