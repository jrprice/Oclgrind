#include "common.h"

class Kernel
{
public:
  Kernel();

  TypedValueMap::const_iterator args_begin() const {return m_arguments.begin();};
  TypedValueMap::const_iterator args_end() const {return m_arguments.end();};
  void setArgument(const llvm::Value *arg, TypedValue value);

private:
  TypedValueMap m_arguments;
};
