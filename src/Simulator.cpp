#include "config.h"
#include <iomanip>
#include <iostream>
#include <map>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Module.h"
#include "llvm/Support/InstIterator.h"

#include "Simulator.h"

// Structure for a private variable
typedef struct
{
  size_t size;
  unsigned char *data;
} PrivateVariable;

using namespace std;

Simulator::Simulator(const llvm::Module *module)
{
  m_module = module;
}

void Simulator::run(string kernel)
{
  llvm::Module::const_iterator fita;
  llvm::const_inst_iterator iita;

  // TODO: Extend to multiple work items
  map<string,PrivateVariable> privateMemory;

  // Iterate over functions in module
  for(fita = m_module->begin(); fita != m_module->end(); fita++)
  {
    // Check kernel name
    if (fita->getName().str() != kernel)
      continue;

    cout << fita->getName().str() << ":" << endl;

    // Iterate over instructions in function
    for (iita = inst_begin(fita); iita != inst_end(fita); iita++)
    {
      cout << "\t" << (&*iita)->getOpcodeName() << endl;

      // TODO: Compute actual result
      int result = 0;

      // TODO: Only allocate if not in map already?
      // TODO: Use actual size/type
      string dest = (&*iita)->getName().str();
      if (!dest.empty())
      {
        PrivateVariable var;
        var.size = 4;
        var.data = new unsigned char(var.size);
        *((int*)var.data) = result;
        privateMemory[dest] = var;
      }

      //cout << "\t\t" << (&*iita)->getOperand(0)->getName().str() << endl;
    }
  }

  // Temporarily dump private memory (TODO: Remove)
  cout << "Private memory:" << endl;
  map<string,PrivateVariable>::iterator pmitr;
  for (pmitr = privateMemory.begin(); pmitr != privateMemory.end(); pmitr++)
  {
    // TODO: Interpret type?
    // TODO: Deal with larger private variables (e.g. arrays)
    cout << setw(12) << setfill(' ') << pmitr->first << ":";
    for (int i = 0; i < pmitr->second.size; i++)
    {
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)pmitr->second.data[i];
    }
    cout << setw(0) << endl;
  }
}
