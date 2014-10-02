// WorkItem.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.`
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <sstream>

#include "llvm/DebugInfo.h"
#include "llvm/GlobalVariable.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"

#include "Device.h"
#include "Kernel.h"
#include "Memory.h"
#include "Program.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace oclgrind;
using namespace std;

#define COUNTED_LOAD_BASE  (llvm::Instruction::OtherOpsEnd + 4)
#define COUNTED_STORE_BASE (COUNTED_LOAD_BASE + 8)
#define COUNTED_CALL_BASE  (COUNTED_STORE_BASE + 8)

vector<size_t> WorkItem::m_instructionCounts;
vector<size_t> WorkItem::m_memopBytes;
vector<const llvm::Function*> WorkItem::m_countedFunctions;

WorkItem::WorkItem(Device *device, WorkGroup& workGroup, const Kernel& kernel,
                   size_t lid_x, size_t lid_y, size_t lid_z)
  : m_device(device), m_workGroup(workGroup), m_kernel(kernel)
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

  const size_t *globalSize = workGroup.getGlobalSize();
  m_globalIndex = (m_globalID[0] +
                  (m_globalID[1] +
                   m_globalID[2]*globalSize[1]) * globalSize[0]);

  // Load or create cached interpreter state
  m_cache = InterpreterCache::get(kernel.getProgram().getUID());
  assert(m_cache);

  // Set initial number of values to store based on cache
  m_values.resize(m_cache->valueIDs.size());

  // Store kernel arguments in private memory
  TypedValueMap::const_iterator argItr;
  for (argItr = kernel.args_begin(); argItr != kernel.args_end(); argItr++)
  {
    set(argItr->first, m_pool.clone(argItr->second));
  }

  m_privateMemory = new Memory(AddrSpacePrivate, device);

  list<const llvm::GlobalVariable*>::const_iterator varItr;
  for (varItr = kernel.vars_begin(); varItr != kernel.vars_end(); varItr++)
  {
    const llvm::Constant *init = (*varItr)->getInitializer();
    size_t size = getTypeSize(init->getType());
    size_t address = m_privateMemory->allocateBuffer(size);

    if (init->getValueID() == llvm::Value::ConstantExprVal)
    {
      TypedValue value = resolveConstExpr((const llvm::ConstantExpr*)init);
      m_privateMemory->store(value.data, address, size);
    }
    else if (isConstantOperand(init))
    {
      unsigned char *data = m_pool.alloc(size);
      getConstantData(data, init);
      m_privateMemory->store(data, address, size);
    }
    else
    {
      FATAL_ERROR("Unsupported global variable initializer: %d",
                  init->getValueID());
    }

    TypedValue var =
    {
      sizeof(size_t),
      1,
      m_pool.alloc(sizeof(size_t))
    };
    *(size_t*)var.data = address;
    set(*varItr, var);
  }

  m_prevBlock = NULL;
  m_nextBlock = NULL;
  m_currBlock = kernel.getFunction()->begin();
  m_currInst = m_currBlock->begin();

  m_state = READY;
}

WorkItem::~WorkItem()
{
  // Free private memory
  delete m_privateMemory;
}

void WorkItem::clearBarrier()
{
  if (m_state == BARRIER)
  {
    m_state = READY;
  }
}

void WorkItem::clearInstructionCounts()
{
  m_instructionCounts.clear();
  m_instructionCounts.resize(COUNTED_LOAD_BASE + 8);

  m_memopBytes.clear();
  m_memopBytes.resize(16);

  m_countedFunctions.clear();
}

void WorkItem::countInstruction(const llvm::Instruction *instruction)
{
  if (!m_device->isShowingInstructionCounts())
  {
    return;
  }

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
    size_t bytes = getTypeSize(type->getPointerElementType());
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
        find(m_countedFunctions.begin(), m_countedFunctions.end(), function);
      if (itr == m_countedFunctions.end())
      {
        opcode = COUNTED_CALL_BASE + m_countedFunctions.size();
        m_countedFunctions.push_back(function);
      }
      else
      {
        opcode = COUNTED_CALL_BASE + (itr - m_countedFunctions.begin());
      }
    }
  }

  if (opcode >= m_instructionCounts.size())
  {
    m_instructionCounts.resize(opcode+1);
  }
  m_instructionCounts[opcode]++;
}

void WorkItem::dispatch(const llvm::Instruction *instruction,
                        TypedValue& result)
{
  switch (instruction->getOpcode())
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
    br(instruction, result);
    break;
  case llvm::Instruction::Call:
    call(instruction, result);
    break;
  case llvm::Instruction::ExtractElement:
    extractelem(instruction, result);
    break;
  case llvm::Instruction::ExtractValue:
    extractval(instruction, result);
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
  case llvm::Instruction::FPToUI:
    fptoui(instruction, result);
    break;
  case llvm::Instruction::FPTrunc:
    fptrunc(instruction, result);
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
    insertelem(instruction, result);
    break;
  case llvm::Instruction::InsertValue:
    insertval(instruction, result);
    break;
  case llvm::Instruction::IntToPtr:
    inttoptr(instruction, result);
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
    store(instruction, result);
    break;
  case llvm::Instruction::Sub:
    sub(instruction, result);
    break;
  case llvm::Instruction::Switch:
    swtch(instruction, result);
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
  case llvm::Instruction::Unreachable:
    FATAL_ERROR("Encountered unreachable instruction");
  case llvm::Instruction::Xor:
    bwxor(instruction, result);
    break;
  case llvm::Instruction::ZExt:
    zext(instruction, result);
    break;
  default:
    FATAL_ERROR("Unsupported instruction: %s", instruction->getOpcodeName());
  }
}

