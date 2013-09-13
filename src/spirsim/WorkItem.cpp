#include "common.h"
#include <cxxabi.h>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Constants.h"
#include "llvm/DebugInfo.h"
#include "llvm/Metadata.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Type.h"

#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
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
  const size_t *globalOffset = workGroup.getGlobalOffset();
  m_globalID[0] = lid_x + groupID[0]*groupSize[0] + globalOffset[0];
  m_globalID[1] = lid_y + groupID[1]*groupSize[1] + globalOffset[1];
  m_globalID[2] = lid_z + groupID[2]*groupSize[2] + globalOffset[2];

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
  delete m_stack;
}

void WorkItem::clearBarrier()
{
  if (m_state == BARRIER || m_state == WAIT_EVENT)
  {
    m_state = READY;
  }
}

void WorkItem::dispatch(const llvm::Instruction& instruction,
                        TypedValue& result)
{
  switch (instruction.getOpcode())
  {
  case llvm::Instruction::Add:
    add(instruction, result);
    break;
  case llvm::Instruction::Alloca:
    alloc(instruction, result);
    break;
  case llvm::Instruction::And:
    bwand(instruction, result);
    break;
  case llvm::Instruction::AShr:
    ashr(instruction, result);
    break;
  case llvm::Instruction::BitCast:
    bitcast(instruction, result);
    break;
  case llvm::Instruction::Br:
    br(instruction);
    break;
  case llvm::Instruction::Call:
    call(instruction, result);
    break;
  case llvm::Instruction::ExtractElement:
    extract(instruction, result);
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
  case llvm::Instruction::FPExt:
    fpext(instruction, result);
    break;
  case llvm::Instruction::FPToSI:
    fptosi(instruction, result);
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
  case llvm::Instruction::InsertElement:
    insert(instruction, result);
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
  case llvm::Instruction::PtrToInt:
    ptrtoint(instruction, result);
    break;
  case llvm::Instruction::Ret:
    ret(instruction, result);
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
  case llvm::Instruction::ShuffleVector:
    shuffle(instruction, result);
    break;
  case llvm::Instruction::SIToFP:
    sitofp(instruction, result);
    break;
  case llvm::Instruction::SRem:
    srem(instruction, result);
    break;
  case llvm::Instruction::Store:
    store(instruction);
    break;
  case llvm::Instruction::Sub:
    sub(instruction, result);
    break;
  case llvm::Instruction::Switch:
    swtch(instruction);
    break;
  case llvm::Instruction::Trunc:
    itrunc(instruction, result);
    break;
  case llvm::Instruction::UDiv:
    udiv(instruction, result);
    break;
  case llvm::Instruction::UIToFP:
    uitofp(instruction, result);
    break;
  case llvm::Instruction::URem:
    urem(instruction, result);
    break;
  case llvm::Instruction::Xor:
    bwxor(instruction, result);
    break;
  case llvm::Instruction::ZExt:
    zext(instruction, result);
    break;
  default:
    cerr << "Unhandled instruction: " << instruction.getOpcodeName() << endl;
    break;
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
    for (int i = 0; i < result.size*result.num; i++)
    {
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)result.data[i];
    }

    // Interpret value
    const llvm::Type::TypeID type = value->getType()->getTypeID();
    switch (type)
    {
    case llvm::Type::IntegerTyID:
      cout << " (" << dec << getUnsignedInt(value) << ")";
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
  if (m_stack->getTotalAllocated() > 0)
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
  pair<size_t,size_t> resultSize = getValueSize(&instruction);

  // TODO: Bitcasting should happen somewhere...
  // Prepare result
  TypedValue result = {
    resultSize.first,
    resultSize.second,
    new unsigned char[resultSize.first*resultSize.second]
  };

  // Dump instruction sequence
  if (m_debugOutput)
  {
    dumpInstruction(cout, instruction, true);
  }

  if (instruction.getOpcode() != llvm::Instruction::PHI &&
      m_phiTemps.size() > 0)
  {
    TypedValueMap::iterator itr;
    for (itr = m_phiTemps.begin(); itr != m_phiTemps.end(); itr++)
    {
      if (m_privateMemory.find(itr->first) != m_privateMemory.end())
      {
        delete[] m_privateMemory[itr->first].data;
      }

      m_privateMemory[itr->first] = itr->second;
    }
    m_phiTemps.clear();
  }

  // Execute instruction
  dispatch(instruction, result);

  // Store result
  if (resultSize.first > 0)
  {
    if (instruction.getOpcode() != llvm::Instruction::PHI)
    {
      if (m_privateMemory.find(&instruction) != m_privateMemory.end())
      {
        delete[] m_privateMemory[&instruction].data;
      }
      m_privateMemory[&instruction] = result;
    }
    else
    {
      m_phiTemps[&instruction] = result;
    }
  }
  else
  {
    delete[] result.data;
  }
}

const size_t* WorkItem::getGlobalID() const
{
  return m_globalID;
}

double WorkItem::getFloatValue(const llvm::Value *operand,
                               unsigned int index) const
{
  double val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = m_privateMemory.at(operand);
    if (op.size == sizeof(float))
    {
      val = ((float*)op.data)[index];
    }
    else if (op.size == sizeof(double))
    {
      val = ((double*)op.data)[index];
    }
    else
    {
      cerr << "Unhandled float size: " << op.size << endl;
      return 0;
    }
  }
  else if (operand->getValueID() == llvm::Value::ConstantVectorVal)
  {
    val = getFloatValue(
      ((const llvm::ConstantVector*)operand)->getAggregateElement(index));
  }
  else if (operand->getValueID() == llvm::Value::ConstantDataVectorVal)
  {
    val = getFloatValue(
      ((const llvm::ConstantDataVector*)operand)->getElementAsConstant(index));
  }
  else if (id == llvm::Value::UndefValueVal)
  {
    val = -1;
  }
  else if (id == llvm::Value::ConstantAggregateZeroVal)
  {
    val = 0;
  }
  else if (isConstantOperand(operand))
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
      cerr << "Unhandled float semantics " << operand->getValueID() << endl;
      return 0;
    }
  }
  else
  {
    cerr << "Unhandled float operand type " << id << endl;
  }
  return val;
}

