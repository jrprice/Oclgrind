#include "common.h"

#include "Kernel.h"

using namespace std;

Kernel::Kernel(const llvm::Function *function)
{
  m_function = function;
  m_localMemory = 0;

  m_globalSize[0] = 0;
  m_globalSize[1] = 0;
  m_globalSize[2] = 0;
}

size_t Kernel::allocateLocalMemory(size_t size)
{
  // TODO: This doens't allow individual local memory arguments to be
  // modified later, and also makes local memory allocations
  // contiguous (more difficult to catch out of bounds accesses)
  size_t address = m_localMemory;
  m_localMemory += size;
  return address;
}

const llvm::Function* Kernel::getFunction() const
{
  return m_function;
}

size_t Kernel::getLocalMemorySize() const
{
  return m_localMemory;
}

const size_t* Kernel::getGlobalSize() const
{
  return m_globalSize;
}

void Kernel::setArgument(const llvm::Value *arg, TypedValue value)
{
  m_arguments[arg] = value;
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