void WorkItem::execute(const llvm::Instruction *instruction)
{
  // Prepare private variable for instruction result
  pair<size_t,size_t> resultSize = getValueSize(instruction);

  // Prepare result
  TypedValue result = {
    resultSize.first,
    resultSize.second,
    NULL
  };
  if (result.size)
  {
    result.data = m_pool.alloc(result.size*result.num);
  }

  if (instruction->getOpcode() != llvm::Instruction::PHI &&
      m_phiTemps.size() > 0)
  {
    TypedValueMap::iterator itr;
    for (itr = m_phiTemps.begin(); itr != m_phiTemps.end(); itr++)
    {
      set(itr->first, itr->second);
    }
    m_phiTemps.clear();
  }

  // Execute instruction
  countInstruction(instruction);
  dispatch(instruction, result);

  // Store result
  if (result.size)
  {
    if (instruction->getOpcode() != llvm::Instruction::PHI)
    {
      set(instruction, result);
    }
    else
    {
      m_phiTemps[instruction] = result;
    }
  }
}

TypedValue WorkItem::get(const llvm::Value *key) const
{
  return m_values[m_cache->valueIDs[key]];
}

const stack<ReturnAddress>& WorkItem::getCallStack() const
{
  return m_callStack;
}

const llvm::Instruction* WorkItem::getCurrentInstruction() const
{
  return m_currInst;
}

const size_t* WorkItem::getGlobalID() const
{
  return m_globalID;
}

size_t WorkItem::getGlobalIndex() const
{
  return m_globalIndex;
}

vector<size_t> WorkItem::getInstructionCounts()
{
  return m_instructionCounts;
}

double WorkItem::getFloatValue(const llvm::Value *operand,
                               unsigned int index)
{
  double val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = get(operand);
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
      FATAL_ERROR("Unsupported float size: %lu bytes", op.size);
    }
  }
  else if (operand->getValueID() == llvm::Value::ConstantVectorVal ||
           operand->getValueID() == llvm::Value::ConstantDataVectorVal)
  {
    val = getFloatValue(
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
  else if (id == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)operand);
    if (result.size == sizeof(float))
    {
      val = ((float*)result.data)[index];
    }
    else if (result.size == sizeof(double))
    {
      val = ((double*)result.data)[index];
    }
    else
    {
      FATAL_ERROR("Unsupported float size: %lu bytes", result.size);
    }
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
      FATAL_ERROR("Unsupported float semantics: %d", operand->getValueID());
    }
  }
  else
  {
    FATAL_ERROR("Unsupported float operand type: %d", id);
  }
  return val;
}

const size_t* WorkItem::getLocalID() const
{
  return m_localID;
}

Memory* WorkItem::getMemory(unsigned int addrSpace)
{
  switch (addrSpace)
  {
    case AddrSpacePrivate:
      return m_privateMemory;
    case AddrSpaceGlobal:
    case AddrSpaceConstant:
      return m_device->getGlobalMemory();
    case AddrSpaceLocal:
      return m_workGroup.getLocalMemory();
    default:
      FATAL_ERROR("Unsupported address space: %d", addrSpace);
  }
}

string WorkItem::getCountedOpcodeName(unsigned opcode)
{
  if (opcode >= COUNTED_CALL_BASE)
  {
    // Get functon name
    unsigned index = opcode - COUNTED_CALL_BASE;
    assert(index < m_countedFunctions.size());
    return "call " + m_countedFunctions[index]->getName().str() + "()";
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
    switch (opcode)
    {
      case AddrSpacePrivate:
        name << " private";
        break;
      case AddrSpaceGlobal:
        name << " global";
        break;
      case AddrSpaceConstant:
        name << " constant";
        break;
      case AddrSpaceLocal:
        name << " local";
        break;
      default:
        name << " unknown";
        break;
    }

    // Add number of bytes to name
    name << " (" << bytes << " bytes)";

    return name.str();
  }

  return llvm::Instruction::getOpcodeName(opcode);
}

Memory* WorkItem::getPrivateMemory() const
{
  return m_privateMemory;
}

