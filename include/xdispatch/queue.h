
/*
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


#ifndef XDISPATCH_QUEUE_H_
#define XDISPATCH_QUEUE_H_

#ifndef __XDISPATCH_INDIRECT__
#error "Please #include <xdispatch/dispatch.h> instead of this file directly."
#endif

#include "dispatch.h"
#include <memory>

__XDISPATCH_BEGIN_NAMESPACE

/**
Provides an interface for representing
a dispatch queue and methods that can be
called to modify or use the queue.

Read Apple's documentation of libDispatch
to understand the concept of tasks and
queues.

@see xdispatch::dispatch for creation of queues
*/
class queue {

public:
    /**
      Creates a new queue using the given
      dispatch_queue_t object
      */
    queue(dispatch_queue_t);
    /**
      Creates a new serial queue featuring
      the given label
      */
    queue(const char* label);
    queue(const queue&);
    ~queue();

    /**
     A pointer automatically deleting
     the queue when going out
     of scope.
     */
    typedef std::auto_ptr< queue > a_ptr;

    /**
      Will dispatch the given operation for
      async execution on the queue and return
      immediately.

      The operation will be deleted as soon
      as it was executed. To change this behaviour,
      set the auto_delete flag of the operation.
      @see operation::auto_delete()
      */
    virtual void async(operation*);
#ifdef XDISPATCH_HAS_BLOCKS
    /**
    Same as async(operation*).
    Will put the given block on the queue.
    */
    virtual void async(dispatch_block_t);
#endif
    /**
    Applies the given iteration_operation for async execution
    in this queue and returns immediately.

      The operation will be deleted as soon
      as it was executed the requested number of times.
      To change this behaviour, set the auto_delete flag
      of the operation.
      @see operation::auto_delete()

    @param times The number of times the operation will be executed
    */
    virtual void apply(iteration_operation*, size_t times);
#ifdef XDISPATCH_HAS_BLOCKS
    /**
    Same as apply(iteration_operation*, size_t times).

    Will wrap the given block in an operation and put it on the
    queue.

    @see apply(iteration_operation*, size_t times)
    */
    virtual void apply(dispatch_iteration_block_t, size_t times);
#endif
    /**
    Applies the given operation for async execution
    in this queue after the given time and returns immediately

    @param time The time to wait until the operation is applied to
    the queue.
    */
    virtual void after(operation*, time_t time);
    virtual void after(operation*, dispatch_time_t time);
#ifdef XDISPATCH_HAS_BLOCKS
    /**
    Same as dispatch_after(operation*, time_t).
    Will wrap the given block in an operation and put it on the
    queue.
    */
    virtual void after(dispatch_block_t, time_t time);
    virtual void after(dispatch_block_t, dispatch_time_t time);
#endif
    /**
    Applies the given operation for execution
    in this queue and blocks until the operation
    was executed
    */
    virtual void sync(operation*);
#ifdef XDISPATCH_HAS_BLOCKS
    /**
    Same as dispatch_sync(operation*).
    Will wrap the given block in an operation and put it on the
    queue.
    */
    virtual void sync(dispatch_block_t);
#endif
    /**
    Sets the given operation as finalizer for this
    queue. A finalizer is called before destroying
    a queue, i.e. if all queue objects
    representing the queue were deleted and all
    pending work on a queue was dispatched.

    When not passing a queue, the finalizer operation
    will be executed on the queue itself.
    */
    virtual void set_finalizer(operation*, queue* = NULL);
#ifdef XDISPATCH_HAS_BLOCKS
    /**
    Same as set_finalizer(operation*, queue*).
    Will wrap the given block in an operation and store
    it as finalizer.
    */
    virtual void set_finalizer(dispatch_block_t, queue* = NULL);
#endif
    /**
    @return The label of the queue that was used while creating it
    */
    virtual const std::string label() const;
    /**
    Suspends the invocation of work items on this queue.

    A suspended queue will not invoke any blocks associated with it. The
    suspension of a queue will occur after any running work item associated with
    the queue completes.

    Calls to suspend() must be balanced with calls to resume().
    */
    virtual void suspend();
    /**
    Resumes the invocation of work items on this queue.

    Calls to suspend() must be balanced with calls to resume().
    */
    virtual void resume();
    /**
    @returns The dispatch_queue_t object associated with this
    C++ object. Use this, if you need to use the plain C Interface
    of libdispatch.
    */
    virtual const dispatch_queue_t native() const;

private:
    class data;
    data* d;

};

__XDISPATCH_END_NAMESPACE

#endif /* XDISPATCH_QUEUE_H_ */