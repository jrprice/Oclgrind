// MemCheck.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
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

#include "MemCheck.h"

using namespace oclgrind;
using namespace std;

MemCheck::MemCheck(const Context *context)
 : Plugin(context)
{
}

void MemCheck::instructionExecuted(const WorkItem *workItem,
                                   const llvm::Instruction *instruction,
                                   const TypedValue& result)
{
  // Check static array bounds if load or store is executed
  const llvm::Value *PtrOp = nullptr;

  if (auto LI = llvm::dyn_cast<llvm::LoadInst>(instruction))
  {
    PtrOp = LI->getPointerOperand();
  }
  else if (auto SI = llvm::dyn_cast<llvm::StoreInst>(instruction))
  {
    PtrOp = SI->getPointerOperand();
  }
  else
  {
    return;
  }

  // Walk up chain of GEP instructions leading to this access
  while (auto GEPI =
           llvm::dyn_cast<llvm::GetElementPtrInst>(PtrOp->stripPointerCasts()))
  {
    checkArrayAccess(workItem, GEPI);

    PtrOp = GEPI->getPointerOperand();
  }
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

void MemCheck::memoryMap(const Memory *memory, size_t address,
                         size_t offset, size_t size, cl_map_flags flags)
{
  MapRegion map =
  {
    address, offset, size, memory->getPointer(address + offset),
    (flags == CL_MAP_READ ? MapRegion::READ : MapRegion::WRITE)
  };
  m_mapRegions.push_back(map);
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

void MemCheck::memoryUnmap(const Memory *memory, size_t address,
                           const void *ptr)
{
  for (auto region = m_mapRegions.begin();
            region != m_mapRegions.end();
            region++)
  {
    if (region->ptr == ptr)
    {
      m_mapRegions.erase(region);
      return;
    }
  }
}

void MemCheck::checkArrayAccess(const WorkItem *workItem,
                                const llvm::GetElementPtrInst *GEPI) const
{
  // Iterate through GEPI indices
  const llvm::Type *ptrType = GEPI->getPointerOperandType();

  for (auto opIndex = GEPI->idx_begin(); opIndex != GEPI->idx_end(); opIndex++)
  {
    int64_t index = workItem->getOperand(opIndex->get()).getSInt();

    if (ptrType->isArrayTy())
    {
      // Check index doesn't exceed size of array
      uint64_t size = ptrType->getArrayNumElements();

      if ((uint64_t)index >= size)
      {
        ostringstream info;
        info << "Index ("
             << index << ") exceeds static array size ("
             << size << ")";
        m_context->logError(info.str().c_str());
      }

      ptrType = ptrType->getArrayElementType();
    }
    else if (ptrType->isPointerTy())
    {
      ptrType = ptrType->getPointerElementType();
    }
    else if (ptrType->isVectorTy())
    {
      ptrType = ptrType->getVectorElementType();
    }
    else if (ptrType->isStructTy())
    {
      ptrType = ptrType->getStructElementType(index);
    }
  }
}

void MemCheck::checkLoad(const Memory *memory,
                         size_t address, size_t size) const
{
  if (!memory->isAddressValid(address, size))
  {
    logInvalidAccess(true, memory->getAddressSpace(), address, size);
    return;
  }

  if (memory->getBuffer(address)->flags & CL_MEM_WRITE_ONLY)
  {
    m_context->logError("Invalid read from write-only buffer");
  }
  
  if (memory->getAddressSpace() == AddrSpaceLocal || memory->getAddressSpace() == AddrSpacePrivate) return;

  // Check if memory location is currently mapped for writing
  for (auto region = m_mapRegions.begin();
            region != m_mapRegions.end();
            region++)
  {
    if (region->type == MapRegion::WRITE &&
        address < region->address + region->size &&
        address + size >= region->address)
    {
      m_context->logError("Invalid read from buffer mapped for writing");
    }
  }
}

void MemCheck::checkStore(const Memory *memory,
                          size_t address, size_t size) const
{
  if (!memory->isAddressValid(address, size))
  {
    logInvalidAccess(false, memory->getAddressSpace(), address, size);
    return;
  }

  if (memory->getBuffer(address)->flags & CL_MEM_READ_ONLY)
  {
    m_context->logError("Invalid write to read-only buffer");
  }

  if (memory->getAddressSpace() == AddrSpaceLocal || memory->getAddressSpace() == AddrSpacePrivate) return;

  // Check if memory location is currently mapped
  for (auto region = m_mapRegions.begin();
            region != m_mapRegions.end();
            region++)
  {
    if (address < region->address + region->size &&
        address + size >= region->address)
    {
      m_context->logError("Invalid write to mapped buffer");
    }
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