size_t WorkItem::getPointer(const llvm::Value *operand, unsigned int index)
{
  unsigned id = operand->getValueID();
  if (id == llvm::Value::ArgumentVal ||
      id == llvm::Value::GlobalVariableVal ||
      id >= llvm::Value::InstructionVal)
  {
    return *(size_t*)get(operand).data;
  }
  else if (id == llvm::Value::UndefValueVal)
  {
    return -1;
  }
  else if (id == llvm::Value::ConstantExprVal)
  {
    size_t ptr;
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)operand);
    memcpy(&ptr, result.data + index*result.size, result.size);
    return ptr;
  }
  if (id == llvm::Value::ConstantAggregateZeroVal ||
      id == llvm::Value::ConstantPointerNullVal)
  {
    return 0;
  }
  if (id == llvm::Value::ConstantDataVectorVal ||
      id == llvm::Value::ConstantVectorVal)
  {
    return getPointer(
      ((const llvm::ConstantVector*)operand)->getAggregateElement(index));
  }
  else
  {
    FATAL_ERROR("Unsupported pointer operand type: %d", id);
  }
}

int64_t WorkItem::getSignedInt(const llvm::Value *operand,
                               unsigned int index)
{
  int64_t val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = get(operand);
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
      FATAL_ERROR("Unsupported signed int size: %lu bytes", op.size);
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
  else if (id == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)operand);
    memcpy(&val, result.data + index*result.size, result.size);
  }
  else if (id == llvm::Value::ConstantPointerNullVal)
  {
    return 0;
  }
  else
  {
    FATAL_ERROR("Unsupported signed operand type: %d", id);
  }
  return val;
}

WorkItem::State WorkItem::getState() const
{
  return m_state;
}

uint64_t WorkItem::getUnsignedInt(const llvm::Value *operand,
                                  unsigned int index)
{
  uint64_t val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = get(operand);
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
  else if (id == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)operand);
    memcpy(&val, result.data + index*result.size, result.size);
  }
  else if (id == llvm::Value::ConstantPointerNullVal)
  {
    return 0;
  }
  else
  {
    FATAL_ERROR("Unsupported unsigned operand type: %d", id);
  }

  return val;
}

const unsigned char* WorkItem::getValueData(const llvm::Value *value) const
{
  if (!has(value))
  {
    return NULL;
  }
  return get(value).data;
}

const llvm::Value* WorkItem::getVariable(std::string name) const
{
  map<string, const llvm::Value*>::const_iterator itr;
  itr = m_variables.find(name);
  if (itr == m_variables.end())
  {
    return NULL;
  }
  return itr->second;
}

bool WorkItem::has(const llvm::Value *key) const
{
  return m_cache->valueIDs.count(key);
}

bool WorkItem::printValue(const llvm::Value *value)
{
  if (!has(value))
  {
    return false;
  }

  printTypedData(value->getType(), get(value).data);

  return true;
}

bool WorkItem::printVariable(string name)
{
  // Find variable
  const llvm::Value *value = getVariable(name);
  if (!value)
  {
    return false;
  }

  // Get variable value
  TypedValue result = get(value);
  const llvm::Type *type = value->getType();

  if (((const llvm::Instruction*)value)->getOpcode()
       == llvm::Instruction::Alloca)
  {
    // If value is alloca result, look-up data at address
    const llvm::Type *elemType = value->getType()->getPointerElementType();
    size_t address = *(size_t*)result.data;
    size_t size = getTypeSize(elemType);
    unsigned char *data = m_pool.alloc(size);
    m_privateMemory->load(data, address, size);

    printTypedData(elemType, data);
  }
  else
  {
    printTypedData(type, result.data);
  }

  return true;
}

TypedValue WorkItem::resolveConstExpr(const llvm::ConstantExpr *expr)
{
  const llvm::Instruction *instruction = getConstExprAsInstruction(expr);
  pair<size_t,size_t> resultSize = getValueSize(instruction);
  TypedValue result =
    {
      resultSize.first,
      resultSize.second,
      m_pool.alloc(resultSize.first*resultSize.second)
    };

  dispatch(instruction, result);

  delete instruction;

  return result;
}

void WorkItem::set(const llvm::Value *key, TypedValue value)
{
  MAP<const llvm::Value*, size_t>::iterator itr = m_cache->valueIDs.find(key);
  if (itr == m_cache->valueIDs.end())
  {
    // Assign next index to value
    size_t pos = m_cache->valueIDs.size();
    m_cache->valueIDs[key] = pos;
    itr = m_cache->valueIDs.insert(make_pair(key, pos)).first;
  }

  // Resize vector if necessary
  if (m_values.size() <= itr->second)
  {
    m_values.resize(m_cache->valueIDs.size());
  }

  m_values[itr->second] = value;
}

void WorkItem::setFloatResult(TypedValue& result, double val,
                              unsigned int index)
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
    FATAL_ERROR("Unsupported float size: %lu bytes", result.size);
  }
}

void WorkItem::setIntResult(TypedValue& result, int64_t val,
                            unsigned int index)
{
  memcpy(result.data + index*result.size, &val, result.size);
}

void WorkItem::setIntResult(TypedValue& result, uint64_t val,
                            unsigned int index)
{
  memcpy(result.data + index*result.size, &val, result.size);
}

WorkItem::State WorkItem::step()
{
  if (m_state != READY)
  {
    FATAL_ERROR("Attempting to step a work-item in state %d", m_state);
  }

  // Execute the next instruction
  execute(m_currInst);

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


///////////////////////////////
//// Instruction execution ////
///////////////////////////////

#define INSTRUCTION(name) \
  void WorkItem::name(const llvm::Instruction *instruction, TypedValue& result)

INSTRUCTION(add)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a + b, i);
  }
}

