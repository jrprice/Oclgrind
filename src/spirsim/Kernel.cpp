#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Function.h"
#include "llvm/Type.h"

#include "CL/cl.h"
#include "Kernel.h"

using namespace spirsim;
using namespace std;

Kernel::Kernel(const llvm::Function *function)
{
  m_function = function;
  m_localMemory = 0;

  m_globalSize[0] = 0;
  m_globalSize[1] = 0;
  m_globalSize[2] = 0;

  // TODO: Get required work-group size from metadata
  m_requiredWorkGroupSize[0] = 0;
  m_requiredWorkGroupSize[1] = 0;
  m_requiredWorkGroupSize[2] = 0;
}

Kernel::~Kernel()
{
}

const llvm::Argument* Kernel::getArgument(unsigned int index) const
{
  assert(index < getNumArguments());

  llvm::Function::const_arg_iterator argItr = m_function->arg_begin();
  for (int i = 0; i < index; i++)
  {
    argItr++;
  }
  return argItr;
}

size_t Kernel::getArgumentSize(unsigned int index) const
{
  const llvm::Type *type = getArgument(index)->getType();

  // Check if pointer argument
  if (type->isPointerTy())
  {
    return sizeof(size_t);
  }

  return type->getPrimitiveSizeInBits()>>3;
}

unsigned int Kernel::getArgumentType(unsigned int index) const
{
  const llvm::Type *type = getArgument(index)->getType();

  // Check if scalar argument
  if (!type->isPointerTy())
  {
    return CL_KERNEL_ARG_ADDRESS_PRIVATE;
  }

  // Check address space
  unsigned addressSpace = type->getPointerAddressSpace();
  switch (addressSpace)
  {
  case AddrSpaceGlobal:
    return CL_KERNEL_ARG_ADDRESS_GLOBAL;
  case AddrSpaceConstant:
    return CL_KERNEL_ARG_ADDRESS_CONSTANT;
  case AddrSpaceLocal:
    return CL_KERNEL_ARG_ADDRESS_LOCAL;
  default:
    cerr << "Unrecognized address space " << addressSpace << endl;
    return 0;
  }
}

const llvm::Function* Kernel::getFunction() const
{
  return m_function;
}

const size_t* Kernel::getGlobalSize() const
{
  return m_globalSize;
}

size_t Kernel::getLocalMemorySize() const
{
  return m_localMemory;
}

unsigned int Kernel::getNumArguments() const
{
  return m_function->arg_size();
}

const size_t* Kernel::getRequiredWorkGroupSize() const
{
  return m_requiredWorkGroupSize;
}

void Kernel::setArgument(unsigned int index, TypedValue value)
{
  if (index >= m_function->arg_size())
  {
    cerr << "Argument index out of range." << endl;
    return;
  }

  unsigned int type = getArgumentType(index);
  if (type == CL_KERNEL_ARG_ADDRESS_LOCAL)
  {
    TypedValue v = {
      value.size,
      new unsigned char[value.size]
    };
    *((size_t*)v.data) = m_localMemory;
    m_localMemory += value.size;

    m_arguments[getArgument(index)] = v;
  }
  else
  {
    m_arguments[getArgument(index)] = clone(value);
  }
}

void Kernel::setGlobalSize(const size_t globalSize[3])
{
  m_globalSize[0] = globalSize[0];
  m_globalSize[1] = globalSize[1];
  m_globalSize[2] = globalSize[2];
}

TypedValueMap::const_iterator Kernel::args_begin() const
{
  return m_arguments.begin();
}

TypedValueMap::const_iterator Kernel::args_end() const
{
  return m_arguments.end();
}