int64_t WorkItem::getSignedInt(const llvm::Value *operand,
                               unsigned int index) const
{
  int64_t val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = m_privateMemory.at(operand);
    switch (op.size)
    {
    case 1:
      val = ((char*)op.data)[index];
      break;
    case 2:
      val = ((short*)op.data)[index];
      break;
    case 4:
      val = ((int*)op.data)[index];
      break;
    case 8:
      val = ((long*)op.data)[index];
      break;
    default:
      cerr << "Unhandled signed int size " << op.size << endl;
      break;
    }
  }
  else if (id == llvm::Value::ConstantVectorVal ||
           id == llvm::Value::ConstantDataVectorVal)
  {
    val = getSignedInt(
      ((const llvm::ConstantVector*)operand)->getAggregateElement(index));
  }
  else if (id == llvm::Value::ConstantIntVal)
  {
    val = ((const llvm::ConstantInt*)operand)->getSExtValue();
  }
  else if (id == llvm::Value::UndefValueVal)
  {
    val = -1;
  }
  else if (id == llvm::Value::ConstantAggregateZeroVal)
  {
    val = 0;
  }
  else
  {
    cerr << "Unhandled signed operand type " << id << endl;
  }
  return val;
}

WorkItem::State WorkItem::getState() const
{
  return m_state;
}

uint64_t WorkItem::getUnsignedInt(const llvm::Value *operand,
                                  unsigned int index) const
{
  uint64_t val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = m_privateMemory.at(operand);
    memcpy(&val, op.data + index*op.size, op.size);
  }
  else if (id == llvm::Value::ConstantVectorVal ||
           id == llvm::Value::ConstantDataVectorVal)
  {
    val = getUnsignedInt(
      ((const llvm::ConstantVector*)operand)->getAggregateElement(index));
  }
  else if (id == llvm::Value::UndefValueVal)
  {
    val = -1;
  }
  else if (id == llvm::Value::ConstantAggregateZeroVal)
  {
    val = 0;
  }
  else if (id == llvm::Value::ConstantIntVal)
  {
    val = ((const llvm::ConstantInt*)operand)->getZExtValue();
  }
  else
  {
    cerr << "Unhandled unsigned operand type " << id << endl;
  }

  return val;
}

void WorkItem::outputMemoryError(const llvm::Instruction& instruction,
                                 const std::string& msg,
                                 unsigned addressSpace,
                                 size_t address, size_t size) const
{
  std::string memType;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memType = "private";
    break;
  case AddrSpaceGlobal:
    memType = "global";
    break;
  case AddrSpaceConstant:
    memType = "constant";
    break;
  case AddrSpaceLocal:
    memType = "local";
    break;
  default:
    assert(false && "Memory error in unsupported address space.");
    break;
  }

  // Basic error info
  cerr << endl << msg
       << " of size " << size
       << " at " << memType
       << " memory address " << hex << address << endl;

  // Work item
  cerr << "\tWork-item:  Global(" << dec
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2] << ")"
       << " Local(" << dec
       << m_localID[0] << ","
       << m_localID[1] << ","
       << m_localID[2] << ")"
       << endl;

  // Work group
  const size_t *group = m_workGroup.getGroupID();
  cerr << "\tWork-group: (" << dec
       << group[0] << ","
       << group[1] << ","
       << group[2] << ")"
       << endl;

  // Kernel
  cerr << "\tKernel:     " << m_kernel.getName() << endl;

  // Instruction
  cerr << "\t";
  dumpInstruction(cerr, instruction);

  // Output debug information
  cerr << "\t";
  llvm::MDNode *md = instruction.getMetadata("dbg");
  if (!md)
  {
    cerr << "Debugging information not available." << endl;
  }
  else
  {
    llvm::DILocation loc(md);
    cerr << "At line " << dec << loc.getLineNumber()
         << " of " << loc.getFilename().str() << endl;
  }
  cerr << endl;
}

TypedValue WorkItem::resolveConstExpr(const llvm::ConstantExpr *expr)
{
  llvm::Instruction *instruction = getConstExprAsInstruction(expr);
  pair<size_t,size_t> resultSize = getValueSize(instruction);
  TypedValue result =
    {
      resultSize.first,
      resultSize.second,
      new unsigned char[resultSize.first*resultSize.second]
    };

  dispatch(*instruction, result);

  delete instruction;

  return result;
}

