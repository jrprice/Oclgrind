// InstructionCounter.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include <sstream>

#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Type.h"

#include "InstructionCounter.h"

#include "core/Kernel.h"
#include "core/KernelInvocation.h"

using namespace oclgrind;
using namespace std;

#define COUNTED_LOAD_BASE  (llvm::Instruction::OtherOpsEnd + 4)
#define COUNTED_STORE_BASE (COUNTED_LOAD_BASE + 8)
#define COUNTED_CALL_BASE  (COUNTED_STORE_BASE + 8)

static bool compareNamedCount(pair<string,size_t> a, pair<string,size_t> b)
{
  return a.second > b.second;
}

string InstructionCounter::getOpcodeName(unsigned opcode) const
{
  if (opcode >= COUNTED_CALL_BASE)
  {
    // Get functon name
    unsigned index = opcode - COUNTED_CALL_BASE;
    assert(index < m_functions.size());
    return "call " + m_functions[index]->getName().str() + "()";
  }
  else if (opcode >= COUNTED_LOAD_BASE)
  {
    // Create stream using default locale
    ostringstream name;
    locale defaultLocale("");
    name.imbue(defaultLocale);

    // Get number of bytes
    size_t bytes = m_memopBytes[opcode-COUNTED_LOAD_BASE];

    // Get name of operation
    if (opcode >= COUNTED_STORE_BASE)
    {
      opcode -= COUNTED_STORE_BASE;
      name << "store";
    }
    else
    {
      opcode -= COUNTED_LOAD_BASE;
      name << "load";
    }

    // Add address space to name
    name << " " << getAddressSpaceName(opcode);

    // Add number of bytes to name
    name << " (" << bytes << " bytes)";

    return name.str();
  }

  return llvm::Instruction::getOpcodeName(opcode);
}

void InstructionCounter::instructionExecuted(
  const WorkItem *workItem, const llvm::Instruction *instruction,
  const TypedValue& result)
{
  unsigned opcode = instruction->getOpcode();

  // Check for loads and stores
  if (opcode == llvm::Instruction::Load || opcode == llvm::Instruction::Store)
  {
    // Track operations in separate address spaces
    bool load = (opcode == llvm::Instruction::Load);
    const llvm::Type *type = instruction->getOperand(load?0:1)->getType();
    unsigned addrSpace = type->getPointerAddressSpace();
    opcode = (load ? COUNTED_LOAD_BASE : COUNTED_STORE_BASE) + addrSpace;

    // Count total number of bytes loaded/stored
    unsigned bytes = getTypeSize(type->getPointerElementType());
    m_memopBytes[opcode-COUNTED_LOAD_BASE] += bytes;
  }
  else if (opcode == llvm::Instruction::Call)
  {
    // Track distinct function calls
    const llvm::CallInst *callInst = (const llvm::CallInst*)instruction;
    const llvm::Function *function = callInst->getCalledFunction();
    if (function)
    {
      vector<const llvm::Function*>::iterator itr =
        find(m_functions.begin(), m_functions.end(), function);
      if (itr == m_functions.end())
      {
        opcode = COUNTED_CALL_BASE + m_functions.size();
        m_functions.push_back(function);
      }
      else
      {
        opcode = COUNTED_CALL_BASE + (itr - m_functions.begin());
      }
    }
  }

  if (opcode >= m_instructionCounts.size())
  {
    m_instructionCounts.resize(opcode+1);
  }
  m_instructionCounts[opcode]++;
}

bool InstructionCounter::isThreadSafe() const
{
  return false;
}

void InstructionCounter::kernelBegin(const KernelInvocation *kernelInvocation)
{
  m_instructionCounts.clear();
  m_instructionCounts.resize(COUNTED_CALL_BASE);

  m_memopBytes.clear();
  m_memopBytes.resize(16);

  m_functions.clear();
}

void InstructionCounter::kernelEnd(const KernelInvocation *kernelInvocation)
{
  // Load default locale
  locale previousLocale = cout.getloc();
  locale defaultLocale("");
  cout.imbue(defaultLocale);

  cout << "Instructions executed for kernel '"
       << kernelInvocation->getKernel()->getName() << "':";
  cout << endl;

  // Generate list named instructions and their counts
  vector< pair<string,size_t> > namedCounts;
  for (unsigned i = 0; i < m_instructionCounts.size(); i++)
  {
    if (m_instructionCounts[i] == 0)
    {
      continue;
    }

    string name = getOpcodeName(i);
    if (name.compare(0, 14, "call llvm.dbg.") == 0)
    {
      continue;
    }

    namedCounts.push_back(make_pair(name, m_instructionCounts[i]));
  }

  // Sort named counts
  sort(namedCounts.begin(), namedCounts.end(), compareNamedCount);

  // Output sorted instruction counts
  for (unsigned i = 0; i < namedCounts.size(); i++)
  {
    cout << setw(16) << dec << namedCounts[i].second << " - "
         << namedCounts[i].first << endl;
  }

  cout << endl;

  // Restore locale
  cout.imbue(previousLocale);
}
