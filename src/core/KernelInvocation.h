// KernelInvocation.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
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

class KernelInvocation
{
public:
  static void run(const Context* context, Kernel* kernel, unsigned int workDim,
                  Size3 globalOffset, Size3 globalSize, Size3 localSize, bool localSizeSpecified=true);

  const Context* getContext() const;
  const WorkGroup* getCurrentWorkGroup() const;
  const WorkItem* getCurrentWorkItem() const;
  Size3 getGlobalOffset() const;
  Size3 getGlobalSize() const;
  Size3 getLocalSize() const;
  const Kernel* getKernel() const;
  Size3 getNumGroups() const;
  size_t getWorkDim() const;
  bool switchWorkItem(const Size3 gid);

  int getWorkerID() const;
  bool workGroupSizeSpecified() const;

private:
  KernelInvocation(const Context* context, const Kernel* kernel,
                   unsigned int workDim, Size3 globalOffset, Size3 globalSize,
                   Size3 localSize, bool localSizeSpecified);
  virtual ~KernelInvocation();
  void run();

  // Kernel launch parameters
  const Context* m_context;
  const Kernel* m_kernel;
  size_t m_workDim;
  Size3 m_globalOffset;
  Size3 m_globalSize;
  Size3 m_localSize;
  Size3 m_numGroups;
  bool m_localSizeSpecified;

  // Current execution state
  std::vector<Size3> m_workGroups;
  std::list<WorkGroup*> m_runningGroups;

  // Worker threads
  void runWorker(int id);
  unsigned m_numWorkers;
};
} // namespace oclgrind