void WorkItem::setFloatResult(TypedValue& result, double val,
                              unsigned int index) const
{
  if (result.size == sizeof(float))
  {
    ((float*)result.data)[index] = val;
  }
  else if (result.size == sizeof(double))
  {
    ((double*)result.data)[index] = val;
  }
  else
  {
    cerr << "Unhandled float size: " << dec << result.size << endl;
  }
}

void WorkItem::setIntResult(TypedValue& result, int64_t val,
                            unsigned int index) const
{
  memcpy(result.data + index*result.size, &val, result.size);
}

void WorkItem::setIntResult(TypedValue& result, uint64_t val,
                            unsigned int index) const
{
  memcpy(result.data + index*result.size, &val, result.size);
}

WorkItem::State WorkItem::step(bool debugOutput)
{
  assert(m_state == READY);

  if (debugOutput)
  {
    dumpInstruction(cout, *m_currInst, true);
  }

  // Execute the next instruction
  execute(*m_currInst);

  // Check if we've reached the end of the block
  if (++m_currInst == m_currBlock->end() || m_nextBlock)
  {
    if (m_nextBlock)
    {
      // Move to next basic block
      m_prevBlock = m_currBlock;
      m_currBlock = m_nextBlock;
      m_nextBlock = NULL;
      m_currInst = m_currBlock->begin();
    }
  }

  return m_state;
}

void WorkItem::trap()
{
  cerr << "Work-item (" << dec
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2] << ") terminated unexpectedly." << endl;
  m_state = FINISHED;
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
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a + b, i);
  }
}

void WorkItem::alloc(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::AllocaInst *allocInst = ((const llvm::AllocaInst*)&instruction);
  const llvm::Type *type = allocInst->getAllocatedType();

  // Perform allocation
  size_t size = getTypeSize(type);
  size_t address = m_stack->allocateBuffer(size);

  // Create pointer to alloc'd memory
  *((size_t*)result.data) = address;
}

void WorkItem::ashr(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a >> b, i);
  }
}

void WorkItem::bitcast(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::CastInst *cast = (const llvm::CastInst*)&instruction;
  llvm::Value *operand = cast->getOperand(0);
  if (m_privateMemory.find(operand) != m_privateMemory.end())
  {
    memcpy(result.data, m_privateMemory[operand].data, result.size*result.num);
  }
  else
  {
    cerr << "Unsupported bitcast operand." << endl;
  }
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
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a & b, i);
  }
}

void WorkItem::bwor(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a | b, i);
  }
}

void WorkItem::bwxor(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a ^ b, i);
  }
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
    cerr << "Unhandled indirect function call: "
         << function->getName().str() << endl;
    return;
  }

  const llvm::Function *function = callInst->getCalledFunction();
  const string fullname = function->getName().str();
  string name, overload;

  // Demangle if necessary
  if (fullname.compare(0,2, "_Z") == 0)
  {
    int len = atoi(fullname.c_str()+2);
    int start = fullname.find_first_not_of("0123456789", 2);
    name = fullname.substr(start, len);
    overload = fullname.substr(start + len);
  }
  else
  {
    name = fullname;
    overload = "";
  }

  // Check if function has definition
  if (!function->isDeclaration())
  {
    ReturnAddress ret(m_currBlock, m_currInst);
    m_callStack.push(ret);

    m_nextBlock = function->begin();

    // Set function arguments
    int i = 0;
    llvm::Function::const_arg_iterator argItr;
    for (argItr = function->arg_begin(); argItr != function->arg_end(); argItr++)
    {
      const llvm::Value *arg = callInst->getArgOperand(i++);

      pair<size_t,size_t> argSize = getValueSize(arg);
      size_t size = argSize.first * argSize.second;
      TypedValue value = {
        argSize.first,
        argSize.second,
        new unsigned char[size]
      };

      unsigned id = arg->getValueID();
      if (id == llvm::Value::GlobalVariableVal ||
          id == llvm::Value::ArgumentVal ||
          id >= llvm::Value::InstructionVal)
      {
        memcpy(value.data, m_privateMemory[arg].data, size);
      }
      else if (id == llvm::Value::ConstantFPVal)
      {
        setFloatResult(value, getFloatValue(arg));
      }
      else if (id == llvm::Value::ConstantIntVal)
      {
        setIntResult(value, getUnsignedInt(arg));
      }
      else
      {
        cerr << "Unhandled function argument type " << id << endl;
      }

      if (m_privateMemory.find(argItr) != m_privateMemory.end())
      {
        delete[] m_privateMemory[argItr].data;
      }
      m_privateMemory[argItr] = value;
    }

    return;
  }

#define BUILTIN(str, fn) else if (name == str) \
  fn(callInst, name, overload, result)
#define BUILTIN_PREFIX(str, fn) else if (name.compare(0, strlen(str), str) == 0) \
  fn(callInst, name, overload, result)
#define BUILTIN_F1ARG(str, fn) else if (name == str) \
  builtin_f1arg(callInst, result, fn)
#define BUILTIN_F2ARG(str, fn) else if (name == str) \
  builtin_f2arg(callInst, result, fn)
#define BUILTIN_F3ARG(str, fn) else if (name == str) \
  builtin_f3arg(callInst, result, fn)
#define BUILTIN_U1ARG(str, fn) else if (name == str) \
  builtin_u1arg(callInst, result, fn)
