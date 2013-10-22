// WorkItem.cpp (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#include "common.h"
#include <cxxabi.h>

#include "llvm/DebugInfo.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"

#include "Device.h"
#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

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

  // Store kernel arguments in private memory
  TypedValueMap::const_iterator argItr;
  for (argItr = kernel.args_begin(); argItr != kernel.args_end(); argItr++)
  {
    m_instResults[argItr->first] = clone(argItr->second);
  }

  m_privateMemory = kernel.getPrivateMemory()->clone();

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
  for (pmItr = m_instResults.begin();
       pmItr != m_instResults.end(); pmItr++)
  {
    delete[] pmItr->second.data;
  }
  delete m_privateMemory;
}

void WorkItem::clearBarrier()
{
  if (m_state == BARRIER)
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
  case llvm::Instruction::Unreachable:
    assert(false);
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

  if (instruction.getOpcode() != llvm::Instruction::PHI &&
      m_phiTemps.size() > 0)
  {
    TypedValueMap::iterator itr;
    for (itr = m_phiTemps.begin(); itr != m_phiTemps.end(); itr++)
    {
      if (m_instResults.find(itr->first) != m_instResults.end())
      {
        delete[] m_instResults[itr->first].data;
      }

      m_instResults[itr->first] = itr->second;
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
      if (m_instResults.find(&instruction) != m_instResults.end())
      {
        delete[] m_instResults[&instruction].data;
      }
      m_instResults[&instruction] = result;
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

double WorkItem::getFloatValue(const llvm::Value *operand,
                               unsigned int index)
{
  double val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = m_instResults.at(operand);
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
      cerr << "Unhandled float size: " << result.size << endl;
      return 0;
    }
    delete[] result.data;
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
      assert(false);
  }
}

Memory* WorkItem::getPrivateMemory() const
{
  return m_privateMemory;
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
    TypedValue op = m_instResults.at(operand);
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
  else if (id == llvm::Value::ConstantExprVal)
  {
    TypedValue result = resolveConstExpr((const llvm::ConstantExpr*)operand);
    memcpy(&val, result.data + index*result.size, result.size);
    delete[] result.data;
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
                                  unsigned int index)
{
  uint64_t val = 0;
  unsigned id = operand->getValueID();
  if (id == llvm::Value::GlobalVariableVal ||
      id == llvm::Value::ArgumentVal ||
      id >= llvm::Value::InstructionVal)
  {
    TypedValue op = m_instResults.at(operand);
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
    delete[] result.data;
  }
  else
  {
    cerr << "Unhandled unsigned operand type " << id << endl;
  }

  return val;
}

const unsigned char* WorkItem::getValueData(const llvm::Value *value) const
{
  TypedValueMap::const_iterator itr = m_instResults.find(value);
  if (itr == m_instResults.end())
  {
    return NULL;
  }
  return itr->second.data;
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

bool WorkItem::printValue(const llvm::Value *value)
{
  if (m_instResults.find(value) == m_instResults.end())
  {
    return false;
  }

  printTypedData(value->getType(), m_instResults[value].data);

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
  TypedValue result = m_instResults[value];
  const llvm::Type *type = value->getType();

  if (((const llvm::Instruction*)value)->getOpcode()
       == llvm::Instruction::Alloca)
  {
    // If value is alloca result, look-up data at address
    const llvm::Type *elemType = value->getType()->getPointerElementType();
    size_t address = *(size_t*)result.data;
    size_t size = getTypeSize(elemType);
    unsigned char *data = new unsigned char[size];
    m_privateMemory->load(data, address, size);

    printTypedData(elemType, data);

    delete[] data;
  }
  else
  {
    printTypedData(type, result.data);
  }

  return true;
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
    cerr << "Unhandled float size: " << dec << result.size << endl;
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
  assert(m_state == READY);

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
  m_workGroup.notifyFinished(this);
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
  size_t address = m_privateMemory->allocateBuffer(size);

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
  const llvm::Value *operand = cast->getOperand(0);
  switch (operand->getValueID())
  {
  case llvm::Value::ConstantExprVal:
  {
    TypedValue op = resolveConstExpr((const llvm::ConstantExpr*)operand);
    memcpy(result.data, op.data, result.size*result.num);
    delete[] op.data;
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
    if (m_instResults.find(operand) != m_instResults.end())
    {
      memcpy(result.data, m_instResults[operand].data, result.size*result.num);
    }
    else
    {
      cerr << "Unsupported bitcast operand type ("
           << operand->getValueID() << ")" << endl;
    }
    break;
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
    bool pred = *((bool*)m_instResults[instruction.getOperand(0)].data);
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
  const llvm::Function *function = callInst->getCalledFunction();

  // Check for indirect function calls
  if (!callInst->getCalledFunction())
  {
    // Resolve indirect function pointer
    const llvm::Value *func = callInst->getCalledValue();
    const llvm::Value *funcPtr = ((const llvm::User*)func)->getOperand(0);
    function = (const llvm::Function*)funcPtr;
  }

  string name, overload;
  const string fullname = function->getName().str();

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
    for (argItr = function->arg_begin();
         argItr != function->arg_end(); argItr++)
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
        memcpy(value.data, m_instResults[arg].data, size);
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

      if (m_instResults.find(argItr) != m_instResults.end())
      {
        delete[] m_instResults[argItr].data;
      }
      m_instResults[argItr] = value;
    }

    return;
  }

  // Find builtin function in map
  map<string,BuiltinFunction>::iterator bItr = workItemBuiltins.find(name);
  if (bItr != workItemBuiltins.end())
  {
    bItr->second.func(this, callInst, name, overload, result, bItr->second.op);
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
      return;
    }
  }

  // Function didn't match any builtins
  cerr << "Undefined function: " << name << endl;
}

void WorkItem::extractelem(const llvm::Instruction& instruction,
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

void WorkItem::extractval(const llvm::Instruction& instruction,
                          TypedValue& result)
{
  llvm::ExtractValueInst *extract = (llvm::ExtractValueInst*)&instruction;
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
        assert(false);
      }
    }

    // Copy target value to result
    memcpy(result.data, m_instResults[agg].data + offset, result.size);
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

void WorkItem::fptoui(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t r = (uint64_t)getFloatValue(instruction.getOperand(0), i);
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

void WorkItem::fptrunc(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    double r = getFloatValue(instruction.getOperand(0), i);
    setFloatResult(result, r, i);
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
    address = *((size_t*)m_instResults[base].data);
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
      address +=
        getStructMemberOffset((const llvm::StructType*)ptrType, offset);
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

void WorkItem::insertelem(const llvm::Instruction& instruction,
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

void WorkItem::insertval(const llvm::Instruction& instruction,
                         TypedValue& result)
{
  llvm::InsertValueInst *insert = (llvm::InsertValueInst*)&instruction;

  // Load original aggregate data
  const llvm::Value *agg = insert->getAggregateOperand();
  if (isConstantOperand(agg))
  {
    getConstantData(result.data, (llvm::Constant*)agg);
  }
  else
  {
    memcpy(result.data, m_instResults[agg].data, result.size);
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
      assert(false);
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
    memcpy(result.data + offset, m_instResults[value].data,
           getTypeSize(value->getType()));
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
    address = *((size_t*)m_instResults[ptrOp].data);
  }

  // Load data
  size_t size = result.size*result.num;
  if (!getMemory(addressSpace)->load(result.data, address, size))
  {
    m_device->notifyMemoryError(true, addressSpace, address, size);
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
      memcpy(result.data, m_instResults[value].data, result.size);
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
      if (m_instResults.find(m_currInst) != m_instResults.end())
      {
        delete[] m_instResults[m_currInst].data;
      }
      m_instResults[m_currInst] = clone(m_instResults[returnVal]);
    }
  }
  else
  {
    m_nextBlock = NULL;
    m_state = FINISHED;
    m_workGroup.notifyFinished(this);
  }
}

void WorkItem::sdiv(const llvm::Instruction& instruction, TypedValue& result)
{
  for (int i = 0; i < result.num; i++)
  {
    int64_t a = getSignedInt(instruction.getOperand(0), i);
    int64_t b = getSignedInt(instruction.getOperand(1), i);
    setIntResult(result, b ? a / b : 0, i);
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
    setIntResult(result, b ? a % b : 0, i);
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
  size_t address = *((size_t*)m_instResults[ptrOp].data);

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
    if (m_instResults.find(valOp) != m_instResults.end())
    {
      // TODO: Cleaner solution for this
      memcpy(data, m_instResults[valOp].data, size);
    }
    else if (valOp->getValueID() >= llvm::Value::InstructionVal)
    {
      execute(*(llvm::Instruction*)valOp);
      memcpy(data, m_instResults[valOp].data, m_instResults[valOp].size);
    }
    else
    {
      cerr << "Store operand not found." << endl;
    }
  }

  // Store data
  if (!getMemory(addressSpace)->store(data, address, size))
  {
    m_device->notifyMemoryError(false, addressSpace, address, size);
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
    setIntResult(result, b ? a / b : 0, i);
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
    setIntResult(result, b ? a % b : 0, i);
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
