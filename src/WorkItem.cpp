#include "config.h"
#include <iomanip>
#include <iostream>
#include <map>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"

#include "GlobalMemory.h"
#include "Kernel.h"
#include "WorkItem.h"

using namespace std;

WorkItem::WorkItem(const Kernel& kernel, GlobalMemory& globalMem,
                   size_t gid_x, size_t gid_y, size_t gid_z)
  : m_globalMemory(globalMem)
{
  m_globalID[0] = gid_x;
  m_globalID[1] = gid_y;
  m_globalID[2] = gid_z;

  // Store kernel arguments in private memory
  KernelArgs::const_iterator aitr;
  for (aitr = kernel.args_begin(); aitr != kernel.args_end(); aitr++)
  {
    PrivateVariable arg = {4, new unsigned char[4]};
    *arg.data = aitr->second;
    m_privateMemory[aitr->first] = arg;
  }
}

void WorkItem::dumpPrivateMemory() const
{
  cout << endl << "Work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2] << "):" << endl;

  PrivateMemory::const_iterator pmitr;
  for (pmitr = m_privateMemory.begin(); pmitr != m_privateMemory.end(); pmitr++)
  {
    // Output symbolic name if available
    cout << setw(16) << setfill(' ');
    if (pmitr->first->hasName())
    {
      cout << pmitr->first->getName().str() << ":";
    }
    else
    {
      cout << pmitr->first << ":";
    }

    // TODO: Interpret type?
    // TODO: Deal with larger private variables (e.g. arrays)
    for (int i = 0; i < pmitr->second.size; i++)
    {
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)pmitr->second.data[i];
    }
    cout << setw(0) << endl;
  }
}

void WorkItem::execute(const llvm::Instruction& instruction)
{
  // Prepare private variable for instruction result
  size_t resultSize = instruction.getType()->getPrimitiveSizeInBits() >> 3;
  // TODO: Is this necessary for GEP?
  if (instruction.getOpcode() == llvm::Instruction::GetElementPtr)
  {
    // TODO: Configurable pointer size
    resultSize = 4;
  }

  // TODO: Only allocate if not in map already?
  PrivateVariable result = {resultSize, new unsigned char[resultSize]};

  // Temporary: Dump instruction sequence (TODO: remove)
  if (resultSize > 0)
  {
    cout << "%" << (&instruction) << "(" << resultSize << ") = ";
  }
  cout << left << setw(14) << instruction.getOpcodeName();
  llvm::User::const_op_iterator opitr;
  for (opitr = instruction.op_begin(); opitr != instruction.op_end(); opitr++)
  {
    cout << " %" << opitr->get();
  }
  cout << right << endl;

  // Execute instruction
  switch (instruction.getOpcode())
  {
  case llvm::Instruction::Call:
    // TODO: Currently assume call is get_global_id(0)
    *result.data = m_globalID[0];
    break;
  case llvm::Instruction::GetElementPtr:
    *result.data = getElementPtr(instruction);
    break;
  case llvm::Instruction::Load:
    load(instruction, result.data);
    break;
  case llvm::Instruction::Store:
    store(instruction);
    break;
  case llvm::Instruction::FAdd:
    *result.data = FAdd(instruction);
    break;
  case llvm::Instruction::Ret:
    break;
  default:
    cout << "Unhandled instruction: " << instruction.getOpcodeName() << endl;
    break;
  }

  // Store result
  if (resultSize > 0)
  {
    m_privateMemory[&instruction] = result;
  }
}

////////////////////////////////
//// Instruction execution  ////
////////////////////////////////

float WorkItem::FAdd(const llvm::Instruction& instruction)
{
  float a = *m_privateMemory[instruction.getOperand(0)].data;
  float b = *m_privateMemory[instruction.getOperand(1)].data;
  return a + b;
}

size_t WorkItem::getElementPtr(const llvm::Instruction& instruction)
{
  size_t base  = *m_privateMemory[instruction.getOperand(0)].data;
  size_t offset = *m_privateMemory[instruction.getOperand(1)].data;
  return base + offset;
}

void WorkItem::load(const llvm::Instruction& instruction,
                    unsigned char *dest)
{
  // TODO: Load correct amount of data
  // TODO: Endian-ness?
  size_t address = *m_privateMemory[instruction.getOperand(0)].data;
  for (int i = 0; i < 4; i++)
  {
    dest[i] = m_globalMemory.load(address + i);
  }
}

void WorkItem::store(const llvm::Instruction& instruction)
{
  // TODO: Store correct amount of data
  // TODO: Endian-ness?
  unsigned char *data = m_privateMemory[instruction.getOperand(0)].data;
  size_t address = *m_privateMemory[instruction.getOperand(1)].data;
  for (int i = 0; i < 4; i++)
  {
    m_globalMemory.store(address + i, data[i]);
  }
}
