// async_queue.h (Oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

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
