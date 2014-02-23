// async_queue.h (Oclgrind)
// Copyright (c) 2013-2014, University of Bristol
// All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "icd.h"

#include "spirsim/Queue.h"

extern void asyncEnqueue(cl_command_queue queue,
                         cl_command_type type,
                         spirsim::Queue::Command *cmd,
                         cl_uint numEvents,
                         const cl_event *waitList,
                         cl_event *eventOut);
extern void asyncQueueRetain(spirsim::Queue::Command *cmd, cl_mem mem);
extern void asyncQueueRetain(spirsim::Queue::Command *cmd, cl_kernel);
extern void asyncQueueRelease(spirsim::Queue::Command *cmd);