#define BUILTIN_U2ARG(str, fn) else if (name == str) \
  builtin_u2arg(callInst, result, fn)
#define BUILTIN_U3ARG(str, fn) else if (name == str) \
  builtin_u3arg(callInst, result, fn)
#define BUILTIN_S1ARG(str, fn) else if (name == str) \
  builtin_s1arg(callInst, result, fn)
#define BUILTIN_S2ARG(str, fn) else if (name == str) \
  builtin_s2arg(callInst, result, fn)
#define BUILTIN_S3ARG(str, fn) else if (name == str) \
  builtin_s3arg(callInst, result, fn)
#define BUILTIN_REL1ARG(str, fn) else if (name == str) \
  builtin_rel1arg(callInst, result, fn)
#define BUILTIN_REL2ARG(str, fn) else if (name == str) \
  builtin_rel2arg(callInst, result, fn)


  if (false);
  // Async Copy and Prefetch Functions
  BUILTIN("async_work_group_copy", async_work_group_copy);
  BUILTIN("async_work_group_strided_copy", async_work_group_copy);
  BUILTIN("wait_group_events", wait_group_events);
  BUILTIN("prefetch", prefetch);

  // Common Functions
  BUILTIN("clamp", clamp);
  BUILTIN_F1ARG("degrees", degrees);
  BUILTIN("max", max);
  BUILTIN("min", min);
  BUILTIN_F3ARG("mix", mix);
  BUILTIN_F1ARG("radians", radians);
  BUILTIN_F1ARG("sign", sign);
  BUILTIN_F3ARG("smoothstep", smoothstep);
  BUILTIN_F2ARG("step", step_builtin);

  // Geometric Functions
  BUILTIN("cross", cross);
  BUILTIN("dot", dot);
  BUILTIN("distance", distance);
  BUILTIN("length", length);
  BUILTIN("normalize", normalize);
  BUILTIN("fast_distance", distance);
  BUILTIN("fast_length", length);
  BUILTIN("fast_normalize", normalize);

  // Integer Functions
  BUILTIN("abs", abs_builtin);
  BUILTIN("abs_diff", abs_diff);
  BUILTIN("add_sat", add_sat);
  BUILTIN("clz", clz);
  BUILTIN("hadd", hadd);
  BUILTIN_U3ARG("mad24", mad);
  BUILTIN("mad_sat", mad_sat);
  BUILTIN_U2ARG("mul24", mul_builtin);
  BUILTIN("mul_hi", mul_hi);
  BUILTIN_U1ARG("popcount", popcount);
  BUILTIN("rhadd", rhadd);
  BUILTIN("rotate", rotate);
  BUILTIN("sub_sat", sub_sat);

  // Math Functions
  BUILTIN_F1ARG("acos", acos);
  BUILTIN_F1ARG("acosh", acosh);
  BUILTIN_F1ARG("acospi", acospi);
  BUILTIN_F1ARG("asin", asin);
  BUILTIN_F1ARG("asinh", asinh);
  BUILTIN_F1ARG("asinpi", asinpi);
  BUILTIN_F1ARG("atan", atan);
  BUILTIN_F2ARG("atan2", atan2);
  BUILTIN_F1ARG("atanh", atanh);
  BUILTIN_F1ARG("atanpi", atanpi);
  BUILTIN_F2ARG("atan2pi", atan2pi);
  BUILTIN_F1ARG("cbrt", cbrt);
  BUILTIN_F1ARG("ceil", ceil);
  BUILTIN_F2ARG("copysign", copysign);
  BUILTIN_F1ARG("cos", cos);
  BUILTIN_F1ARG("cosh", cosh);
  BUILTIN_F1ARG("cospi", cospi);
  BUILTIN_F1ARG("erfc", erfc);
  BUILTIN_F1ARG("erf", erf);
  BUILTIN_F1ARG("exp", exp);
  BUILTIN_F1ARG("exp2", exp2);
  BUILTIN_F1ARG("exp10", exp10);
  BUILTIN_F1ARG("expm1", expm1);
  BUILTIN_F1ARG("fabs", fabs);
  BUILTIN_F2ARG("fdim", fdim);
  BUILTIN_F1ARG("floor", floor);
  BUILTIN_F3ARG("fma", fma);
  BUILTIN_F2ARG("fmax", fmax);
  BUILTIN_F2ARG("fmin", fmin);
  BUILTIN_F2ARG("fmod", fmod);
  BUILTIN("fract", fract);
  BUILTIN("frexp", frexp_builtin);
  BUILTIN_F2ARG("hypot", hypot);
  BUILTIN("ilogb", ilogb_builtin);
  BUILTIN("ldexp", ldexp_builtin);
  BUILTIN_F1ARG("lgamma", lgamma);
  BUILTIN("lgamma_r", lgamma_r);
  BUILTIN_F1ARG("log", log);
  BUILTIN_F1ARG("log2", log2);
  BUILTIN_F1ARG("log10", log10);
  BUILTIN_F1ARG("log1p", log1p);
  BUILTIN_F1ARG("logb", logb);
  BUILTIN_F3ARG("mad", fma);
  BUILTIN_F2ARG("maxmag", maxmag);
  BUILTIN_F2ARG("minmag", minmag);
  BUILTIN("modf", modf_builtin);
  BUILTIN("nan", nan_builtin);
  BUILTIN_F2ARG("nextafter", nextafter);
  BUILTIN_F2ARG("pow", pow);
  BUILTIN("pown", pown);
  BUILTIN_F2ARG("powr", pow);
  BUILTIN_F2ARG("remainder", remainder);
  BUILTIN("remquo", remquo_builtin);
  BUILTIN_F1ARG("rint", rint);
  BUILTIN("rootn", rootn);
  BUILTIN_F1ARG("round", round);
  BUILTIN_F1ARG("rsqrt", rsqrt);
  BUILTIN_F1ARG("sin", sin);
  BUILTIN_F1ARG("sinh", sinh);
  BUILTIN_F1ARG("sinpi", sinpi);
  BUILTIN("sincos", sincos);
  BUILTIN_F1ARG("sqrt", sqrt);
  BUILTIN_F1ARG("tan", tan);
  BUILTIN_F1ARG("tanh", tanh);
  BUILTIN_F1ARG("tanpi", tanpi);
  BUILTIN_F1ARG("tgamma", tgamma);
  BUILTIN_F1ARG("trunc", trunc);

  // Native Math Functions
  BUILTIN_F1ARG("half_cos", cos);
  BUILTIN_F1ARG("native_cos", cos);
  BUILTIN_F2ARG("half_divide", fdivide);
  BUILTIN_F2ARG("native_divide", fdivide);
  BUILTIN_F1ARG("half_exp", exp);
  BUILTIN_F1ARG("native_exp", exp);
  BUILTIN_F1ARG("half_exp2", exp2);
  BUILTIN_F1ARG("native_exp2", exp2);
  BUILTIN_F1ARG("half_exp10", exp10);
  BUILTIN_F1ARG("native_exp10", exp10);
  BUILTIN_F1ARG("half_log", log);
  BUILTIN_F1ARG("native_log", log);
  BUILTIN_F1ARG("half_log2", log2);
  BUILTIN_F1ARG("native_log2", log2);
  BUILTIN_F1ARG("half_log10", log10);
  BUILTIN_F1ARG("native_log10", log10);
  BUILTIN_F2ARG("half_powr", pow);
  BUILTIN_F2ARG("native_powr", pow);
  BUILTIN_F1ARG("half_recip", frecip);
  BUILTIN_F1ARG("native_recip", frecip);
  BUILTIN_F1ARG("half_rsqrt", rsqrt);
  BUILTIN_F1ARG("native_rsqrt", rsqrt);
  BUILTIN_F1ARG("half_sin", sin);
  BUILTIN_F1ARG("native_sin", sin);
  BUILTIN_F1ARG("half_sqrt", sqrt);
  BUILTIN_F1ARG("native_sqrt", sqrt);
  BUILTIN_F1ARG("half_tan", tan);
  BUILTIN_F1ARG("native_tan", tan);

  // Relational Functional
  BUILTIN("all", all);
  BUILTIN("any", any);
  BUILTIN("bitselect", bitselect);
  BUILTIN_REL2ARG("isequal", isequal_builtin);
  BUILTIN_REL2ARG("isnotequal", isnotequal_builtin);
  BUILTIN_REL2ARG("isgreater", isgreater_builtin);
  BUILTIN_REL2ARG("isgreaterequal", isgreaterequal_builtin);
  BUILTIN_REL2ARG("isless", isless_builtin);
  BUILTIN_REL2ARG("islessequal", islessequal_builtin);
  BUILTIN_REL2ARG("islessgreater", islessgreater_builtin);
  BUILTIN_REL1ARG("isfinite", isfinite_builtin);
  BUILTIN_REL1ARG("isinf", isinf_builtin);
  BUILTIN_REL1ARG("isnan", isnan_builtin);
  BUILTIN_REL1ARG("isnormal", isnormal_builtin);
  BUILTIN_REL2ARG("isordered", isordered_builtin);
  BUILTIN_REL2ARG("isunordered", isunordered_builtin);
  BUILTIN("select", select_builtin);
  BUILTIN_REL1ARG("signbit", signbit_builtin);

  // Synchronization Functions
  BUILTIN("barrier", barrier);
  BUILTIN("mem_fence", mem_fence);
  BUILTIN("read_mem_fence", mem_fence);
  BUILTIN("write_mem_fence", mem_fence);

  // Vector Data Load and Store Functions
  BUILTIN_PREFIX("vload", vload);
  BUILTIN_PREFIX("vstore", vstore);

  // Work-Item Functions
  BUILTIN("get_global_id", get_global_id);
  BUILTIN("get_global_size", get_global_size);
  BUILTIN("get_global_offset", get_global_offset);
  BUILTIN("get_group_id", get_group_id);
  BUILTIN("get_local_id", get_local_id);
  BUILTIN("get_local_size", get_local_size);
  BUILTIN("get_num_groups", get_num_groups);
  BUILTIN("get_work_dim", get_work_dim);

  // Other Functions
  BUILTIN("printf", printf_builtin);

  // LLVM Intrinsics
  BUILTIN("llvm.dbg.declare", llvm_dbg_declare);
  BUILTIN("llvm.dbg.value", llvm_dbg_value);
  BUILTIN("llvm.lifetime.start", llvm_lifetime_start);
  BUILTIN("llvm.lifetime.end", llvm_lifetime_end);
  BUILTIN_PREFIX("llvm.memcpy", llvm_memcpy);
  BUILTIN_PREFIX("llvm.memset", llvm_memset);
  BUILTIN("llvm.trap", llvm_trap);

  // Function didn't match builtin
  else
  {
    cerr << "Unhandled direct function call: " << name << endl;
  }
  return;
}

