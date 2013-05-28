#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/DebugInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
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

  m_prevBlock = NULL;
  m_currBlock = NULL;
  m_nextBlock = NULL;
}

WorkItem::~WorkItem()
{
  // Free private memory
  TypedValueMap::iterator pmitr;
  for (pmitr = m_privateMemory.begin();
       pmitr != m_privateMemory.end(); pmitr++)
  {
    delete[] pmitr->second.data;
  }
}

void WorkItem::dumpPrivateMemory() const
{
  cout << endl << "Work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2]
       << ") Private Memory:" << endl;

  TypedValueMap::const_iterator pmitr;
  for (pmitr = m_privateMemory.begin();
       pmitr != m_privateMemory.end(); pmitr++)
  {
    // Output symbolic name
    cout << setw(12) << setfill(' ') << left;
    cout << pmitr->first->getName().str() << right << ":";

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

void WorkItem::enableDebugOutput(bool enable)
{
  m_debugOutput = enable;
}

void WorkItem::execute(const llvm::Instruction& instruction)
{
  // Prepare private variable for instruction result
  size_t resultSize = getInstructionResultSize(instruction);

  // Allocate result memory if not in map already
  TypedValue result = {resultSize, NULL};
  if (m_privateMemory.find(&instruction) == m_privateMemory.end())
  {
    result.data = new unsigned char[resultSize];
  }
  else
  {
    assert(result.size == m_privateMemory[&instruction].size);
    result.data = m_privateMemory[&instruction].data;
  }

  // Temporary: Dump instruction sequence (TODO: remove)
  if (m_debugOutput)
  {
    dumpInstruction(instruction, true);
  }

  // Execute instruction
  switch (instruction.getOpcode())
  {
  case llvm::Instruction::Add:
    add(instruction, result);
    break;
  case llvm::Instruction::And:
    land(instruction, result);
    break;
  case llvm::Instruction::Br:
    br(instruction);
    break;
  case llvm::Instruction::Call:
    call(instruction, result);
    break;
  case llvm::Instruction::FAdd:
    fadd(instruction, result);
    break;
  case llvm::Instruction::FMul:
    fmul(instruction, result);
    break;
  case llvm::Instruction::GetElementPtr:
    gep(instruction, result);
    break;
  case llvm::Instruction::ICmp:
    icmp(instruction, result);
    break;
  case llvm::Instruction::Load:
    load(instruction, result);
    break;
  case llvm::Instruction::Mul:
    mul(instruction, result);
    break;
  case llvm::Instruction::PHI:
    phi(instruction, result);
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

const size_t* WorkItem::getGlobalID() const
{
  return m_globalID;
}

const llvm::Value* WorkItem::getNextBlock() const
{
  return m_nextBlock;
}

void WorkItem::outputMemoryError(const llvm::Instruction& instruction,
                                 const std::string& msg,
                                 size_t address, size_t size) const
{
  cout << endl << msg
       << " of size " << size
       << " at address " << hex << address
       << " by work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2] << ")"
       << endl << "\t";
  dumpInstruction(instruction);

  // Output debug information
  cout << "\t";
  llvm::MDNode *md = instruction.getMetadata("dbg");
  if (!md)
  {
    cout << "Debugging information not available." << endl;
  }
  else
  {
    llvm::DILocation loc(md);
    cout << "At line " << dec << loc.getLineNumber()
         << " of " << loc.getFilename().str() << endl;
  }
  cout << endl;
}

void WorkItem::setCurrentBlock(const llvm::Value *block)
{
  m_prevBlock = m_currBlock;
  m_currBlock = block;
  m_nextBlock = NULL;
}

////////////////////////////////
//// Instruction execution  ////
////////////////////////////////

void WorkItem::add(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: 64-bit, unsigned
  // TODO: constants
  int a, b;
  const llvm::Value *opA = instruction.getOperand(0);
  const llvm::Value *opB = instruction.getOperand(1);

  if (isConstantOperand(opA))
  {
    a = ((llvm::ConstantInt*)opA)->getSExtValue();
  }
  else
  {
    a = *((int*)m_privateMemory[opA].data);
  }

  if (isConstantOperand(opB))
  {
    b = ((llvm::ConstantInt*)opB)->getSExtValue();
  }
  else
  {
    b = *((int*)m_privateMemory[opB].data);
  }

  *((int*)result.data) = (a + b);
}

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

void WorkItem::call(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::CallInst *callInst = (const llvm::CallInst*)&instruction;

  // TODO: Currently assume call is get_global_id()
  if (result.size == 0)
  {
    return;
  }
  llvm::ConstantInt *operand = (llvm::ConstantInt*)callInst->getArgOperand(0);
  int dim = operand->getLimitedValue();
  *result.data = m_globalID[dim];
}

void WorkItem::fadd(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: double
  // TODO: constants
  float a = *((float*)m_privateMemory[instruction.getOperand(0)].data);
  float b = *((float*)m_privateMemory[instruction.getOperand(1)].data);
  *((float*)result.data) = (a + b);
}

void WorkItem::fmul(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: double
  // TODO: constants
  float a = *((float*)m_privateMemory[instruction.getOperand(0)].data);
  float b = *((float*)m_privateMemory[instruction.getOperand(1)].data);
  *((float*)result.data) = (a * b);
}

void WorkItem::gep(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::GetElementPtrInst *gepInst =
    (const llvm::GetElementPtrInst*)&instruction;

  const llvm::Value *baseOperand = gepInst->getPointerOperand();
  size_t base = *m_privateMemory[baseOperand].data;

  // TODO: Multiple indices (use GEP instruction)
  size_t offset;
  const llvm::Value *offsetOperand = instruction.getOperand(1);
  if (isConstantOperand(offsetOperand))
  {
    // TODO: Is this a valid method of extracting offset?
    // TODO: Probably not - negative offsets?
    offset = ((llvm::ConstantInt*)offsetOperand)->getLimitedValue();
  }
  else
  {
    offset = *m_privateMemory[offsetOperand].data;
  }

  // Get element size
  const llvm::Type *ptrType = gepInst->getPointerOperandType();
  size_t size = ptrType->getPointerElementType()->getPrimitiveSizeInBits()>>3;

  *((size_t*)result.data) = base + offset*size;
}

void WorkItem::icmp(const llvm::Instruction& instruction, TypedValue& result)
{
  // Load operands
  // TODO: 64-bit
  llvm::CmpInst::Predicate pred = ((llvm::CmpInst&)instruction).getPredicate();

  llvm::Value *opA = instruction.getOperand(0);
  llvm::Value *opB = instruction.getOperand(1);
  unsigned int ua, ub;
  int sa, sb;

  if (isConstantOperand(opA))
  {
    ua = ((llvm::ConstantInt*)opA)->getZExtValue();
    sa = ((llvm::ConstantInt*)opA)->getSExtValue();
  }
  else
  {
    ua = *((unsigned int*)m_privateMemory[opA].data);
    sa = *((int*)m_privateMemory[opA].data);
  }

  if (isConstantOperand(opB))
  {
    ub = ((llvm::ConstantInt*)opB)->getZExtValue();
    sb = ((llvm::ConstantInt*)opB)->getSExtValue();
  }
  else
  {
    ub = *((unsigned int*)m_privateMemory[opB].data);
    sb = *((int*)m_privateMemory[opB].data);
  }

  bool b;
  switch (pred)
  {
  case llvm::CmpInst::ICMP_EQ:
    b = ua == ub;
    break;
  case llvm::CmpInst::ICMP_NE:
    b = ua != ub;
    break;
  case llvm::CmpInst::ICMP_UGT:
    b = ua > ub;
    break;
  case llvm::CmpInst::ICMP_UGE:
    b = ua >= ub;
    break;
  case llvm::CmpInst::ICMP_ULT:
    b = ua < ub;
    break;
  case llvm::CmpInst::ICMP_ULE:
    b = ua <= ub;
    break;
  case llvm::CmpInst::ICMP_SGT:
    b = sa > sb;
    break;
  case llvm::CmpInst::ICMP_SGE:
    b = sa >= sb;
    break;
  case llvm::CmpInst::ICMP_SLT:
    b = sa < sb;
    break;
  case llvm::CmpInst::ICMP_SLE:
    b = sa <= sb;
    break;
  default:
    cout << "Unhandled ICmp predicated." << endl;
    break;
  }

  *((bool*)result.data) = b;
}

void WorkItem::land(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Constant operands?
  bool a = *m_privateMemory[instruction.getOperand(0)].data;
  bool b = *m_privateMemory[instruction.getOperand(1)].data;
  *((bool*)result.data) = a && b;
}

void WorkItem::load(const llvm::Instruction& instruction,
                    TypedValue& result)
{
  // TODO: Endian-ness?
  size_t address = *m_privateMemory[instruction.getOperand(0)].data;
  if (!m_globalMemory.load(address, result.size, result.data))
  {
    outputMemoryError(instruction, "Invalid read", address, result.size);
  }
}

void WorkItem::mul(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: 64-bit, unsigned
  // TODO: constants
  int a, b;
  const llvm::Value *opA = instruction.getOperand(0);
  const llvm::Value *opB = instruction.getOperand(1);

  if (isConstantOperand(opA))
  {
    a = ((llvm::ConstantInt*)opA)->getSExtValue();
  }
  else
  {
    a = *((int*)m_privateMemory[opA].data);
  }

  if (isConstantOperand(opB))
  {
    b = ((llvm::ConstantInt*)opB)->getSExtValue();
  }
  else
  {
    b = *((int*)m_privateMemory[opB].data);
  }

  *((int*)result.data) = (a * b);
}

void WorkItem::phi(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::PHINode *phiNode = (llvm::PHINode*)&instruction;
  const llvm::Value *value = phiNode->getIncomingValueForBlock((const llvm::BasicBlock*)m_prevBlock);
  if (isConstantOperand(value))
  {
    // TODO
    *((int*)result.data) = 0;
  }
  else
  {
    memcpy(result.data, m_privateMemory[value].data, result.size);
  }
}

void WorkItem::store(const llvm::Instruction& instruction)
{
  // TODO: Address space
  size_t address = *m_privateMemory[instruction.getOperand(1)].data;

  // TODO: Genericise operand handling
  const llvm::Value *value = instruction.getOperand(0);
  size_t size = value->getType()->getPrimitiveSizeInBits() >> 3;
  unsigned char *data = new unsigned char[size];

  switch (value->getValueID())
  {
  case llvm::Value::ConstantFPVal:
    if (size == 4)
    {
      (*(float*)data) =
        ((llvm::ConstantFP*)value)->getValueAPF().convertToFloat();
    }
    else if (size == 8)
    {
      (*(double*)data) =
        ((llvm::ConstantFP*)value)->getValueAPF().convertToDouble();
    }
    else
    {
      cout << "Unhandled APFloat size." << endl;
    }
    break;
  case llvm::Value::ConstantIntVal:
    memcpy((uint64_t*)data,
           ((llvm::ConstantInt*)value)->getValue().getRawData(),
           size);
    break;
  default:
    if (m_privateMemory.find(value) != m_privateMemory.end())
    {
      // TODO: Cleaner solution for this
      memcpy(data, m_privateMemory[value].data, size);
    }
    else
    {
      cout << "Unhandled operand type." << endl;
    }
    break;
  }

  // Store value
  if (!m_globalMemory.store(address, size, data))
  {
    outputMemoryError(instruction, "Invalid write", address, size);
  }

  delete[] data;
}
