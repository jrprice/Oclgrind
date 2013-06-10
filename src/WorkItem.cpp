#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/DebugInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Type.h"

#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace std;

WorkItem::WorkItem(WorkGroup& workGroup,
                   const Kernel& kernel, Memory& globalMem,
                   size_t lid_x, size_t lid_y, size_t lid_z)
  : m_workGroup(workGroup), m_kernel(kernel),
    m_globalMemory(globalMem), m_debugOutput(false)
{
  m_localID[0] = lid_x;
  m_localID[1] = lid_y;
  m_localID[2] = lid_z;

  // Compute global ID
  const size_t *groupID = workGroup.getGroupID();
  const size_t *groupSize = workGroup.getGroupSize();
  m_globalID[0] = lid_x + groupID[0]*groupSize[0];
  m_globalID[1] = lid_y + groupID[1]*groupSize[1];
  m_globalID[2] = lid_z + groupID[2]*groupSize[2];

  // Store kernel arguments in private memory
  TypedValueMap::const_iterator argItr;
  for (argItr = kernel.args_begin(); argItr != kernel.args_end(); argItr++)
  {
    m_privateMemory[argItr->first] = clone(argItr->second);
  }

  m_stack = new Memory();

  m_prevBlock = NULL;
  m_nextBlock = NULL;
  m_currBlock = kernel.getFunction()->begin();
  m_currInst = m_currBlock->begin();

  m_state = READY;
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

void WorkItem::clearBarrier()
{
  if (m_state == BARRIER)
  {
    m_state = READY;
  }
}

void WorkItem::dumpPrivateMemory() const
{
  cout << endl << "Work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2]
       << ") Private Memory:" << endl;

  map<string,const llvm::Value*>::const_iterator varItr;
  for (varItr = m_variables.begin(); varItr != m_variables.end(); varItr++)
  {
    // Check variable has an assigned value
    const llvm::Value *value = varItr->second;
    TypedValueMap::const_iterator itr = m_privateMemory.find(value);
    if (itr == m_privateMemory.end())
    {
      continue;
    }

    // Output synbolic name
    cout << setw(16) << setfill(' ') << left;
    cout << varItr->first << right << ":";

    // Output bytes
    const TypedValue result = itr->second;
    for (int i = 0; i < result.size; i++)
    {
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)result.data[i];
    }

    // Interpret value
    const llvm::Type::TypeID type = value->getType()->getTypeID();
    switch (type)
    {
    case llvm::Type::IntegerTyID:
      cout << " (" << getIntValue(value) << ")";
      break;
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
      cout << " (" << getFloatValue(value) << ")";
      break;
    default:
      break;
    }

    cout << setw(0) << endl;
  }

  // Dump stack contents
  if (m_stack->getSize() > 0)
  {
    cout << endl << "Stack:";
    m_stack->dump();
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

  // TODO: Bitcasting should happen somewhere...
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
    bwand(instruction, result);
    break;
  case llvm::Instruction::AShr:
    ashr(instruction, result);
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
  case llvm::Instruction::FCmp:
    fcmp(instruction, result);
    break;
  case llvm::Instruction::FDiv:
    fdiv(instruction, result);
    break;
  case llvm::Instruction::FMul:
    fmul(instruction, result);
    break;
  case llvm::Instruction::FRem:
    frem(instruction, result);
    break;
  case llvm::Instruction::FSub:
    fsub(instruction, result);
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
  case llvm::Instruction::LShr:
    lshr(instruction, result);
    break;
  case llvm::Instruction::Mul:
    mul(instruction, result);
    break;
  case llvm::Instruction::Or:
    bwor(instruction, result);
    break;
  case llvm::Instruction::PHI:
    phi(instruction, result);
    break;
  case llvm::Instruction::Ret:
    // TODO: ret from functions that aren't the kernel
    m_nextBlock = NULL;
    break;
  case llvm::Instruction::SDiv:
    sdiv(instruction, result);
    break;
  case llvm::Instruction::Select:
    select(instruction, result);
    break;
  case llvm::Instruction::SExt:
    sext(instruction, result);
    break;
  case llvm::Instruction::Shl:
    shl(instruction, result);
    break;
  case llvm::Instruction::SRem:
    srem(instruction, result);
    break;
  case llvm::Instruction::Store:
    store(instruction);
    break;
  case llvm::Instruction::Trunc:
    trunc(instruction, result);
    break;
  case llvm::Instruction::UDiv:
    udiv(instruction, result);
    break;
  case llvm::Instruction::URem:
    urem(instruction, result);
    break;
  case llvm::Instruction::Xor:
    bwxor(instruction, result);
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

double WorkItem::getFloatValue(const llvm::Value *operand) const
{
  double val = 0;
  if (isConstantOperand(operand))
  {
    llvm::APFloat apf = ((const llvm::ConstantFP*)operand)->getValueAPF();
    if (&(apf.getSemantics()) == &(llvm::APFloat::IEEEsingle))
    {
      val = apf.convertToFloat();
    }
    else if (&(apf.getSemantics()) == &(llvm::APFloat::IEEEdouble))
    {
      val = apf.convertToDouble();
    }
    else
    {
      cout << "Unhandled float semantics." << endl;
      return 0;
    }
  }
  else
  {
    TypedValue op = m_privateMemory.at(operand);
    if (op.size == sizeof(float))
    {
      val = *((float*)op.data);
    }
    else if (op.size == sizeof(double))
    {
      val = *((double*)op.data);
    }
    else
    {
      cout << "Unhandled float size: " << op.size << endl;
      return 0;
    }
  }
  return val;
}

uint64_t WorkItem::getIntValue(const llvm::Value *operand) const
{
  uint64_t val = 0;
  if (isConstantOperand(operand))
  {
    // TODO: Signed constants
    val = ((const llvm::ConstantInt*)operand)->getZExtValue();
  }
  else
  {
    TypedValue op = m_privateMemory.at(operand);
    memcpy(&val, op.data, op.size);
  }
  return val;
}

WorkItem::State WorkItem::getState() const
{
  return m_state;
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
  case 3:
    memType = "local";
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
    // TODO: Filename not working for new SPIR output?
    llvm::DILocation loc(md);
    cout << "At line " << dec << loc.getLineNumber()
         << " of " << loc.getFilename().str() << endl;
  }
  cout << endl;
}

void WorkItem::setFloatResult(TypedValue& result, double val) const
{
  if (result.size == sizeof(float))
  {
    *((float*)result.data) = val;
  }
  else if (result.size == sizeof(double))
  {
    *((double*)result.data) = val;
  }
  else
  {
    cout << "Unhandled float size: " << result.size << endl;
  }
}

WorkItem::State WorkItem::step(bool debugOutput)
{
  assert(m_state == READY);

  if (debugOutput)
  {
    dumpInstruction(*m_currInst, true);
  }

  // Execute the next instruction
  execute(*m_currInst);

  // Check if we've reached the end of the block
  if (++m_currInst == m_currBlock->end())
  {
    if (m_nextBlock)
    {
      // Move to next basic block
      m_prevBlock = m_currBlock;
      m_currBlock = m_nextBlock;
      m_nextBlock = NULL;
      m_currInst = m_currBlock->begin();
    }
    else
    {
      // We've reached the end of the function
      m_state = FINISHED;
    }
  }

  return m_state;
}

void WorkItem::updateVariable(const llvm::DbgValueInst *instruction)
{
  const llvm::Value *value = instruction->getValue();
  const llvm::MDNode *variable = instruction->getVariable();
  uint64_t offset = instruction->getOffset();

  const llvm::MDString *var = ((const llvm::MDString*)variable->getOperand(2));
  std::string name = var->getString().str();
  m_variables[name] = value;
}

////////////////////////////////
//// Instruction execution  ////
////////////////////////////////

void WorkItem::add(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Signed?
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a + b;
  memcpy(result.data, &r, result.size);
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

void WorkItem::ashr(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Sign extension
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a >> b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::br(const llvm::Instruction& instruction)
{
  if (instruction.getNumOperands() == 1)
  {
    // Unconditional branch
    m_nextBlock = (const llvm::BasicBlock*)instruction.getOperand(0);
  }
  else
  {
    // Conditional branch
    bool pred = *((bool*)m_privateMemory[instruction.getOperand(0)].data);
    const llvm::Value *iftrue = instruction.getOperand(2);
    const llvm::Value *iffalse = instruction.getOperand(1);
    m_nextBlock = (const llvm::BasicBlock*)(pred ? iftrue : iffalse);
  }
}

void WorkItem::bwand(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a & b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::bwor(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a | b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::bwxor(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a ^ b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::call(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::CallInst *callInst = (const llvm::CallInst*)&instruction;

  // TODO: Handle indirect function calls
  if (!callInst->getCalledFunction())
  {
    // Resolve indirect function pointer
    const llvm::Value *func = callInst->getCalledValue();
    const llvm::Value *funcPtr = ((const llvm::User*)func)->getOperand(0);
    const llvm::Function *function = (const llvm::Function*)funcPtr;
    cout << "Unhandled indirect function call: "
         << function->getName().str() << endl;
    return;
  }

  const llvm::Function *function = callInst->getCalledFunction();
  const string name = function->getName().str();

  // TODO: Implement more builtin functions
  // TODO: Cleaner implementation of this?
  if (name == "barrier")
  {
    // TODO: Different types of barrier?
    m_state = BARRIER;
  }
  else if (name == "get_global_id")
  {
    uint64_t dim = getIntValue(callInst->getArgOperand(0));
    assert(dim < 3);
    *((size_t*)result.data) = m_globalID[dim];
  }
  else if (name == "get_global_size")
  {
    uint64_t dim = getIntValue(callInst->getArgOperand(0));
    assert(dim < 3);
    *((size_t*)result.data) = m_kernel.getGlobalSize()[dim];
  }
  else if (name == "get_group_id")
  {
    uint64_t dim = getIntValue(callInst->getArgOperand(0));
    assert(dim < 3);
    *((size_t*)result.data) = m_workGroup.getGroupID()[dim];
  }
  else if (name == "get_local_id")
  {
    uint64_t dim = getIntValue(callInst->getArgOperand(0));
    assert(dim < 3);
    *((size_t*)result.data) = m_localID[dim];
  }
  else if (name == "get_local_size")
  {
    uint64_t dim = getIntValue(callInst->getArgOperand(0));
    assert(dim < 3);
    *((size_t*)result.data) = m_workGroup.getGroupSize()[dim];
  }
  else if (name == "min")
  {
    // TODO: Non-integer overloads
    uint64_t a = getIntValue(callInst->getArgOperand(0));
    uint64_t b = getIntValue(callInst->getArgOperand(1));
    uint64_t r = min(a,b);
    memcpy(result.data, &r, result.size);
  }
  else if (name == "llvm.dbg.value")
  {
    updateVariable((const llvm::DbgValueInst*)callInst);
  }
  else
  {
    cout << "Unhandled direct function call: " << name << endl;
  }
}

void WorkItem::fadd(const llvm::Instruction& instruction, TypedValue& result)
{
  double a = getFloatValue(instruction.getOperand(0));
  double b = getFloatValue(instruction.getOperand(1));
  setFloatResult(result, a + b);
}

void WorkItem::fcmp(const llvm::Instruction& instruction, TypedValue& result)
{
  llvm::CmpInst::Predicate pred = ((llvm::CmpInst&)instruction).getPredicate();
  double a = getFloatValue(instruction.getOperand(0));
  double b = getFloatValue(instruction.getOperand(1));

  // TODO: Consider nans in ordered comparisons?
  // TODO: Implemented unordered comparisons
  uint64_t r;
  switch (pred)
  {
  case llvm::CmpInst::FCMP_OEQ:
    r = a == b;
    break;
  case llvm::CmpInst::FCMP_ONE:
    r = a != b;
    break;
  case llvm::CmpInst::FCMP_OGT:
    r = a > b;
    break;
  case llvm::CmpInst::FCMP_OGE:
    r = a >= b;
    break;
  case llvm::CmpInst::FCMP_OLT:
    r = a < b;
    break;
  case llvm::CmpInst::FCMP_OLE:
    r = a <= b;
    break;
  default:
    cout << "Unhandled FCmp predicate." << endl;
    break;
  }

  memcpy(result.data, &r, result.size);
}

void WorkItem::fdiv(const llvm::Instruction& instruction, TypedValue& result)
{
  double a = getFloatValue(instruction.getOperand(0));
  double b = getFloatValue(instruction.getOperand(1));
  setFloatResult(result, a / b);
}

void WorkItem::fmul(const llvm::Instruction& instruction, TypedValue& result)
{
  double a = getFloatValue(instruction.getOperand(0));
  double b = getFloatValue(instruction.getOperand(1));
  setFloatResult(result, a * b);
}

void WorkItem::frem(const llvm::Instruction& instruction, TypedValue& result)
{
  double a = getFloatValue(instruction.getOperand(0));
  double b = getFloatValue(instruction.getOperand(1));
  setFloatResult(result, fmod(a, b));
}

void WorkItem::fsub(const llvm::Instruction& instruction, TypedValue& result)
{
  double a = getFloatValue(instruction.getOperand(0));
  double b = getFloatValue(instruction.getOperand(1));
  setFloatResult(result, a - b);
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
    // TODO: Signed?
    uint64_t offset = getIntValue(opItr->get());

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
  llvm::CmpInst::Predicate pred = ((llvm::CmpInst&)instruction).getPredicate();

  // Load operands
  // TODO: Use getIntValue()?
  llvm::Value *opA = instruction.getOperand(0);
  llvm::Value *opB = instruction.getOperand(1);
  uint64_t ua, ub;
  uint64_t sa, sb;

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

  uint64_t r;
  switch (pred)
  {
  case llvm::CmpInst::ICMP_EQ:
    r = ua == ub;
    break;
  case llvm::CmpInst::ICMP_NE:
    r = ua != ub;
    break;
  case llvm::CmpInst::ICMP_UGT:
    r = ua > ub;
    break;
  case llvm::CmpInst::ICMP_UGE:
    r = ua >= ub;
    break;
  case llvm::CmpInst::ICMP_ULT:
    r = ua < ub;
    break;
  case llvm::CmpInst::ICMP_ULE:
    r = ua <= ub;
    break;
  case llvm::CmpInst::ICMP_SGT:
    r = sa > sb;
    break;
  case llvm::CmpInst::ICMP_SGE:
    r = sa >= sb;
    break;
  case llvm::CmpInst::ICMP_SLT:
    r = sa < sb;
    break;
  case llvm::CmpInst::ICMP_SLE:
    r = sa <= sb;
    break;
  default:
    cout << "Unhandled ICmp predicate." << endl;
    break;
  }

  memcpy(result.data, &r, result.size);
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
  Memory *memory = NULL;
  switch (addressSpace)
  {
  case 0: // Private memory
    memory = m_stack;
    break;
  case 1: // Global memory
    memory = &m_globalMemory;
    break;
  case 3: // Local memory
    memory = m_workGroup.getLocalMemory();
    break;
  default:
    cout << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  // Load data
  if (memory)
  {
    if (!memory->load(address, result.size, result.data))
    {
      outputMemoryError(instruction, "Invalid read",
                        addressSpace, address, result.size);
    }
  }
}

void WorkItem::lshr(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Signed?
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a >> b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::mul(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Signed?
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a * b;
  memcpy(result.data, &r, result.size);
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

void WorkItem::sdiv(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Need to reinterpret for signed
  int64_t a = getIntValue(instruction.getOperand(0));
  int64_t b = getIntValue(instruction.getOperand(1));
  int64_t r = a / b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::select(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::SelectInst *selectInst = (llvm::SelectInst*)&instruction;
  const bool cond = getIntValue(selectInst->getCondition());
  const llvm::Value *op = cond ?
    selectInst->getTrueValue() :
    selectInst->getFalseValue();

  uint64_t i;
  double f;

  llvm::Type::TypeID type = op->getType()->getTypeID();
  switch (type)
  {
  case llvm::Type::IntegerTyID:
    i = getIntValue(op);
    memcpy(result.data, &i, result.size);
    break;
  case llvm::Type::FloatTyID:
  case llvm::Type::DoubleTyID:
    f = getFloatValue(op);
    setFloatResult(result, f);
    break;
  default:
    cout << "Unhandled type in select instruction: " << type << endl;
    break;
  }
}

void WorkItem::sext(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Need to reinterpret
  int64_t val = getIntValue(instruction.getOperand(0));
  switch (result.size)
  {
  case 1:
    *((char*)result.data) = val;
    break;
  case 2:
    *((short*)result.data) = val;
    break;
  case 4:
    *((int*)result.data) = val;
    break;
  case 8:
    *((long*)result.data) = val;
    break;
  }
}

void WorkItem::shl(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Signed?
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a << b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::srem(const llvm::Instruction& instruction, TypedValue& result)
{
  // TODO: Need to reinterpret for signed
  int64_t a = getIntValue(instruction.getOperand(0));
  int64_t b = getIntValue(instruction.getOperand(1));
  int64_t r = a % b;
  memcpy(result.data, &r, result.size);
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

  Memory *memory = NULL;

  // TODO: Find or create address space enum
  switch (addressSpace)
  {
  case 0: // Private memory
    memory = m_stack;
    break;
  case 1: // Global memory
    memory = &m_globalMemory;
    break;
  case 3: // Local memory
    memory = m_workGroup.getLocalMemory();
    break;
  default:
    cout << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  // Store data
  if (memory)
  {
    if (!memory->store(address, size, data))
    {
      outputMemoryError(instruction, "Invalid write",
                        addressSpace, address, size);
    }
  }

  delete[] data;
}

void WorkItem::trunc(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t val = getIntValue(instruction.getOperand(0));
  switch (result.size)
  {
  case 1:
    *((unsigned char*)result.data) = val;
    break;
  case 2:
    *((unsigned short*)result.data) = val;
    break;
  case 4:
    *((unsigned int*)result.data) = val;
    break;
  case 8:
    *((unsigned long*)result.data) = val;
    break;
  }
}

void WorkItem::udiv(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a / b;
  memcpy(result.data, &r, result.size);
}

void WorkItem::urem(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t a = getIntValue(instruction.getOperand(0));
  uint64_t b = getIntValue(instruction.getOperand(1));
  uint64_t r = a / b;
  memcpy(result.data, &r, result.size);
}
