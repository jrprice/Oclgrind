#include "common.h"

namespace llvm
{
  class Instruction;
}

class GlobalMemory;
class Kernel;

class WorkItem
{
public:
  WorkItem(const Kernel& kernel, GlobalMemory &globalMem,
           size_t gid_x, size_t gid_y=0, size_t gid_z=0);

  void dumpPrivateMemory() const;
  void enableDebugOutput(bool enable) {m_debugOutput = enable;};
  void execute(const llvm::Instruction& instruction);

  float fadd(const llvm::Instruction& instruction);
  bool icmp(const llvm::Instruction& instruction);
  size_t gep(const llvm::Instruction& instruction);
  void load(const llvm::Instruction& instruction, unsigned char *dest);
  void store(const llvm::Instruction& instruction);

private:
  size_t m_globalID[3];
  TypedValueMap m_privateMemory;
  GlobalMemory& m_globalMemory;

  bool m_debugOutput;
};