INSTRUCTION(alloc)
{
  const llvm::AllocaInst *allocInst = ((const llvm::AllocaInst*)instruction);
  const llvm::Type *type = allocInst->getAllocatedType();

  // Perform allocation
  size_t size = getTypeSize(type);
  size_t address = m_privateMemory->allocateBuffer(size);

  // Create pointer to alloc'd memory
  *((size_t*)result.data) = address;
}

INSTRUCTION(ashr)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a >> b, i);
  }
}

INSTRUCTION(bitcast)
{
  const llvm::CastInst *cast = (const llvm::CastInst*)instruction;
  const llvm::Value *operand = cast->getOperand(0);
  switch (operand->getValueID())
  {
  case llvm::Value::ConstantExprVal:
  {
    TypedValue op = resolveConstExpr((const llvm::ConstantExpr*)operand);
    memcpy(result.data, op.data, result.size*result.num);
    break;
  }
  case llvm::Value::ConstantVectorVal:
  {
    const llvm::Type *type = operand->getType();
    const llvm::Type *elemType = type->getVectorElementType();
    size_t num = type->getVectorNumElements();
    size_t elemSize = getTypeSize(elemType);
    for (int i = 0; i < num; i++)
    {
      if (elemType->isIntegerTy())
      {
        uint64_t u = getUnsignedInt(operand, i);
        memcpy(result.data + elemSize*i, &u, elemSize);
      }
      else if (elemType->isFloatTy())
      {
        TypedValue tmp = result;
        tmp.size = elemSize;
        tmp.num = num;
        setFloatResult(tmp, getFloatValue(operand, i), i);
      }
    }
    break;
  }
  default:
    if (has(operand))
    {
      memcpy(result.data, get(operand).data, result.size*result.num);
    }
    else
    {
      FATAL_ERROR("Unsupported operand type: %d", operand->getValueID());
    }
    break;
  }
}

INSTRUCTION(br)
{
  if (instruction->getNumOperands() == 1)
  {
    // Unconditional branch
    m_nextBlock = (const llvm::BasicBlock*)instruction->getOperand(0);
  }
  else
  {
    // Conditional branch
    bool pred = *((bool*)get(instruction->getOperand(0)).data);
    const llvm::Value *iftrue = instruction->getOperand(2);
    const llvm::Value *iffalse = instruction->getOperand(1);
    m_nextBlock = (const llvm::BasicBlock*)(pred ? iftrue : iffalse);
  }
}

INSTRUCTION(bwand)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a & b, i);
  }
}

INSTRUCTION(bwor)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a | b, i);
  }
}

INSTRUCTION(bwxor)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a ^ b, i);
  }
}

INSTRUCTION(call)
{
  const llvm::CallInst *callInst = (const llvm::CallInst*)instruction;
  const llvm::Function *function = callInst->getCalledFunction();

  // Check for indirect function calls
  if (!callInst->getCalledFunction())
  {
    // Resolve indirect function pointer
    const llvm::Value *func = callInst->getCalledValue();
    const llvm::Value *funcPtr = ((const llvm::User*)func)->getOperand(0);
    function = (const llvm::Function*)funcPtr;
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
    for (argItr = function->arg_begin();
         argItr != function->arg_end(); argItr++)
    {
      const llvm::Value *arg = callInst->getArgOperand(i++);

      pair<size_t,size_t> argSize = getValueSize(arg);
      size_t size = argSize.first * argSize.second;
      TypedValue value = {
        argSize.first,
        argSize.second,
        m_pool.alloc(size)
      };

      unsigned id = arg->getValueID();
      if (id == llvm::Value::GlobalVariableVal ||
          id == llvm::Value::ArgumentVal ||
          id >= llvm::Value::InstructionVal)
      {
        memcpy(value.data, get(arg).data, size);
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
        FATAL_ERROR("Unsupported function argument type: %d", id);
      }

      set(argItr, value);
    }

    return;
  }

  // Check function cache
  map<const llvm::Function*, CachedBuiltin>::iterator fItr;
  fItr = m_cache->builtins.find(function);
  if (fItr != m_cache->builtins.end())
  {
    fItr->second.function.func(this, callInst,
                               fItr->second.name,
                               fItr->second.overload,
                               result,
                               fItr->second.function.op);
    return;
  }

  // Extract unmangled name and overload
  string name, overload;
  const string fullname = function->getName().str();
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

  // Find builtin function in map
  MAP<string,BuiltinFunction>::iterator bItr = workItemBuiltins.find(name);
  if (bItr != workItemBuiltins.end())
  {
    bItr->second.func(this, callInst, name, overload, result, bItr->second.op);

    const CachedBuiltin entry = {bItr->second, name, overload};
    m_cache->builtins[function] = entry;

    return;
  }

  // Check for builtin with matching prefix
  list< pair<string, BuiltinFunction> >::iterator pItr;
  for (pItr = workItemPrefixBuiltins.begin();
       pItr != workItemPrefixBuiltins.end(); pItr++)
  {
    if (name.compare(0, pItr->first.length(), pItr->first) == 0)
    {
      pItr->second.func(this, callInst, name,
                        overload, result, pItr->second.op);

      const CachedBuiltin entry = {pItr->second, name, overload};
      m_cache->builtins[function] = entry;

      return;
    }
  }

  // Function didn't match any builtins
  FATAL_ERROR("Undefined function: %s", name.c_str());
}