void WorkItem::extract(const llvm::Instruction& instruction,
                       TypedValue& result)
{
  llvm::ExtractElementInst *extract = (llvm::ExtractElementInst*)&instruction;

  llvm::Value *vector = extract->getVectorOperand();
  unsigned int index = getUnsignedInt(extract->getIndexOperand());
  llvm::Type *type = vector->getType()->getVectorElementType();
  switch (type->getTypeID())
  {
  case llvm::Type::FloatTyID:
  case llvm::Type::DoubleTyID:
    setFloatResult(result, getFloatValue(vector, index));
    break;
  case llvm::Type::IntegerTyID:
    setIntResult(result, getUnsignedInt(vector, index));
    break;
  default:
    cerr << "Unhandled vector type " << type->getTypeID() << endl;
    return;
  }
}

void WorkItem::fadd(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction.getOperand(0), i);
    double b = getFloatValue(instruction.getOperand(1), i);
    setFloatResult(result, a + b, i);
  }
}

void WorkItem::fcmp(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t t = result.num > 1 ? -1 : 1;
  llvm::CmpInst::Predicate pred = ((llvm::CmpInst&)instruction).getPredicate();
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction.getOperand(0), i);
    double b = getFloatValue(instruction.getOperand(1), i);

    // TODO: Consider nans in comparisons?
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
    case llvm::CmpInst::FCMP_UEQ:
      r = a == b;
      break;
    case llvm::CmpInst::FCMP_UNE:
      r = a != b;
      break;
    case llvm::CmpInst::FCMP_UGT:
      r = a > b;
      break;
    case llvm::CmpInst::FCMP_UGE:
      r = a >= b;
      break;
    case llvm::CmpInst::FCMP_ULT:
      r = a < b;
      break;
    case llvm::CmpInst::FCMP_ULE:
      r = a <= b;
      break;
    default:
      cerr << "Unhandled FCmp predicate " << pred << endl;
      break;
    }

    setIntResult(result, r ? t : 0, i);
  }
}

