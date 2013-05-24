#include "config.h"

namespace llvm
{
  class Instruction;
}

// Structure for a private variable
typedef struct
{
  size_t size;
  unsigned char *data;
} PrivateVariable;

// Private memory map type
typedef std::map<std::string,PrivateVariable> PrivateMemory;

class WorkItem
{
public:
  WorkItem(size_t gid_x, size_t gid_y=0, size_t gid_z=0);

  void dumpPrivateMemory() const;
  void execute(const llvm::Instruction& instruction);

private:
  size_t m_globalID[3];
  PrivateMemory m_privateMemory;
};
