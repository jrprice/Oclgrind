#include "config.h"
#include <iostream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/SourceMgr.h"

#include "Simulator.h"

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

  Simulator simulator(module);
  simulator.run("vecadd");
}
