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
