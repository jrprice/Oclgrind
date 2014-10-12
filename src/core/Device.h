// Device.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace oclgrind
{
  class Context;
  class Kernel;
  class WorkGroup;
  class WorkItem;

  class Device
  {
  public:
    Device(const Context *context);
    virtual ~Device();

    const WorkGroup* getCurrentWorkGroup() const;
    const WorkItem* getCurrentWorkItem() const;
    const KernelInvocation* getCurrentKernelInvocation() const;
    void run(Kernel *kernel, unsigned int workDim,
             const size_t *globalOffset,
             const size_t *globalSize,
             const size_t *localSize);
    bool switchWorkItem(const size_t gid[3]);

  private:
    const Context *m_context;

    struct PendingWorkGroup
    {
      size_t group[3];
      size_t operator[](int i) const
      {
        return group[i];
      }
    };

    // Current kernel invocation
    const KernelInvocation *m_kernelInvocation;
    WorkGroup *m_currentWorkGroup;
    WorkItem *m_currentWorkItem;
    std::list<PendingWorkGroup> m_pendingGroups;
    std::list<WorkGroup*> m_runningGroups;

    bool m_quickMode;

    WorkGroup* createWorkGroup(size_t x, size_t y, size_t z);
    bool nextWorkItem();
  };
}
