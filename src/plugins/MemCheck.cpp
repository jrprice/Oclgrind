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
  if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(instruction))
  {
    // Iterate through GEP indices
    const llvm::Type *ptrType = gep->getPointerOperandType();
    for (auto opIndex = gep->idx_begin(); opIndex != gep->idx_end(); opIndex++)
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