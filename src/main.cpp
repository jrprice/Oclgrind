#include "config.h"
#include <iostream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/SourceMgr.h"

using namespace std;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "Usage: oclgrind filename" << endl;
    return 1;
  }

  llvm::LLVMContext context;
  llvm::SMDiagnostic err;
  llvm::Module *module = ParseIRFile(argv[1], err, context);
  if (!module)
  {
    cout << "Failed to parse input file." << endl;
    return 1;
  }

  for(llvm::Module::iterator fita = module->begin(); fita != module->end(); fita++)
  {
    cout << fita->getName().str() << ":" << endl;

    for (llvm::inst_iterator iita = inst_begin(fita); iita != inst_end(fita); iita++)
    {
      cout << "\t" << (&*iita)->getOpcodeName() << endl;
    }
  }

  delete module;
}