void WorkItem::fdiv(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction.getOperand(0), i);
    double b = getFloatValue(instruction.getOperand(1), i);
    setFloatResult(result, a / b, i);
  }
}

void WorkItem::fmul(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction.getOperand(0), i);
    double b = getFloatValue(instruction.getOperand(1), i);
    setFloatResult(result, a * b, i);
  }
}

void WorkItem::fpext(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double r = getFloatValue(instruction.getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

void WorkItem::fptosi(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t r = (int64_t)getFloatValue(instruction.getOperand(0), i);
    setIntResult(result, r, i);
  }
}

void WorkItem::frem(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction.getOperand(0), i);
    double b = getFloatValue(instruction.getOperand(1), i);
    setFloatResult(result, fmod(a, b), i);
  }
}

void WorkItem::fsub(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction.getOperand(0), i);
    double b = getFloatValue(instruction.getOperand(1), i);
    setFloatResult(result, a - b, i);
  }
}

void WorkItem::gep(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::GetElementPtrInst *gepInst =
    (const llvm::GetElementPtrInst*)&instruction;

  // Get base address
  size_t address;
  const llvm::Value *base = gepInst->getPointerOperand();
  if (base->getValueID() == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)base);
    address = *(size_t*)result.data;
    delete[] result.data;
  }
  else
  {
    address = *((size_t*)m_privateMemory[base].data);
  }
  llvm::Type *ptrType = gepInst->getPointerOperandType();
  assert(ptrType->isPointerTy());

  // Iterate over indices
  llvm::User::const_op_iterator opItr;
  for (opItr = gepInst->idx_begin(); opItr != gepInst->idx_end(); opItr++)
  {
    int64_t offset = getSignedInt(opItr->get());

    if (ptrType->isPointerTy())
    {
      // Get pointer element size
      llvm::Type *elemType = ptrType->getPointerElementType();
      address += offset*getTypeSize(elemType);
      ptrType = elemType;
    }
    else if (ptrType->isArrayTy())
    {
      // Get array element size
      llvm::Type *elemType = ptrType->getArrayElementType();
      address += offset*getTypeSize(elemType);
      ptrType = elemType;
    }
    else if (ptrType->isVectorTy())
    {
      // Get vector element size
      llvm::Type *elemType = ptrType->getVectorElementType();
      address += offset*getTypeSize(elemType);
      ptrType = elemType;
    }
    else if (ptrType->isStructTy())
    {
      // Get structure member offset
      for (int i = 0; i < offset; i++)
      {
        address += getTypeSize(ptrType->getStructElementType(i));
      }
      ptrType = ptrType->getStructElementType(offset);
    }
    else
    {
      cerr << "Unhandled GEP base type." << endl;
    }
  }

  *((size_t*)result.data) = address;
}

