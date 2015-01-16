// MemCheck.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include "core/Context.h"
#include "core/Memory.h"

#include "MemCheck.h"

using namespace oclgrind;
using namespace std;

MemCheck::MemCheck(const Context *context)
 : Plugin(context)
{
}

void MemCheck::memoryAtomicLoad(const Memory *memory,
                                const WorkItem *workItem,
                                AtomicOp op, size_t address, size_t size)
{
  checkLoad(memory, address, size);
}

void MemCheck::memoryAtomicStore(const Memory *memory,
                                 const WorkItem *workItem,
                                 AtomicOp op, size_t address, size_t size)
{
  checkStore(memory, address, size);
}

void MemCheck::memoryLoad(const Memory *memory, const WorkItem *workItem,
                          size_t address, size_t size)
{
  checkLoad(memory, address, size);
}

void MemCheck::memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                          size_t address, size_t size)
{
  checkLoad(memory, address, size);
}

void MemCheck::memoryStore(const Memory *memory, const WorkItem *workItem,
                           size_t address, size_t size,
                           const uint8_t *storeData)
{
  checkStore(memory, address, size);
}

void MemCheck::memoryStore(const Memory *memory, const WorkGroup *workGroup,
                           size_t address, size_t size,
                           const uint8_t *storeData)
{
  checkStore(memory, address, size);
}

void MemCheck::checkLoad(const Memory *memory, size_t address, size_t size)
{
  const Memory::Buffer *buffer = memory->getBuffer(address);
  if (!buffer || EXTRACT_OFFSET(address)+size > buffer->size)
  {
    logInvalidAccess(true, memory->getAddressSpace(), address, size);
    return;
  }

  if (buffer->flags & CL_MEM_WRITE_ONLY)
  {
    m_context->logError("Invalid read from write-only buffer");
  }
}

void MemCheck::checkStore(const Memory *memory, size_t address, size_t size)
{
  const Memory::Buffer *buffer = memory->getBuffer(address);
  if (!buffer || EXTRACT_OFFSET(address)+size > buffer->size)
  {
    logInvalidAccess(false, memory->getAddressSpace(), address, size);
    return;
  }

  if (buffer->flags & CL_MEM_READ_ONLY)
  {
    m_context->logError("Invalid write to read-only buffer");
  }
}

void MemCheck::logInvalidAccess(bool read, unsigned addrSpace,
                                size_t address, size_t size) const
{
  Context::Message msg(ERROR, m_context);
  msg << "Invalid " << (read ? "read" : "write")
      << " of size " << size
      << " at " << getAddressSpaceName(addrSpace)
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}