#include "config.h"

class Simulator
{
public:
  Simulator(const llvm::Module *module);
  void run(std::string kernel);

private:
  const llvm::Module *m_module;
};