void WorkItem::icmp(const llvm::Instruction& instruction, TypedValue& result)
{
  uint64_t t = result.num > 1 ? -1 : 1;
  llvm::CmpInst::Predicate pred = ((llvm::CmpInst&)instruction).getPredicate();
  for (int i = 0; i < result.num; i++)
  {
    // Load operands
    llvm::Value *opA = instruction.getOperand(0);
    llvm::Value *opB = instruction.getOperand(1);
    uint64_t ua = getUnsignedInt(opA, i);
    uint64_t ub = getUnsignedInt(opB, i);
    int64_t sa = getSignedInt(opA, i);
    int64_t sb = getSignedInt(opB, i);

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
      cerr << "Unhandled ICmp predicate." << endl;
      break;
    }

    setIntResult(result, r ? t : 0, i);
  }
}

void WorkItem::insert(const llvm::Instruction& instruction,
                      TypedValue& result)
{
  llvm::InsertElementInst *insert = (llvm::InsertElementInst*)&instruction;
  llvm::Value *vector = insert->getOperand(0);
  unsigned int index = getUnsignedInt(insert->getOperand(2));
  llvm::Type *type = vector->getType()->getVectorElementType();
  for (int i = 0; i < result.num; i++)
  {
    switch (type->getTypeID())
    {
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
      if (i == index)
      {
        setFloatResult(result, getFloatValue(insert->getOperand(1)), index);
      }
      else
      {
        setFloatResult(result, getFloatValue(vector, i), i);
      }
      break;
    case llvm::Type::IntegerTyID:
      if (i == index)
      {
        setIntResult(result, getUnsignedInt(insert->getOperand(1)), index);
      }
      else
      {
        setIntResult(result, getUnsignedInt(vector, i), i);
      }
      break;
    default:
      cerr << "Unhandled vector type " << type->getTypeID() << endl;
      return;
    }
  }
}

void WorkItem::itrunc(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t val = getUnsignedInt(instruction.getOperand(0), i);
    setIntResult(result, val, i);
  }
}

void WorkItem::load(const llvm::Instruction& instruction,
                    TypedValue& result)
{
  const llvm::LoadInst *loadInst = (const llvm::LoadInst*)&instruction;
  const llvm::Value *ptrOp = loadInst->getPointerOperand();
  unsigned addressSpace = loadInst->getPointerAddressSpace();

  // Get address
  size_t address;
  if (ptrOp->getValueID() == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((llvm::ConstantExpr*)ptrOp);
    address = *(size_t*)result.data;
    delete[] result.data;
  }
  else
  {
    address = *((size_t*)m_privateMemory[ptrOp].data);
  }

  // Check address space
  Memory *memory = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memory = m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    memory = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    memory = m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  // Load data
  if (memory)
  {
    if (!memory->load(result.data, address, result.size*result.num))
    {
      outputMemoryError(instruction, "Invalid read",
                        addressSpace, address, result.size*result.num);
    }
  }
}

void WorkItem::lshr(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a >> b, i);
  }
}

void WorkItem::mul(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a * b, i);
  }
}

void WorkItem::phi(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::PHINode *phiNode = (llvm::PHINode*)&instruction;
  const llvm::Value *value =
    phiNode->getIncomingValueForBlock((const llvm::BasicBlock*)m_prevBlock);

  llvm::Type::TypeID type = value->getType()->getTypeID();
  if (type == llvm::Type::VectorTyID)
  {
    type = value->getType()->getVectorElementType()->getTypeID();
  }

  for (int i = 0; i < result.num; i++)
  {
    switch (type)
    {
    case llvm::Type::IntegerTyID:
      setIntResult(result, getUnsignedInt(value, i), i);
      break;
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
      setFloatResult(result, getFloatValue(value, i), i);
      break;
    case llvm::Type::PointerTyID:
      memcpy(result.data, m_privateMemory[value].data, result.size);
      break;
    default:
      cerr << "Unhandled type in phi instruction: " << type << endl;
      break;
    }
  }
}

void WorkItem::ptrtoint(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::CastInst *cast = (const llvm::CastInst*)&instruction;
  for (int i = 0; i < result.num; i++)
  {
    uint64_t r = getUnsignedInt(instruction.getOperand(0), i);
    setIntResult(result, r, i);
  }
}

void WorkItem::ret(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::ReturnInst *retInst = (const llvm::ReturnInst*)&instruction;

  if (!m_callStack.empty())
  {
    ReturnAddress ret = m_callStack.top();
    m_callStack.pop();
    m_currBlock = ret.first;
    m_currInst = ret.second;

    // Set return value
    const llvm::Value *returnVal = retInst->getReturnValue();
    if (returnVal)
    {
      if (m_privateMemory.find(m_currInst) != m_privateMemory.end())
      {
        delete[] m_privateMemory[m_currInst].data;
      }
      m_privateMemory[m_currInst] = clone(m_privateMemory[returnVal]);
    }
  }
  else
  {
    m_nextBlock = NULL;
    m_state = FINISHED;
  }
}

void WorkItem::sdiv(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction.getOperand(0), i);
    int64_t b = getSignedInt(instruction.getOperand(1), i);
    setIntResult(result, a / b, i);
  }
}

