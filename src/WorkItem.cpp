#include "config.h"
#include <iomanip>
#include <iostream>
#include <map>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Instruction.h"

#include "WorkItem.h"

using namespace std;

WorkItem::WorkItem(size_t gid_x, size_t gid_y, size_t gid_z)
{
  m_globalID[0] = gid_x;
  m_globalID[1] = gid_y;
  m_globalID[2] = gid_z;
}

void WorkItem::dumpPrivateMemory() const
{
  cout << "Work-item ("
       << m_globalID[0] << ","
       << m_globalID[1] << ","
       << m_globalID[2] << "):" << endl;

  PrivateMemory::const_iterator pmitr;
  for (pmitr = m_privateMemory.begin(); pmitr != m_privateMemory.end(); pmitr++)
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

void WorkItem::execute(const llvm::Instruction& instruction)
{
  // TODO: Compute actual result
  int result = 0;

  switch (instruction.getOpcode())
  {
  case llvm::Instruction::Call:
    break;
  case llvm::Instruction::GetElementPtr:
    break;
  case llvm::Instruction::Load:
    break;
  case llvm::Instruction::Store:
    break;
  case llvm::Instruction::FAdd:
    break;
  case llvm::Instruction::Ret:
    break;
  default:
    cout << "Unhandled instruction: " << instruction.getOpcodeName() << endl;
    break;
  }

  // TODO: Only allocate if not in map already?
  // TODO: Use actual size/type
  string dest = instruction.getName().str();
  if (!dest.empty())
  {
    PrivateVariable var;
    var.size = 4;
    var.data = new unsigned char(var.size);
    *((int*)var.data) = result;
    m_privateMemory[dest] = var;
  }
}