INSTRUCTION(extractelem)
{
  const llvm::ExtractElementInst *extract =
    (const llvm::ExtractElementInst*)instruction;

  const llvm::Value *vector = extract->getVectorOperand();
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
    FATAL_ERROR("Unsupported operand type: %d", type->getTypeID());
  }
}

INSTRUCTION(extractval)
{
  const llvm::ExtractValueInst *extract =
    (const llvm::ExtractValueInst*)instruction;
  const llvm::Value *agg = extract->getAggregateOperand();
  llvm::ArrayRef<unsigned int> indices = extract->getIndices();

  if (isConstantOperand(agg))
  {
    // Find constant value
    const llvm::Value *value = agg;
    for (int i = 0; i < indices.size(); i++)
    {
      value = ((llvm::Constant*)value)->getAggregateElement(indices[i]);
    }

    // Copy constant data to result
    getConstantData(result.data, (llvm::Constant*)value);
  }
  else
  {
    // Compute offset for target value
    int offset = 0;
    const llvm::Type *type = agg->getType();
    for (int i = 0; i < indices.size(); i++)
    {
      if (type->isArrayTy())
      {
        type = type->getArrayElementType();
        offset += getTypeSize(type) * indices[i];
      }
      else if (type->isStructTy())
      {
        offset += getStructMemberOffset((const llvm::StructType*)type,
                                        indices[i]);
        type = type->getStructElementType(indices[i]);
      }
      else
      {
        FATAL_ERROR("Unsupported aggregate type: %d", type->getTypeID())
      }
    }

    // Copy target value to result
    memcpy(result.data, get(agg).data + offset, result.size);
  }
}

INSTRUCTION(fadd)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction->getOperand(0), i);
    double b = getFloatValue(instruction->getOperand(1), i);
    setFloatResult(result, a + b, i);
  }
}

INSTRUCTION(fcmp)
{
  const llvm::CmpInst *cmpInst = (const llvm::CmpInst*)instruction;
  llvm::CmpInst::Predicate pred = cmpInst->getPredicate();

  uint64_t t = result.num > 1 ? -1 : 1;
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction->getOperand(0), i);
    double b = getFloatValue(instruction->getOperand(1), i);

    uint64_t r;
    switch (pred)
    {
    case llvm::CmpInst::FCMP_OEQ:
    case llvm::CmpInst::FCMP_UEQ:
      r = a == b;
      break;
    case llvm::CmpInst::FCMP_ONE:
    case llvm::CmpInst::FCMP_UNE:
      r = a != b;
      break;
    case llvm::CmpInst::FCMP_OGT:
    case llvm::CmpInst::FCMP_UGT:
      r = a > b;
      break;
    case llvm::CmpInst::FCMP_OGE:
    case llvm::CmpInst::FCMP_UGE:
      r = a >= b;
      break;
    case llvm::CmpInst::FCMP_OLT:
    case llvm::CmpInst::FCMP_ULT:
      r = a < b;
      break;
    case llvm::CmpInst::FCMP_OLE:
    case llvm::CmpInst::FCMP_ULE:
      r = a <= b;
      break;
    case llvm::CmpInst::FCMP_FALSE:
      r = false;
      break;
    case llvm::CmpInst::FCMP_TRUE:
      r = true;
      break;
    default:
      FATAL_ERROR("Unsupported FCmp predicate: %d", pred);
    }

    // Deal with NaN operands
    if (::isnan(a) || ::isnan(b))
    {
      r = !llvm::CmpInst::isOrdered(pred);
    }

    setIntResult(result, r ? t : 0, i);
  }
}

INSTRUCTION(fdiv)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction->getOperand(0), i);
    double b = getFloatValue(instruction->getOperand(1), i);
    setFloatResult(result, a / b, i);
  }
}

INSTRUCTION(fmul)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction->getOperand(0), i);
    double b = getFloatValue(instruction->getOperand(1), i);
    setFloatResult(result, a * b, i);
  }
}

INSTRUCTION(fpext)
{
  for (int i = 0; i < result.num; i++)
  {
    double r = getFloatValue(instruction->getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

INSTRUCTION(fptosi)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t r = (int64_t)getFloatValue(instruction->getOperand(0), i);
    setIntResult(result, r, i);
  }
}

INSTRUCTION(fptoui)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t r = (uint64_t)getFloatValue(instruction->getOperand(0), i);
    setIntResult(result, r, i);
  }
}

INSTRUCTION(frem)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction->getOperand(0), i);
    double b = getFloatValue(instruction->getOperand(1), i);
    setFloatResult(result, fmod(a, b), i);
  }
}

