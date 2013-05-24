#include "config.h"
#include <iostream>
#include <map>

#include "Kernel.h"

using namespace std;

Kernel::Kernel()
{
}

void Kernel::setArgument(const llvm::Value *arg, size_t value)
{
  m_arguments[arg] = value;
}
