#include "common.h"

#include "Kernel.h"

using namespace std;

Kernel::Kernel()
{
  m_localMemory = 0;
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

size_t Kernel::getLocalMemorySize() const
{
  return m_localMemory;
}

void Kernel::setArgument(const llvm::Value *arg, TypedValue value)
{
  m_arguments[arg] = value;
}

TypedValueMap::const_iterator Kernel::args_begin() const
{
  return m_arguments.begin();
}

TypedValueMap::const_iterator Kernel::args_end() const
{
  return m_arguments.end();
}