INSTRUCTION(fptrunc)
{
  for (int i = 0; i < result.num; i++)
  {
    double r = getFloatValue(instruction->getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

INSTRUCTION(fsub)
{
  for (int i = 0; i < result.num; i++)
  {
    double a = getFloatValue(instruction->getOperand(0), i);
    double b = getFloatValue(instruction->getOperand(1), i);
    setFloatResult(result, a - b, i);
  }
}

INSTRUCTION(gep)
{
  const llvm::GetElementPtrInst *gepInst =
    (const llvm::GetElementPtrInst*)instruction;

  // Get base address
  size_t address;
  const llvm::Value *base = gepInst->getPointerOperand();
  if (base->getValueID() == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)base);
    address = *(size_t*)result.data;
  }
  else
  {
    address = *((size_t*)get(base).data);
  }
  const llvm::Type *ptrType = gepInst->getPointerOperandType();

  // Iterate over indices
  llvm::User::const_op_iterator opItr;
  for (opItr = gepInst->idx_begin(); opItr != gepInst->idx_end(); opItr++)
  {
    int64_t offset = getSignedInt(opItr->get());

    if (ptrType->isPointerTy())
    {
      // Get pointer element size
      const llvm::Type *elemType = ptrType->getPointerElementType();
      address += offset*getTypeSize(elemType);
      ptrType = elemType;
    }
    else if (ptrType->isArrayTy())
    {
      // Get array element size
      const llvm::Type *elemType = ptrType->getArrayElementType();
      address += offset*getTypeSize(elemType);
      ptrType = elemType;
    }
    else if (ptrType->isVectorTy())
    {
      // Get vector element size
      const llvm::Type *elemType = ptrType->getVectorElementType();
      address += offset*getTypeSize(elemType);
      ptrType = elemType;
    }
    else if (ptrType->isStructTy())
    {
      address +=
        getStructMemberOffset((const llvm::StructType*)ptrType, offset);
      ptrType = ptrType->getStructElementType(offset);
    }
    else
    {
      FATAL_ERROR("Unsupported GEP base type: %d", ptrType->getTypeID());
    }
  }

  *((size_t*)result.data) = address;
}

INSTRUCTION(icmp)
{
  const llvm::CmpInst *cmpInst = (const llvm::CmpInst*)instruction;
  llvm::CmpInst::Predicate pred = cmpInst->getPredicate();

  uint64_t t = result.num > 1 ? -1 : 1;
  for (int i = 0; i < result.num; i++)
  {
    // Load operands
    const llvm::Value *opA = instruction->getOperand(0);
    const llvm::Value *opB = instruction->getOperand(1);
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
      FATAL_ERROR("Unsupported ICmp predicate: %d", pred);
    }

    setIntResult(result, r ? t : 0, i);
  }
}

INSTRUCTION(insertelem)
{
  const llvm::InsertElementInst *insert =
    (const llvm::InsertElementInst*)instruction;
  const llvm::Value *vector = insert->getOperand(0);
  unsigned int index = getUnsignedInt(insert->getOperand(2));
  const llvm::Type *type = vector->getType()->getVectorElementType();
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
      FATAL_ERROR("Unsupported operand type: %d", type->getTypeID());
    }
  }
}

INSTRUCTION(insertval)
{
  const llvm::InsertValueInst *insert =
    (const llvm::InsertValueInst*)instruction;

  // Load original aggregate data
  const llvm::Value *agg = insert->getAggregateOperand();
  if (isConstantOperand(agg))
  {
    getConstantData(result.data, (llvm::Constant*)agg);
  }
  else
  {
    memcpy(result.data, get(agg).data, result.size);
  }

  // Compute offset for inserted value
  int offset = 0;
  llvm::ArrayRef<unsigned int> indices = insert->getIndices();
  const llvm::Type *type = agg->getType();
  for (int i = 0; i < indices.size(); i++)
  {
    if (type->isArrayTy())
    {
      type = type->getArrayElementType();
      offset += getTypeSize(type) * indices[i];
    }
    else if (type->isStructTy())
    {
      offset += getStructMemberOffset((const llvm::StructType*)type,
                                      indices[i]);
      type = type->getStructElementType(indices[i]);
    }
    else
    {
      FATAL_ERROR("Unsupported aggregate type: %d", type->getTypeID())
    }
  }

  // Copy inserted value into result
  const llvm::Value *value = insert->getInsertedValueOperand();
  if (isConstantOperand(value))
  {
    getConstantData(result.data + offset, (const llvm::Constant*)value);
  }
  else
  {
    memcpy(result.data + offset, get(value).data,
           getTypeSize(value->getType()));
  }
}

INSTRUCTION(inttoptr)
{
  // Generate a mask to test pointer alignment
  const unsigned destSize =
    getTypeAlignment(instruction->getType()->getPointerElementType());
  const unsigned alignment = log2(destSize);
  const unsigned mask = ~(((unsigned)-1) << alignment);

  for (int i = 0; i < result.num; i++)  {
    uint64_t r = getUnsignedInt(instruction->getOperand(0), i);
    // Verify that the cast pointer fits the alignment requirements
    // of the destination type (undefined behaviour in C99)
    if ((r & mask) != 0) {
      m_device->notifyError("Invalid pointer cast - destination pointer is"
        " not aligned to the pointed type");
    }
    setIntResult(result, r, i);
  }
}

