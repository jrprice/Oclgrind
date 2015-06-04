// Uninitialized.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include "core/Context.h"
#include "core/Memory.h"

#include "Uninitialized.h"

using namespace oclgrind;
using namespace std;

#define KEY(memory,address) make_pair(memory, EXTRACT_BUFFER(address))

Uninitialized::Uninitialized(const Context *context)
 : Plugin(context)
{
}

void Uninitialized::hostMemoryStore(const Memory *memory,
                                    size_t address, size_t size,
                                    const uint8_t *storeData)
{
  setState(memory, address, size);
}

void Uninitialized::memoryAllocated(const Memory *memory, size_t address,
                                    size_t size, cl_mem_flags flags,
                                    const uint8_t *initData)
{
  m_state[KEY(memory,address)] = new bool[size]();
  if (initData)
    setState(memory, address, size);
}

void Uninitialized::memoryAtomicLoad(const Memory *memory,
                                     const WorkItem *workItem,
                                     AtomicOp op, size_t address, size_t size)
{
  checkState(memory, address, size);
}

void Uninitialized::memoryAtomicStore(const Memory *memory,
                                      const WorkItem *workItem,
                                      AtomicOp op, size_t address, size_t size)
{
  setState(memory, address, size);
}

void Uninitialized::memoryDeallocated(const Memory *memory, size_t address)
{
  delete[] m_state[KEY(memory,address)];
  m_state.erase(KEY(memory,address));
}

void Uninitialized::memoryLoad(const Memory *memory, const WorkItem *workItem,
                              size_t address, size_t size)
{
  checkState(memory, address, size);
}

void Uninitialized::memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                              size_t address, size_t size)
{
  checkState(memory, address, size);
}

void Uninitialized::memoryMap(const Memory *memory, size_t address,
                              size_t offset, size_t size, cl_map_flags flags)
{
  if (flags != CL_MAP_READ)
    setState(memory, address+offset, size);
}

void Uninitialized::memoryStore(const Memory *memory, const WorkItem *workItem,
                               size_t address, size_t size,
                               const uint8_t *storeData)
{
  setState(memory, address, size);
}

void Uninitialized::memoryStore(const Memory *memory, const WorkGroup *workGroup,
                               size_t address, size_t size,
                               const uint8_t *storeData)
{
  setState(memory, address, size);
}

void Uninitialized::checkState(const Memory *memory,
                               size_t address, size_t size) const
{
  if (!memory->isAddressValid(address, size))
    return;

  const bool *state = m_state.at(KEY(memory,address)) + EXTRACT_OFFSET(address);
  for (size_t offset = 0; offset < size; offset++)
  {
    if (!state[offset])
    {
      logError(memory->getAddressSpace(), address + offset);
      break;
    }
  }
}

void Uninitialized::logError(unsigned int addrSpace, size_t address) const
{
  Context::Message msg(ERROR, m_context);
  msg << "Uninitialized value read from "
      << getAddressSpaceName(addrSpace)
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

void Uninitialized::setState(const Memory *memory, size_t address, size_t size)
{
  if (!memory->isAddressValid(address, size))
    return;

  bool *state = m_state[KEY(memory,address)] + EXTRACT_OFFSET(address);
  fill(state, state+size, true);
}
