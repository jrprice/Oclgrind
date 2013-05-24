#include "config.h"

class Simulator
{
public:
  Simulator(llvm::Module *module);
  void run();

private:
  llvm::Module *m_module;
};