void WorkItem::select(const llvm::Instruction& instruction, TypedValue& result)
{
  const llvm::SelectInst *selectInst = (llvm::SelectInst*)&instruction;

  for (int i = 0; i < result.num; i++)
  {
    const bool cond = getUnsignedInt(selectInst->getCondition(), i);
    const llvm::Value *op = cond ?
      selectInst->getTrueValue() :
      selectInst->getFalseValue();

    uint64_t u;
    double f;

    llvm::Type::TypeID type = op->getType()->getTypeID();
    if (type == llvm::Type::VectorTyID)
    {
      type = op->getType()->getVectorElementType()->getTypeID();
    }

    switch (type)
    {
    case llvm::Type::IntegerTyID:
      u = getUnsignedInt(op, i);
      setIntResult(result, u, i);
      break;
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
      f = getFloatValue(op, i);
      setFloatResult(result, f, i);
      break;
    default:
      cerr << "Unhandled type in select instruction: " << type << endl;
      break;
    }
  }
}

void WorkItem::sext(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t val = getSignedInt(instruction.getOperand(0), i);
    setIntResult(result, val, i);
  }
}

void WorkItem::shl(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a << b, i);
  }
}

void WorkItem::shuffle(const llvm::Instruction& instruction,
                       TypedValue& result)
{
  llvm::ShuffleVectorInst *shuffle = (llvm::ShuffleVectorInst*)&instruction;

  llvm::Value *v1 = shuffle->getOperand(0);
  llvm::Value *v2 = shuffle->getOperand(1);
  llvm::Value *mask = shuffle->getMask();

  unsigned num = v1->getType()->getVectorNumElements();
  llvm::Type *type = v1->getType()->getVectorElementType();
  for (int i = 0; i < result.num; i++)
  {
    llvm::Value *src = v1;
    unsigned int index = getUnsignedInt(mask, i);
    if (index == -1)
    {
      // Don't care / undef
      setIntResult(result, (uint64_t)0, i);
      continue;
    }
    else if (index >= num)
    {
      index -= num;
      src = v2;
    }

    switch (type->getTypeID())
    {
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
      setFloatResult(result, getFloatValue(src, index), i);
      break;
    case llvm::Type::IntegerTyID:
      setIntResult(result, getUnsignedInt(src, index), i);
      break;
    default:
      cerr << "Unhandled vector type " << type->getTypeID() << endl;
      return;
    }
  }
}

void WorkItem::sitofp(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    const llvm::CastInst *cast = (const llvm::CastInst*)&instruction;
    double r = (double)getSignedInt(instruction.getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

void WorkItem::srem(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction.getOperand(0), i);
    int64_t b = getSignedInt(instruction.getOperand(1), i);
    setIntResult(result, a % b, i);
  }
}

void WorkItem::store(const llvm::Instruction& instruction)
{
  const llvm::StoreInst *storeInst = (const llvm::StoreInst*)&instruction;
  const llvm::Value *ptrOp = storeInst->getPointerOperand();
  const llvm::Value *valOp = storeInst->getValueOperand();
  const llvm::Type *type = valOp->getType();
  unsigned addressSpace = storeInst->getPointerAddressSpace();

  // Get address
  size_t address = *((size_t*)m_privateMemory[ptrOp].data);

  // TODO: Genericise operand handling
  size_t size = getTypeSize(valOp->getType());
  unsigned char *data = new unsigned char[size];

  if (isConstantOperand(valOp))
  {
    if (valOp->getValueID() == llvm::Value::ConstantExprVal)
    {
      TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)valOp);
      memcpy(data, result.data, result.size*result.num);
      delete[] result.data;
    }
    else
    {
      getConstantData(data, (const llvm::Constant*)valOp);
    }
  }
  else
  {
    if (m_privateMemory.find(valOp) != m_privateMemory.end())
    {
      // TODO: Cleaner solution for this
      memcpy(data, m_privateMemory[valOp].data, size);
    }
    else if (valOp->getValueID() >= llvm::Value::InstructionVal)
    {
      execute(*(llvm::Instruction*)valOp);
      memcpy(data, m_privateMemory[valOp].data, m_privateMemory[valOp].size);
    }
    else
    {
      cerr << "Store operand not found." << endl;
    }
  }

  Memory *memory = NULL;

  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memory = m_stack;
    break;
  case AddrSpaceGlobal:
    memory = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    memory = m_workGroup.getLocalMemory();
    break;
  case AddrSpaceConstant:
    assert(false && "Store to constant address space");
    return;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  // Store data
  if (memory)
  {
    if (!memory->store(data, address, size))
    {
      outputMemoryError(instruction, "Invalid write",
                        addressSpace, address, size);
    }
  }

  delete[] data;
}

void WorkItem::sub(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a - b, i);
  }
}

void WorkItem::swtch(const llvm::Instruction& instruction)
{
  llvm::SwitchInst *swtch = (llvm::SwitchInst*)&instruction;
  llvm::Value *cond = swtch->getCondition();
  uint64_t val = getUnsignedInt(cond);
  llvm::ConstantInt *cval =
    (llvm::ConstantInt*)llvm::ConstantInt::get(cond->getType(), val);
  m_nextBlock = swtch->findCaseValue(cval).getCaseSuccessor();
}

void WorkItem::udiv(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a / b, i);
  }
}

void WorkItem::uitofp(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    const llvm::CastInst *cast = (const llvm::CastInst*)&instruction;
    double r = (double)getUnsignedInt(instruction.getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

void WorkItem::urem(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction.getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction.getOperand(1), i);
    setIntResult(result, a % b, i);
  }
}

void WorkItem::zext(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t val = getUnsignedInt(instruction.getOperand(0), i);
    setIntResult(result, val, i);
  }
}