INSTRUCTION(itrunc)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t val = getUnsignedInt(instruction->getOperand(0), i);
    setIntResult(result, val, i);
  }
}

INSTRUCTION(load)
{
  const llvm::LoadInst *loadInst = (const llvm::LoadInst*)instruction;
  unsigned addressSpace = loadInst->getPointerAddressSpace();
  size_t address = getPointer(loadInst->getPointerOperand());

  // Generate a mask to test pointer alignment
  const unsigned destSize = getTypeAlignment(loadInst->getType());
  const unsigned alignment = log2(destSize);
  const unsigned mask = ~(((unsigned)-1) << alignment);
  if ((address & mask) != 0) {
    m_device->notifyError("Invalid memory load - source pointer is"
      " not aligned to the pointed type");
  }

  // Load data
  size_t size = result.size*result.num;
  getMemory(addressSpace)->load(result.data, address, size);
}

INSTRUCTION(lshr)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a >> b, i);
  }
}

INSTRUCTION(mul)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a * b, i);
  }
}

INSTRUCTION(phi)
{
  const llvm::PHINode *phiNode = (const llvm::PHINode*)instruction;
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
      memcpy(result.data, get(value).data, result.size);
      break;
    default:
      FATAL_ERROR("Unsupported operand type: %d", type);
    }
  }
}

INSTRUCTION(ptrtoint)
{
  const llvm::CastInst *cast = (const llvm::CastInst*)instruction;
  for (int i = 0; i < result.num; i++)
  {
    uint64_t r = getUnsignedInt(instruction->getOperand(0), i);
    setIntResult(result, r, i);
  }
}

INSTRUCTION(ret)
{
  const llvm::ReturnInst *retInst = (const llvm::ReturnInst*)instruction;

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
      if (isConstantOperand(returnVal))
      {
        if (returnVal->getValueID() == llvm::Value::ConstantExprVal)
        {
          TypedValue value =
            resolveConstExpr((const llvm::ConstantExpr*)returnVal);
          set(m_currInst, value);
        }
        else
        {
          pair<size_t,size_t> size = getValueSize(returnVal);
          TypedValue value =
          {
            size.first,
            size.second,
            m_pool.alloc(size.first*size.second)
          };
          getConstantData(value.data, (const llvm::Constant*)returnVal);
          set(m_currInst, value);
        }
      }
      else
      {
        set(m_currInst, m_pool.clone(get(returnVal)));
      }
    }
  }
  else
  {
    m_nextBlock = NULL;
    m_state = FINISHED;
    m_workGroup.notifyFinished(this);
  }
}

INSTRUCTION(sdiv)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction->getOperand(0), i);
    int64_t b = getSignedInt(instruction->getOperand(1), i);

    int64_t r = 0;
    if (b && !(a == INT64_MIN && b == -1))
    {
      r = a / b;
    }

    setIntResult(result, r, i);
  }
}

INSTRUCTION(select)
{
  const llvm::SelectInst *selectInst = (const llvm::SelectInst*)instruction;

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
      FATAL_ERROR("Unsupported operand type: %d", type);
    }
  }
}

INSTRUCTION(sext)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t val = getSignedInt(instruction->getOperand(0), i);
    if (instruction->getOperand(0)->getType()->getPrimitiveSizeInBits() == 1)
    {
      val = val ? -1 : 0;
    }
    setIntResult(result, val, i);
  }
}

INSTRUCTION(shl)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a << b, i);
  }
}

INSTRUCTION(shuffle)
{
  const llvm::ShuffleVectorInst *shuffle =
    (const llvm::ShuffleVectorInst*)instruction;

  const llvm::Value *v1 = shuffle->getOperand(0);
  const llvm::Value *v2 = shuffle->getOperand(1);
  const llvm::Value *mask = shuffle->getMask();

  unsigned num = v1->getType()->getVectorNumElements();
  const llvm::Type *type = v1->getType()->getVectorElementType();
  for (int i = 0; i < result.num; i++)
  {
    const llvm::Value *src = v1;
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
      FATAL_ERROR("Unsupported operand type: %d", type->getTypeID());
    }
  }
}

INSTRUCTION(sitofp)
{
  for (int i = 0; i < result.num; i++)
  {
    const llvm::CastInst *cast = (const llvm::CastInst*)instruction;
    double r = (double)getSignedInt(instruction->getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

INSTRUCTION(srem)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction->getOperand(0), i);
    int64_t b = getSignedInt(instruction->getOperand(1), i);

    int64_t r = 0;
    if (b && !(a == INT64_MIN && b == -1))
    {
      r = a % b;
    }

    setIntResult(result, r, i);
  }
}

