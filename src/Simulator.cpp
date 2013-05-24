#include "config.h"
#include <iostream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Module.h"
#include "llvm/Support/InstIterator.h"

#include "Simulator.h"

using namespace std;

Simulator::Simulator(llvm::Module *module)
{
  m_module = module;
}

void Simulator::run()
{
  for(llvm::Module::iterator fita = m_module->begin(); fita != m_module->end(); fita++)
  {
    cout << fita->getName().str() << ":" << endl;

    for (llvm::inst_iterator iita = inst_begin(fita); iita != inst_end(fita); iita++)
    {
      cout << "\t" << (&*iita)->getOpcodeName() << endl;
    }
  }
}
