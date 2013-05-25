#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/InstrTypes.h"

#include "GlobalMemory.h"
#include "Kernel.h"
#include "WorkItem.h"

using namespace std;

WorkItem::WorkItem(const Kernel& kernel, GlobalMemory& globalMem,
                   size_t gid_x, size_t gid_y, size_t gid_z)
  : m_globalMemory(globalMem), m_debugOutput(false)
{
  m_globalID[0] = gid_x;
  m_globalID[1] = gid_y;
  m_globalID[2] = gid_z;

  // Store kernel arguments in private memory
  TypedValueMap::const_iterator aitr;
  for (aitr = kernel.args_begin(); aitr != kernel.args_end(); aitr++)
  {
    m_privateMemory[aitr->first] = clone(aitr->second);
  }
}

void WorkItem::dumpPrivateMemory() const
{
  cout << endl << "Work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2] << "):" << endl;

  TypedValueMap::const_iterator pmitr;
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

  // TODO: Is this necessary for ?Cmp instructions?
  if (instruction.getOpcode() == llvm::Instruction::ICmp ||
      instruction.getOpcode() == llvm::Instruction::FCmp)
  {
    resultSize = 1;
  }

  // TODO: Only allocate if not in map already?
  TypedValue result = {resultSize, new unsigned char[resultSize]};

  // Temporary: Dump instruction sequence (TODO: remove)
  if (m_debugOutput)
  {
    cout << "%" << (&instruction) << "(" << resultSize << ") = ";
    cout << left << setw(14) << instruction.getOpcodeName();
    llvm::User::const_op_iterator opitr;
    for (opitr = instruction.op_begin(); opitr != instruction.op_end(); opitr++)
    {
      cout << " %" << opitr->get();
    }
    cout << right << endl;
  }

  // Execute instruction
  switch (instruction.getOpcode())
  {
  case llvm::Instruction::Br:
    br(instruction);
    break;
  case llvm::Instruction::Call:
    // TODO: Currently assume call is get_global_id(0)
    *result.data = m_globalID[0];
    break;
  case llvm::Instruction::FAdd:
    *result.data = fadd(instruction);
    break;
  case llvm::Instruction::GetElementPtr:
    *result.data = gep(instruction);
    break;
  case llvm::Instruction::ICmp:
    *result.data = icmp(instruction);
    break;
  case llvm::Instruction::Load:
    load(instruction, result.data);
    break;
  case llvm::Instruction::Ret:
    // TODO: Cleaner handling of ret
    m_nextBlock = NULL;
    break;
  case llvm::Instruction::Store:
    store(instruction);
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

void WorkItem::br(const llvm::Instruction& instruction)
{
  if (instruction.getNumOperands() == 1)
  {
    // Unconditional branch
    m_nextBlock = instruction.getOperand(0);
  }
  else
  {
    // Conditional branch
    bool pred = *m_privateMemory[instruction.getOperand(0)].data;
    llvm::Value *iftrue = instruction.getOperand(2);
    llvm::Value *iffalse = instruction.getOperand(1);
    m_nextBlock = pred ? iftrue : iffalse;
  }
}

float WorkItem::fadd(const llvm::Instruction& instruction)
{
  float a = *m_privateMemory[instruction.getOperand(0)].data;
  float b = *m_privateMemory[instruction.getOperand(1)].data;
  return a + b;
}

size_t WorkItem::gep(const llvm::Instruction& instruction)
{
  // TODO: Use actual size of type
  size_t base  = *m_privateMemory[instruction.getOperand(0)].data;
  size_t offset = *m_privateMemory[instruction.getOperand(1)].data;
  return base + offset*4;
}

bool WorkItem::icmp(const llvm::Instruction& instruction)
{
  // Load operands
  llvm::CmpInst::Predicate pred = ((llvm::CmpInst*)&instruction)->getPredicate();
  unsigned int ua = *m_privateMemory[instruction.getOperand(0)].data;
  unsigned int ub = *m_privateMemory[instruction.getOperand(1)].data;
  int sa = *m_privateMemory[instruction.getOperand(0)].data;
  int sb = *m_privateMemory[instruction.getOperand(1)].data;

  switch (pred)
  {
  case llvm::CmpInst::ICMP_EQ:
    return ua == ub;
  case llvm::CmpInst::ICMP_NE:
    return ua != ub;
  case llvm::CmpInst::ICMP_UGT:
    return ua > ub;
  case llvm::CmpInst::ICMP_UGE:
    return ua >= ub;
  case llvm::CmpInst::ICMP_ULT:
    return ua < ub;
  case llvm::CmpInst::ICMP_ULE:
    return ua <= ub;
  case llvm::CmpInst::ICMP_SGT:
    return sa > sb;
  case llvm::CmpInst::ICMP_SGE:
    return sa >= sb;
  case llvm::CmpInst::ICMP_SLT:
    return sa < sb;
  case llvm::CmpInst::ICMP_SLE:
    return sa <= sb;
  default:
    cout << "Unhandled ICmp predicated." << endl;
    return false;
  }
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