INSTRUCTION(store)
{
  const llvm::StoreInst *storeInst = (const llvm::StoreInst*)instruction;
  const llvm::Value *valOp = storeInst->getValueOperand();
  const llvm::Type *type = valOp->getType();
  unsigned addressSpace = storeInst->getPointerAddressSpace();
  size_t address = getPointer(storeInst->getPointerOperand());

  // Generate a mask to test pointer alignment
  const unsigned alignment = log2(getTypeAlignment(type));
  const unsigned mask = ~(((unsigned)-1) << alignment);
  if ((address & mask) != 0) {
    m_device->notifyError("Invalid memory store - source pointer is"
      " not aligned to the pointed type");
  }

  // Get store operand
  size_t size = getTypeSize(type);
  unsigned char *data = m_pool.alloc(size);
  if (isConstantOperand(valOp))
  {
    if (valOp->getValueID() == llvm::Value::ConstantExprVal)
    {
      TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)valOp);
      memcpy(data, result.data, result.size*result.num);
    }
    else
    {
      getConstantData(data, (const llvm::Constant*)valOp);
    }
  }
  else
  {
    if (has(valOp))
    {
      memcpy(data, get(valOp).data, size);
    }
    else if (valOp->getValueID() >= llvm::Value::InstructionVal)
    {
      execute((const llvm::Instruction*)valOp);
      TypedValue result = get(valOp);
      memcpy(data, result.data, result.size*result.num);
    }
    else
    {
      FATAL_ERROR("Unable to find store operand");
    }
  }

  // Store data
  getMemory(addressSpace)->store(data, address, size);
}

INSTRUCTION(sub)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, a - b, i);
  }
}

INSTRUCTION(swtch)
{
  const llvm::SwitchInst *swtch = (const llvm::SwitchInst*)instruction;
  const llvm::Value *cond = swtch->getCondition();
  uint64_t val = getUnsignedInt(cond);
  const llvm::ConstantInt *cval =
    (const llvm::ConstantInt*)llvm::ConstantInt::get(cond->getType(), val);
  m_nextBlock = swtch->findCaseValue(cval).getCaseSuccessor();
}

INSTRUCTION(udiv)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, b ? a / b : 0, i);
  }
}

INSTRUCTION(uitofp)
{
  for (int i = 0; i < result.num; i++)
  {
    const llvm::CastInst *cast = (const llvm::CastInst*)instruction;
    double r = (double)getUnsignedInt(instruction->getOperand(0), i);
    setFloatResult(result, r, i);
  }
}

INSTRUCTION(urem)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t a = getUnsignedInt(instruction->getOperand(0), i);
    uint64_t b = getUnsignedInt(instruction->getOperand(1), i);
    setIntResult(result, b ? a % b : 0, i);
  }
}

INSTRUCTION(zext)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t val = getUnsignedInt(instruction->getOperand(0), i);
    setIntResult(result, val, i);
  }
}

#undef INSTRUCTION


////////////////////////////////
// WorkItem::InterpreterCache //
////////////////////////////////

MAP<unsigned long, WorkItem::InterpreterState*>
  WorkItem::InterpreterCache::m_cache;

void WorkItem::InterpreterCache::clear(unsigned long uid)
{
  MAP<unsigned long, InterpreterState*>::iterator itr = m_cache.find(uid);
  if (itr != m_cache.end())
  {
    delete itr->second;
    m_cache.erase(itr);
  }
}

WorkItem::InterpreterState* WorkItem::InterpreterCache::get(unsigned long uid)
{
  // Check for existing state
  MAP<unsigned long, InterpreterState*>::iterator itr = m_cache.find(uid);
  if (itr != m_cache.end())
  {
    return itr->second;
  }

  // Create new state
  InterpreterState *state = new InterpreterState;
  m_cache[uid] = state;

#if HAVE_CXX11
  state->valueIDs.reserve(1024); // TODO: Determine this number dynamically?
#endif

  return state;
}


//////////////////////////
// WorkItem::MemoryPool //
//////////////////////////

WorkItem::MemoryPool::MemoryPool(size_t blockSize) : m_blockSize(blockSize)
{
  // Force first allocation to create new block
  m_offset = m_blockSize;
}

WorkItem::MemoryPool::~MemoryPool()
{
  list<unsigned char*>::iterator itr;
  for (itr = m_blocks.begin(); itr != m_blocks.end(); itr++)
  {
    delete[] *itr;
  }
}

unsigned char* WorkItem::MemoryPool::alloc(size_t size)
{
  // Check if requested size larger than block size
  if (size > m_blockSize)
  {
    // Oversized buffers allocated separately from main pool
    unsigned char *buffer = new unsigned char[size];
    m_blocks.push_back(buffer);
    return buffer;
  }

  // Check if enough space in current block
  if (m_offset + size > m_blockSize)
  {
    // Allocate new block
    m_blocks.push_front(new unsigned char[m_blockSize]);
    m_offset = 0;
  }
  unsigned char *buffer = m_blocks.front() + m_offset;
  m_offset += size;
  return buffer;
}

TypedValue WorkItem::MemoryPool::clone(const TypedValue& source)
{
  TypedValue dest;
  dest.size = source.size;
  dest.num = source.num;
  dest.data = alloc(dest.size*dest.num);
  memcpy(dest.data, source.data, dest.size*dest.num);
  return dest;
}
