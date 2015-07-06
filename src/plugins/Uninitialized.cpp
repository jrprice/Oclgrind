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
#include "core/WorkItem.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

#include "Uninitialized.h"

using namespace oclgrind;
using namespace std;

THREAD_LOCAL Uninitialized::LocalState Uninitialized::m_localState = {NULL};

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

void Uninitialized::instructionExecuted(const WorkItem *workItem,
                                        const llvm::Instruction *instruction,
                                        const TypedValue& result)
{
  if (auto alloca = llvm::dyn_cast<llvm::AllocaInst>(instruction))
  {
    // Set state for any padding bytes in structures
    const llvm::Type *type = alloca->getAllocatedType();
    if (auto structType = llvm::dyn_cast<llvm::StructType>(type))
    {
      if (!structType->isPacked())
      {
        size_t base = result.getPointer();

        unsigned size = 0;
        for (unsigned i = 0; i < structType->getStructNumElements(); i++)
        {
          // Get member size and alignment
          const llvm::Type *elemType = structType->getStructElementType(i);
          unsigned sz    = getTypeSize(elemType);
          unsigned align = getTypeAlignment(elemType);

          // Set state for padding
          if (size % align)
          {
            size_t padding = (align - (size%align));
            setState(workItem->getPrivateMemory(), base+size, padding);
            size += padding;
          }

          size += sz;
        }

        // Set state for padding at end of structure
        unsigned alignment = getTypeAlignment(structType);
        if (size % alignment)
        {
          unsigned padding = (alignment - (size%alignment));
          setState(workItem->getPrivateMemory(), base+size, padding);
        }
      }
    }
  }
}

void Uninitialized::memoryAllocated(const Memory *memory, size_t address,
                                    size_t size, cl_mem_flags flags,
                                    const uint8_t *initData)
{
  size_t buffer = memory->extractBuffer(address);
  if (memory->getAddressSpace() == AddrSpaceGlobal)
  {
    m_globalState[buffer] = new bool[size]();
  }
  else
  {
    if (!m_localState.state)
      m_localState.state = new map<const Memory*,StateMap>;
    (*m_localState.state)[memory][buffer] = new bool[size]();
  }
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
  size_t buffer = memory->extractBuffer(address);
  if (memory->getAddressSpace() == AddrSpaceGlobal)
  {
    delete[] m_globalState[buffer];
    m_globalState.erase(buffer);
  }
  else
  {
    delete[] m_localState.state->at(memory)[buffer];
    m_localState.state->at(memory).erase(buffer);
    if (!m_localState.state->at(memory).size())
    {
      m_localState.state->erase(memory);
      if (!m_localState.state->size())
      {
        delete m_localState.state;
        m_localState.state = NULL;
      }
    }
  }
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

  size_t buffer = memory->extractBuffer(address);
  size_t offset = memory->extractOffset(address);

  const bool *state;
  if (memory->getAddressSpace() == AddrSpaceGlobal)
    state = m_globalState.at(buffer) + offset;
  else
    state = m_localState.state->at(memory).at(buffer) + offset;

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
  Context::Message msg(WARNING, m_context);
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

  size_t buffer = memory->extractBuffer(address);
  size_t offset = memory->extractOffset(address);

  bool *state;
  if (memory->getAddressSpace() == AddrSpaceGlobal)
    state = m_globalState.at(buffer) + offset;
  else
    state = m_localState.state->at(memory).at(buffer) + offset;

  fill(state, state+size, true);
}
