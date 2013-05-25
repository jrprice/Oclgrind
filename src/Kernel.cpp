#include "common.h"

#include "Kernel.h"

using namespace std;

Kernel::Kernel()
{
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
