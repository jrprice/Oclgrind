// MemCheck.h (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

namespace oclgrind
{
  class MemCheck : public Plugin
  {
  public:
    MemCheck(const Context *context);

    virtual void memoryAtomicLoad(const Memory *memory,
                                  const WorkItem *workItem,
                                  AtomicOp op, size_t address, size_t size);
    virtual void memoryAtomicStore(const Memory *memory,
                                   const WorkItem *workItem,
                                   AtomicOp op, size_t address, size_t size);
    virtual void memoryLoad(const Memory *memory, const WorkItem *workItem,
                            size_t address, size_t size);
    virtual void memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                            size_t address, size_t size);
    virtual void memoryStore(const Memory *memory, const WorkItem *workItem,
                             size_t address, size_t size,
                             const uint8_t *storeData);
    virtual void memoryStore(const Memory *memory, const WorkGroup *workGroup,
                             size_t address, size_t size,
                             const uint8_t *storeData);

  private:
    void checkLoad(const Memory *memory, size_t address, size_t size);
    void checkStore(const Memory *memory, size_t address, size_t size);
    void logInvalidAccess(bool read, unsigned addrSpace,
                          size_t address, size_t size) const;
  };
}
