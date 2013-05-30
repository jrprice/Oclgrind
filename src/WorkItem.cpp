#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/DebugInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

#include "Kernel.h"
#include "Memory.h"
#include "WorkItem.h"

using namespace std;

WorkItem::WorkItem(const Kernel& kernel, Memory& globalMem,
                   size_t gid_x, size_t gid_y, size_t gid_z)
  : m_globalMemory(globalMem), m_debugOutput(false)
{
  m_globalID[0] = gid_x;
  m_globalID[1] = gid_y;
  m_globalID[2] = gid_z;

  // Store kernel arguments in private memory
  TypedValueMap::const_iterator argItr;
  for (argItr = kernel.args_begin(); argItr != kernel.args_end(); argItr++)
  {
    m_privateMemory[argItr->first] = clone(argItr->second);
  }

  m_stack = new Memory();

  m_prevBlock = NULL;
  m_currBlock = NULL;
  m_nextBlock = NULL;
}

WorkItem::~WorkItem()
{
  // Free private memory
  TypedValueMap::iterator pmItr;
  for (pmItr = m_privateMemory.begin();
       pmItr != m_privateMemory.end(); pmItr++)
  {
    delete[] pmItr->second.data;
  }
}

void WorkItem::dumpPrivateMemory() const
{
  cout << endl << "Work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2]
       << ") Private Memory:" << endl;

  TypedValueMap::const_iterator pmItr;
  for (pmItr = m_privateMemory.begin();
       pmItr != m_privateMemory.end(); pmItr++)
  {
    // Output symbolic name
    cout << setw(12) << setfill(' ') << left;
    cout << pmItr->first->getName().str() << right << ":";

    // Output bytes
    for (int i = 0; i < pmItr->second.size; i++)
    {
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)pmItr->second.data[i];
    }

    // TODO: Interpret values?

    cout << setw(0) << endl;
  }

  // Dump stack contents
  cout << endl << "Stack:";
  m_stack->dump();
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

  // Dump instruction sequence
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
  case llvm::Instruction::Alloca:
    alloca(instruction);
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
                                 unsigned addressSpace,
                                 size_t address, size_t size) const
{
  std::string memType;
  switch (addressSpace)
  {
  case 0:
    memType = "private";
    break;
  case 1:
    memType = "global";
    break;
  default:
    assert(false && "Memory error in unsupported address space.");
    break;
  }

  cout << endl << msg
       << " of size " << size
       << " at " << memType
       << " memory address " << hex << address
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

void WorkItem::alloca(const llvm::Instruction& instruction)
{
  const llvm::AllocaInst *allocInst = ((const llvm::AllocaInst*)&instruction);
  const llvm::Type *type = allocInst->getAllocatedType();

  // TODO: Handle allocations for non-arrays
  unsigned elementSize = type->getArrayElementType()->getScalarSizeInBits()>>3;
  unsigned numElements = type->getArrayNumElements();

  // Perform allocation
  size_t address = m_stack->allocateBuffer(elementSize*numElements);

  // Create pointer to alloc'd memory
  TypedValue result;
  result.size = sizeof(size_t);
  result.data = new unsigned char[result.size];
  *((size_t*)result.data) = address;
  m_privateMemory[&instruction] = result;
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
    bool pred = *((bool*)m_privateMemory[instruction.getOperand(0)].data);
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

  // Get base address
  const llvm::Value *baseOperand = gepInst->getPointerOperand();
  size_t address = *((size_t*)m_privateMemory[baseOperand].data);
  llvm::Type *ptrType = gepInst->getPointerOperandType();
  assert(ptrType->isPointerTy());

  // Iterate over indices
  llvm::User::const_op_iterator opItr;
  for (opItr = gepInst->idx_begin(); opItr != gepInst->idx_end(); opItr++)
  {
    size_t offset;
    if (isConstantOperand(*opItr))
    {
      // TODO: Is this a valid method of extracting offset?
      // TODO: Probably not - negative offsets
      offset = ((llvm::ConstantInt*)opItr->get())->getLimitedValue();
    }
    else
    {
      // TODO: Use type of offset
      offset = *((int*)m_privateMemory[opItr->get()].data);
    }

    // Get pointer element size
    size_t size;
    llvm::Type *elemType = ptrType->getPointerElementType();
    if (elemType->isArrayTy())
    {
      size_t num = elemType->getArrayNumElements();
      size_t sz = elemType->getArrayElementType()->getScalarSizeInBits() >> 3;
      size = num*sz;
    }
    else
    {
      size = elemType->getScalarSizeInBits() >> 3;
    }

    ptrType = elemType;
    address += offset*size;
  }

  *((size_t*)result.data) = address;
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
  bool a = *((bool*)m_privateMemory[instruction.getOperand(0)].data);
  bool b = *((bool*)m_privateMemory[instruction.getOperand(1)].data);
  *((bool*)result.data) = a && b;
}

void WorkItem::load(const llvm::Instruction& instruction,
                    TypedValue& result)
{
  const llvm::LoadInst *loadInst = (const llvm::LoadInst*)&instruction;
  const llvm::Value *ptrOp = loadInst->getPointerOperand();
  unsigned addressSpace = loadInst->getPointerAddressSpace();

  // Get address
  size_t address = *((size_t*)m_privateMemory[ptrOp].data);

  // TODO: Find or create enum for address spaces
  Memory *memory;
  switch (addressSpace)
  {
  case 0: // Private memory
    memory = m_stack;
    break;
  case 1: // Global memory
    memory = &m_globalMemory;
    break;
  default:
    cout << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  // Load data
  if (!memory->load(address, result.size, result.data))
  {
    outputMemoryError(instruction, "Invalid read",
                      addressSpace, address, result.size);
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
  const llvm::StoreInst *storeInst = (const llvm::StoreInst*)&instruction;
  const llvm::Value *ptrOp = storeInst->getPointerOperand();
  const llvm::Value *valOp = storeInst->getValueOperand();
  unsigned addressSpace = storeInst->getPointerAddressSpace();

  // Get address
  size_t address = *((size_t*)m_privateMemory[ptrOp].data);

  // TODO: Genericise operand handling
  size_t size = valOp->getType()->getPrimitiveSizeInBits() >> 3;
  unsigned char *data = new unsigned char[size];

  switch (valOp->getValueID())
  {
  case llvm::Value::ConstantFPVal:
    if (size == 4)
    {
      (*(float*)data) =
        ((llvm::ConstantFP*)valOp)->getValueAPF().convertToFloat();
    }
    else if (size == 8)
    {
      (*(double*)data) =
        ((llvm::ConstantFP*)valOp)->getValueAPF().convertToDouble();
    }
    else
    {
      cout << "Unhandled APFloat size." << endl;
    }
    break;
  case llvm::Value::ConstantIntVal:
    memcpy((uint64_t*)data,
           ((llvm::ConstantInt*)valOp)->getValue().getRawData(),
           size);
    break;
  default:
    if (m_privateMemory.find(valOp) != m_privateMemory.end())
    {
      // TODO: Cleaner solution for this
      memcpy(data, m_privateMemory[valOp].data, size);
    }
    else
    {
      cout << "Unhandled operand type." << endl;
    }
    break;
  }

  Memory *memory;

  // TODO: Find or create address space enum
  switch (addressSpace)
  {
  case 0: // Private memory
    memory = m_stack;
    break;
  case 1: // Global memory
    memory = &m_globalMemory;
    break;
  default:
    cout << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  // Store data
  if (!memory->store(address, size, data))
  {
    outputMemoryError(instruction, "Invalid write",
                      addressSpace, address, size);
  }

  delete[] data;
}
